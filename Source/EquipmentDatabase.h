/*--------------------------------------------------------------------

ファイル名：EquipmentDatabase.h
お仕事　　：武器／防具のマスターデータ参照

----------------------------------------------------------------------*/

#ifndef EQUIPMENTDATABASE_H
#define EQUIPMENTDATABASE_H

#include <vector>

#include "WeaponData.h"
#include "ArmorData.h"

class EquipmentDatabase
{
public:
    static const std::vector<WeaponData>& GetWeapons();
    static const std::vector<ArmorData>& GetArmorTemplates();

    static const WeaponData* FindWeapon(WeaponType type);
    static const ArmorData* FindArmor(ArmorSlot slot);
};

#endif
