/* Copyright (C) 2009 - 2010 ScriptDevZero <http://github.com/scriptdevzero/scriptdevzero>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "scriptPCH.h"
#include "custom.h"
#include "ScriptedAI.h"
#include <ctime>
#include "Bag.h"
#include "Transmog/Transmogrification.h"

#define sT sTransmogrification

void ShowTransmogItems(Player* player, Creature* creature, uint8 slot, uint16 gossipPageNumber) // Only checks bags while can use an item from anywhere in inventory
{
    Item* oldItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (oldItem)
    {
        player->ADD_GOSSIP_ITEM(5, 100004, EQUIPMENT_SLOT_END + 3, slot);
        uint32 limit = 0;
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (limit > MAX_OPTIONS)
                break;
            Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!newItem)
                continue;
            if (!sT->CanTransmogrifyItemWithItem(player, oldItem->GetProto(), newItem->GetProto()))
                continue;
            if (sT->GetFakeEntry(oldItem->GetGUID()) == newItem->GetEntry())
                continue;
            ++limit;
            ItemPrototype const* proto = newItem->GetProto();
            std::string name = proto->Name1;
            ItemLocale const* il = sObjectMgr.GetItemLocale(newItem->GetEntry());
            if (il)
            {
                if (il->Name.size() > size_t(DB_LOCALE_zhCN) && !il->Name[DB_LOCALE_zhCN].empty())
                    name = il->Name[DB_LOCALE_zhCN];
            }
            //sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "ShowTransmogItems:%u %s", newItem->GetEntry(), name);
            player->ADD_GOSSIP_ITEM(6, name.c_str(), slot, newItem->GetEntry());
        }

        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            Bag* bag = (Bag*)player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!bag)
                continue;
            for (uint32 j = 0; j < bag->GetBagSize(); ++j)
            {
                if (limit > MAX_OPTIONS)
                    break;
                Item* newItem = player->GetItemByPos(i, j);
                if (!newItem)
                    continue;
                if (!sT->CanTransmogrifyItemWithItem(player, oldItem->GetProto(), newItem->GetProto()))
                    continue;
                if (sT->GetFakeEntry(oldItem->GetGUID()) == newItem->GetEntry())
                    continue;
                ++limit;
                ItemPrototype const* proto = newItem->GetProto();
                std::string name = proto->Name1;
                ItemLocale const* il = sObjectMgr.GetItemLocale(newItem->GetEntry());
                if (il)
                {
                    if (il->Name.size() > size_t(DB_LOCALE_zhCN) && !il->Name[DB_LOCALE_zhCN].empty())
                        name = il->Name[DB_LOCALE_zhCN];
                }
                //sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "ShowTransmogItems:%u %s", newItem->GetEntry(), name);
                player->ADD_GOSSIP_ITEM(6, name.c_str(), slot, newItem->GetEntry());
            }
        }
    }
    player->ADD_GOSSIP_ITEM(5, 100019, EQUIPMENT_SLOT_END + 1, slot);
    player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
}

bool OnGossipHello(Player *player, Creature * creature)
{
    player->ADD_GOSSIP_ITEM(5, 100005, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_HEAD);
    player->ADD_GOSSIP_ITEM(5, 100006, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_NECK);
    player->ADD_GOSSIP_ITEM(5, 100007, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_SHOULDERS);
    player->ADD_GOSSIP_ITEM(5, 100008, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_CHEST);
    player->ADD_GOSSIP_ITEM(5, 100009, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_WAIST);
    player->ADD_GOSSIP_ITEM(5, 100010, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_LEGS);
    player->ADD_GOSSIP_ITEM(5, 100011, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_FEET);
    player->ADD_GOSSIP_ITEM(5, 100012, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_WRISTS);
    player->ADD_GOSSIP_ITEM(5, 100013, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_HANDS);
    player->ADD_GOSSIP_ITEM(5, 100014, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_BACK);
    player->ADD_GOSSIP_ITEM(5, 100015, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_MAINHAND);
    player->ADD_GOSSIP_ITEM(5, 100016, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_OFFHAND);
    player->ADD_GOSSIP_ITEM(5, 100017, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_RANGED);
    player->ADD_GOSSIP_ITEM(5, 100018, EQUIPMENT_SLOT_END, EQUIPMENT_SLOT_TABARD);
    player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    return true;
}

bool OnGossipSelect(Player *player, Creature *creature, uint32 sender, uint32 action)
{
    player->PlayerTalkClass->ClearMenus();
    WorldSession* session = player->GetSession();
    switch (sender)
    {
        case EQUIPMENT_SLOT_END: // Show items you can use
            ShowTransmogItems(player, creature, action, sender);
            break;
        case EQUIPMENT_SLOT_END + 1: // Main menu
            OnGossipHello(player, creature);
            break;
        case EQUIPMENT_SLOT_END + 3: // Remove Transmogrification from single item
        {
            if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, action))
            {
                sT->DeleteFakeEntry(player, action, newItem);
                session->SendAreaTriggerMessage("%s", session->GetMangosString(LANG_ERR_UNTRANSMOG_OK));
                player->CLOSE_GOSSIP_MENU();
            }
        } break;
        default: // Transmogrify
        {
            //Item* itemTransmogrifier = Item::CreateItem(sender, 1, 0);
            //player->SetVisibleItemSlot(EQUIPMENT_SLOT_MAINHAND, itemTransmogrifier);
            TransmogAcoreStrings res = sT->Transmogrify(player, action, sender);
            if (res == LANG_ERR_TRANSMOG_OK)
                session->SendAreaTriggerMessage("%s", session->GetMangosString(LANG_ERR_TRANSMOG_OK));
            else
                session->SendNotification(res);
            player->CLOSE_GOSSIP_MENU();
        } break;
    }
    return true;
}

void AddSC_Transmog()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "npc_transmogrifier";
    newscript->pGossipHello = &OnGossipHello;
    newscript->pGossipSelect = &OnGossipSelect;
    newscript->RegisterSelf(false);
}
