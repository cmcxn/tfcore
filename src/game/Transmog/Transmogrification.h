#ifndef DEF_TRANSMOGRIFICATION_H
#define DEF_TRANSMOGRIFICATION_H

#include <unordered_map>
#include <vector>

#define HIDDEN_ITEM_ID 1 // used for hidden transmog - do not use a valid equipment ID
#define MAX_OPTIONS 25 // do not alter

enum MixedWeaponSettings
{
    MIXED_WEAPONS_STRICT = 0,
    MIXED_WEAPONS_MODERN = 1,
    MIXED_WEAPONS_LOOSE = 2
};

enum TransmogAcoreStrings // Language.h might have same entries, appears when executing SQL, change if needed
{
    LANG_ERR_TRANSMOG_OK = 900005, // change this
    LANG_ERR_TRANSMOG_INVALID_SLOT = 900006,
    LANG_ERR_TRANSMOG_MISSING_DEST_ITEM = 900007,
    LANG_ERR_TRANSMOG_INVALID_ITEMS = 900008,
    LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY = 900009,
    LANG_ERR_UNTRANSMOG_OK = 900010
};

class Transmogrification
{
public:
    static Transmogrification* instance();

    typedef std::unordered_map<uint32, uint32> transmogData;
    transmogData dataMap; // dataMap[iGUID] = pGUID

    float ScaledCostModifier = 1.0;
    int32 CopperCost = 500000;

    bool AllowMixedArmorTypes = false;
    bool AllowMixedWeaponHandedness = false;

    uint8 AllowMixedWeaponTypes = MIXED_WEAPONS_STRICT;

    bool IgnoreReqRace;
    bool IgnoreReqClass;
    bool IgnoreReqSkill;
    bool IgnoreReqSpell;
    bool IgnoreReqLevel;
    bool IgnoreReqEvent;
    bool IgnoreReqStats;

    bool IsRangedWeapon(uint32 Class, uint32 SubClass) const;
    bool IsAllowedQuality(uint32 quality) const;

    uint32 GetFakeEntry(ObjectGuid itemGUID) const;
    void UpdateItem(Player* player, Item* item) const;
    void DeleteFakeEntry(Player* player, uint8 slot, Item* itemTransmogrified/*, CharacterDatabaseTransaction* trans = nullptr*/);
    void SetFakeEntry(Player* player, uint32 newEntry, uint8 slot, Item* itemTransmogrified);

    TransmogAcoreStrings Transmogrify(Player* player, uint32 itemEntry, uint8 slot, /*uint32 newEntry, */bool no_cost = false);
    TransmogAcoreStrings Transmogrify(Player* player, Item* itemTransmogrifier, uint8 slot, /*uint32 newEntry, */bool no_cost = false, bool hidden_transmog = false);
    void OnEquip(Player* player, Item* pItem);
    void OnLogin(Player* player);
    void OnAfterPlayerSetVisibleItemSlot(Player* player, uint8 slot, Item* pItem);
    bool CanTransmogrifyItemWithItem(Player* player, ItemPrototype const* destination, ItemPrototype const* source) const;
    bool SuitableForTransmogrification(Player* player, ItemPrototype const* proto) const;
    bool IsItemTransmogrifiable(ItemPrototype const* proto) const;
    uint32 GetSpecialPrice(ItemPrototype const* proto) const;
};
#define sTransmogrification Transmogrification::instance()

#endif
