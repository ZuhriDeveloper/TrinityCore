/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is where scripts' loading functions should be declared:

// [hook] playerbot: prabobots is not a script module -- GetScriptsBasePath only globs
// src/server/scripts -- so its scripts are registered from here instead. This is the one
// place the PRABOBOTS=0 and PRABOBOTS=1 builds differ; without the guard the symbol does
// not exist and worldserver fails to link.
#ifdef TRINITY_PRABOBOTS
void AddPlayerbotScripts();
#endif

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddCustomScripts()
{
#ifdef TRINITY_PRABOBOTS
    AddPlayerbotScripts();
#endif
}
