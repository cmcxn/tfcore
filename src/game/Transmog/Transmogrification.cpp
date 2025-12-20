#include "Transmogrification.h"
#include "ObjectMgr.h"

Transmogrification* Transmogrification::instance()
{
    static Transmogrification instance;
    return &instance;
}

uint32 Transmogrification::GetFakeEntry(ObjectGuid itemGUID) const
{
    //LOG_DEBUG("module", "Transmogrification::GetFakeEntry");

    transmogData::const_iterator itr = dataMap.find(itemGUID);
    if (itr == dataMap.end()) return 0;
    return itr->second;
}

void Transmogrification::UpdateItem(Player* player, Item* item) const
{
    if (item->IsEquipped())
    {
        player->SetVisibleItemSlot(item->GetSlot(), item);
        if (player->IsInWorld())
            item->SendCreateUpdateToPlayer(player);
    }
}

void Transmogrification::DeleteFakeEntry(Player* player, uint8 /*slot*/, Item* itemTransmogrified/*, CharacterDatabaseTransaction* trans /*= nullptr*/)
{
    uint32 itemGUID = itemTransmogrified->GetGUIDLow();
    if (dataMap.find(itemGUID) != dataMap.end())
        dataMap.erase(itemGUID);
    CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE GUID = %u", itemGUID);
    UpdateItem(player, itemTransmogrified);
}

void Transmogrification::SetFakeEntry(Player* player, uint32 newEntry, uint8 slot, Item* itemTransmogrified)
{
    uint32 itemGUID = itemTransmogrified->GetGUIDLow();
    dataMap[itemGUID] = newEntry;
    CharacterDatabase.PExecute("REPLACE INTO custom_transmogrification (GUID, FakeEntry, Owner) VALUES (%u, %u, %u)", itemGUID, newEntry, player->GetGUIDLow());
    UpdateItem(player, itemTransmogrified);
}

TransmogAcoreStrings Transmogrification::Transmogrify(Player* player, uint32 itemEntry, uint8 slot, /*uint32 newEntry, */bool no_cost) {
    if (itemEntry == UINT_MAX) // Hidden transmog
    {
        return Transmogrify(player, nullptr, slot, no_cost, true);
    }
    Item* itemTransmogrifier = Item::CreateItem(itemEntry, 1, 0);
    return Transmogrify(player, itemTransmogrifier, slot, no_cost, false);
}

TransmogAcoreStrings Transmogrification::Transmogrify(Player* player, Item* itemTransmogrifier, uint8 slot, /*uint32 newEntry, */bool no_cost, bool hidden_transmog)
{
    int32 cost = 0;
    // slot of the transmogrified item
    if (slot >= EQUIPMENT_SLOT_END)
    {
        // TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: HandleTransmogrifyItems - Player (GUID: {}, name: {}) tried to transmogrify an item (lowguid: {}) with a wrong slot ({}) when transmogrifying items.", player->GetGUIDLow(), player->GetName(), GUID_LOPART(itemGUID), slot);
        return LANG_ERR_TRANSMOG_INVALID_SLOT;
    }

    // transmogrified item
    Item* itemTransmogrified = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (!itemTransmogrified)
    {
        //TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: HandleTransmogrifyItems - Player (GUID: {}, name: {}) tried to transmogrify an invalid item in a valid slot (slot: {}).", player->GetGUIDLow(), player->GetName(), slot);
        return LANG_ERR_TRANSMOG_MISSING_DEST_ITEM;
    }

    if (hidden_transmog)
    {
        SetFakeEntry(player, HIDDEN_ITEM_ID, slot, itemTransmogrified); // newEntry
        return LANG_ERR_TRANSMOG_OK;
    }

    if (!itemTransmogrifier) // reset look newEntry
    {
        // Custom
        DeleteFakeEntry(player, slot, itemTransmogrified);
    }
    else
    {
        if (!CanTransmogrifyItemWithItem(player, itemTransmogrified->GetProto(), itemTransmogrifier->GetProto()))
        {
            //TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: HandleTransmogrifyItems - Player (GUID: {}, name: {}) failed CanTransmogrifyItemWithItem ({} with {}).", player->GetGUIDLow(), player->GetName(), itemTransmogrified->GetEntry(), itemTransmogrifier->GetEntry());
            return LANG_ERR_TRANSMOG_INVALID_ITEMS;
        }

        if (!no_cost)
        {
            cost = GetSpecialPrice(itemTransmogrified->GetProto());
            cost *= ScaledCostModifier;
            cost += CopperCost;

            if (cost) // 0 cost if reverting look
            {
                if (cost < 0)
                    //LOG_DEBUG("module", "Transmogrification::Transmogrify - {} ({}) transmogrification invalid cost (non negative, amount {}). Transmogrified {} with {}",
                    //    player->GetName(), player->GetGUID().ToString(), -cost, itemTransmogrified->GetEntry(), itemTransmogrifier->GetEntry());
                    sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "transmogrification invalid cost");
                else
                {
                    if (!(player->GetMoney() >= cost))
                        return LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY;
                    //sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "transmogrification invalid cost %d", cost);
                    player->ModifyMoney(-cost);
                }
            }
        }

        // Custom
        SetFakeEntry(player, itemTransmogrifier->GetEntry(), slot, itemTransmogrified); // newEntry
    }

    return LANG_ERR_TRANSMOG_OK;
}

void Transmogrification::OnEquip(Player* player, Item* pItem)
{
    
}

void Transmogrification::OnLogin(Player* player)
{
    uint32 playerGUID = player->GetGUIDLow();
    QueryResult* result = CharacterDatabase.PQuery("SELECT GUID, FakeEntry FROM custom_transmogrification WHERE Owner = %u", playerGUID);
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 itemGUID = fields[0].GetUInt32();
            uint32 fakeEntry = fields[1].GetUInt32();
            if (fakeEntry == 1 || sObjectMgr.GetItemPrototype(fakeEntry))
            {
                dataMap[itemGUID] = fakeEntry;
            }
        } while (result->NextRow());

        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                player->SetVisibleItemSlot(slot, item);
        }
    }
}

void Transmogrification::OnAfterPlayerSetVisibleItemSlot(Player* player, uint8 slot, Item* pItem)
{
    if (!pItem)
        return;

    if (uint32 entry = dataMap[pItem->GetGUIDLow()])
        player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_0 + (slot * MAX_VISIBLE_ITEM_OFFSET), entry);
}

bool Transmogrification::CanTransmogrifyItemWithItem(Player* player, ItemPrototype const* target, ItemPrototype const* source) const
{
    if (!target || !source)
        return false;

    if (source->ItemId == target->ItemId)
        return false;

    if (source->DisplayInfoID == target->DisplayInfoID)
        return false;

    if (source->Class != target->Class)
        return false;
	
	// 高等级可以幻化低等级，低等级不能幻化高等级
	//if (source->Quality > target->Quality)
    //    return false;

    if (source->InventoryType == INVTYPE_BAG ||
        source->InventoryType == INVTYPE_RELIC ||
        // source->InventoryType == INVTYPE_BODY ||
        source->InventoryType == INVTYPE_FINGER ||
        source->InventoryType == INVTYPE_TRINKET ||
        source->InventoryType == INVTYPE_AMMO ||
        source->InventoryType == INVTYPE_QUIVER)
        return false;

    if (target->InventoryType == INVTYPE_BAG ||
        target->InventoryType == INVTYPE_RELIC ||
        // target->InventoryType == INVTYPE_BODY ||
        target->InventoryType == INVTYPE_FINGER ||
        target->InventoryType == INVTYPE_TRINKET ||
        target->InventoryType == INVTYPE_AMMO ||
        target->InventoryType == INVTYPE_QUIVER)
        return false;

    if (!SuitableForTransmogrification(player, target) || !SuitableForTransmogrification(player, source))
        return false;

    if (IsRangedWeapon(source->Class, source->SubClass) != IsRangedWeapon(target->Class, target->SubClass))
        return false;

    if (source->SubClass != target->SubClass && !IsRangedWeapon(target->Class, target->SubClass))
    {
        if (source->Class == ITEM_CLASS_ARMOR && !AllowMixedArmorTypes)
            return false;
        if (source->Class == ITEM_CLASS_WEAPON)
        {
            if (AllowMixedWeaponTypes == MIXED_WEAPONS_STRICT)
            {
                return false;
            }
            if (AllowMixedWeaponTypes == MIXED_WEAPONS_MODERN)
            {
                switch (source->SubClass)
                {
                case ITEM_SUBCLASS_WEAPON_WAND:
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_FIST:
                    return false;
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_MACE:
                    if (target->SubClass != ITEM_SUBCLASS_WEAPON_MACE &&
                        target->SubClass != ITEM_SUBCLASS_WEAPON_AXE &&
                        target->SubClass != ITEM_SUBCLASS_WEAPON_SWORD)
                    {
                        return false;
                    }
                    break;
                case ITEM_SUBCLASS_WEAPON_AXE2:
                case ITEM_SUBCLASS_WEAPON_SWORD2:
                case ITEM_SUBCLASS_WEAPON_MACE2:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                    if (target->SubClass != ITEM_SUBCLASS_WEAPON_MACE2 &&
                        target->SubClass != ITEM_SUBCLASS_WEAPON_AXE2 &&
                        target->SubClass != ITEM_SUBCLASS_WEAPON_SWORD2 &&
                        target->SubClass != ITEM_SUBCLASS_WEAPON_STAFF &&
                        target->SubClass != ITEM_SUBCLASS_WEAPON_POLEARM)
                    {
                        return false;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }

    if (source->InventoryType != target->InventoryType)
    {

        // Main-hand to offhand restrictions - see https://wowpedia.fandom.com/wiki/Transmogrification
        if (!AllowMixedWeaponHandedness && AllowMixedWeaponTypes != MIXED_WEAPONS_LOOSE)
        {
            if ((source->InventoryType == INVTYPE_WEAPONMAINHAND && target->InventoryType != INVTYPE_WEAPONMAINHAND) ||
                (source->InventoryType == INVTYPE_WEAPONOFFHAND && target->InventoryType != INVTYPE_WEAPONOFFHAND))
            {
                return false;
            }

        }

        if (source->Class == ITEM_CLASS_WEAPON && !(IsRangedWeapon(target->Class, target->SubClass) ||
            (
                // [AZTH] Yehonal: fixed weapon check
                (target->InventoryType == INVTYPE_WEAPON || target->InventoryType == INVTYPE_2HWEAPON || target->InventoryType == INVTYPE_WEAPONMAINHAND || target->InventoryType == INVTYPE_WEAPONOFFHAND)
                && (source->InventoryType == INVTYPE_WEAPON || source->InventoryType == INVTYPE_2HWEAPON || source->InventoryType == INVTYPE_WEAPONMAINHAND || source->InventoryType == INVTYPE_WEAPONOFFHAND)
                )
            ))
            return false;
        if (source->Class == ITEM_CLASS_ARMOR &&
            !((source->InventoryType == INVTYPE_CHEST || source->InventoryType == INVTYPE_ROBE) &&
                (target->InventoryType == INVTYPE_CHEST || target->InventoryType == INVTYPE_ROBE)))
            return false;
    }

    return true;
}

bool Transmogrification::SuitableForTransmogrification(Player* player, ItemPrototype const* proto) const
{
    // ItemTemplate const* proto = item->GetTemplate();
    if (!player || !proto)
        return false;

    if (proto->Class != ITEM_CLASS_ARMOR &&
        proto->Class != ITEM_CLASS_WEAPON)
        return false;

    if (!IsItemTransmogrifiable(proto))
        return false;

    if (!IgnoreReqClass && (proto->AllowableClass & player->GetClassMask()) == 0)
        return false;

    if (!IgnoreReqRace && (proto->AllowableRace & player->GetRaceMask()) == 0)
        return false;

    if (!IgnoreReqSkill && proto->RequiredSkill != 0)
    {
        if (player->GetSkillValue(proto->RequiredSkill) == 0)
            return false;

        if (player->GetSkillValue(proto->RequiredSkill) < proto->RequiredSkillRank)
            return false;
    }

    if (!IgnoreReqLevel && player->GetLevel() < proto->RequiredLevel)
        return false;

    if (!IgnoreReqSpell && proto->RequiredSpell != 0 && !player->HasSpell(proto->RequiredSpell))
        return false;

    return true;
}

bool Transmogrification::IsItemTransmogrifiable(ItemPrototype const* proto) const
{
    if (!proto)
        return false;

    if (!IsAllowedQuality(proto->Quality)) // (proto->Quality == ITEM_QUALITY_LEGENDARY)
        return false;

    return true;
}

uint32 Transmogrification::GetSpecialPrice(ItemPrototype const* proto) const
{
    uint32 cost = proto->SellPrice < 10000 ? 10000 : proto->SellPrice;
    return cost;
}

bool Transmogrification::IsRangedWeapon(uint32 Class, uint32 SubClass) const
{
    return Class == ITEM_CLASS_WEAPON && (
        SubClass == ITEM_SUBCLASS_WEAPON_BOW ||
        SubClass == ITEM_SUBCLASS_WEAPON_GUN ||
        SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW);
}

bool Transmogrification::IsAllowedQuality(uint32 quality) const
{
    switch (quality)
    {
    case ITEM_QUALITY_NORMAL: return true;
    case ITEM_QUALITY_UNCOMMON: return true;
    case ITEM_QUALITY_RARE: return true;
    case ITEM_QUALITY_EPIC: return true;
    case ITEM_QUALITY_LEGENDARY: return true;
    case ITEM_QUALITY_ARTIFACT: return true;
    default: return false;
    }
}
