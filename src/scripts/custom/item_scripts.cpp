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

bool ItemUse_item_x(Player* pPlayer, Item* pItem, const SpellCastTargets& /*pTargets*/)
{
    
    sLog.Out(LOG_BASIC, LOG_LVL_BASIC, "ItemUse_item_x");
    return true;
}

void AddSC_item_scripts()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "item_x";
    pNewScript->pItemUse = &ItemUse_item_x;
    pNewScript->RegisterSelf();
}
