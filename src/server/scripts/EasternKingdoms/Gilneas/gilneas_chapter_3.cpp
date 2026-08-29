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

#include "gilneas.h"
#include "ScriptMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"

namespace Gilneas::Chapter3
{
/*######
## Quest 24468 - Stranded at the Marsh
######*/

enum CrashSurvivor
{
    SPELL_SUMMON_SWAMP_CROCOLISK     = 69854,

    NPC_SWAMP_CROCOLISK              = 37078,

    QUEST_STRANDED_AT_THE_MARSH      = 24468,

    EVENT_CRASH_SURVIVOR_RESET       = 1
};

// The eleven survivors sit in the water at the Hailwood Marsh with nothing that a player
// can click: no gossip flag, no spellclick row and no quest item. What the data does carry
// is 69854 "Summon Swamp Crocolisk", and the quest counts kills of the crocolisk it spawns
// (quest_template.RequiredNpcOrGo1 = 37078) while displaying "Crash Survivor rescued". So
// the survivor springs the ambush on approach and the kill is the rescue.
constexpr float CrashSurvivorAmbushRange = 12.0f;

struct npc_gilneas_crash_survivor : public ScriptedAI
{
    npc_gilneas_crash_survivor(Creature* creature) : ScriptedAI(creature)
    {
        Initialize();
    }

    void Initialize()
    {
        _playerGUID.Clear();
        _crocoliskGUID.Clear();
        _ambushing = false;
    }

    void Reset() override
    {
        Initialize();
        _events.Reset();
        me->SetReactState(REACT_PASSIVE);
        me->SetStandState(UNIT_STAND_STATE_STAND);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (_ambushing)
            return;

        Player* player = who->ToPlayer();
        if (!player || player->IsGameMaster())
            return;

        if (player->GetQuestStatus(QUEST_STRANDED_AT_THE_MARSH) != QUEST_STATUS_INCOMPLETE)
            return;

        if (!me->IsWithinDistInMap(player, CrashSurvivorAmbushRange))
            return;

        _ambushing = true;
        _playerGUID = player->GetGUID();
        DoCastSelf(SPELL_SUMMON_SWAMP_CROCOLISK, true);
    }

    void JustSummoned(Creature* summon) override
    {
        if (summon->GetEntry() != NPC_SWAMP_CROCOLISK)
            return;

        summon->SetImmuneToPC(false);
        _crocoliskGUID = summon->GetGUID();

        if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
            summon->AI()->AttackStart(player);

        // The rescue can always be abandoned - release the survivor again after a while so
        // he does not stay locked for the next player who comes past.
        _events.ScheduleEvent(EVENT_CRASH_SURVIVOR_RESET, Minutes(2));
    }

    void SummonedCreatureDies(Creature* /*summon*/, Unit* /*killer*/) override
    {
        // Quest credit comes from the kill itself, the survivor only has to look rescued.
        _crocoliskGUID.Clear();
        me->SetStandState(UNIT_STAND_STATE_KNEEL);
        _events.RescheduleEvent(EVENT_CRASH_SURVIVOR_RESET, Seconds(20));
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            if (eventId == EVENT_CRASH_SURVIVOR_RESET)
            {
                // 69854 has SpellDuration index 21, i.e. no expiry, so an abandoned
                // crocolisk would sit in the marsh forever. Clear it out unless it is
                // still busy with someone.
                if (Creature* crocolisk = ObjectAccessor::GetCreature(*me, _crocoliskGUID))
                {
                    if (crocolisk->IsAlive() && crocolisk->IsInCombat())
                    {
                        _events.RescheduleEvent(EVENT_CRASH_SURVIVOR_RESET, Seconds(30));
                        continue;
                    }

                    crocolisk->DespawnOrUnsummon();
                }

                me->SetStandState(UNIT_STAND_STATE_STAND);
                Initialize();
            }
        }
    }

private:
    EventMap _events;
    ObjectGuid _playerGUID;
    ObjectGuid _crocoliskGUID;
    bool _ambushing;
};

/*######
## Quest 24616 - Losing Your Tail
######*/

enum DarkScout
{
    SPELL_FREEZING_TRAP_EFFECT       = 70794,
    SPELL_SUMMON_DARK_SCOUT          = 70795,
    SPELL_AIMED_SHOT                 = 70796,

    NPC_DARK_SCOUT                   = 37953,

    QUEST_LOSING_YOUR_TAIL           = 24616,

    SAY_TRAP_BROKEN                  = 0,

    EVENT_AIMED_SHOT                 = 1
};

constexpr float DarkScoutSearchRange = 40.0f;

struct npc_gilneas_dark_scout : public ScriptedAI
{
    npc_gilneas_dark_scout(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
    }

    void IsSummonedBy(Unit* summoner) override
    {
        if (!summoner)
            return;

        me->SetFacingToObject(summoner);
        Talk(SAY_TRAP_BROKEN);
        me->SetReactState(REACT_AGGRESSIVE);
        AttackStart(summoner);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_AIMED_SHOT, Seconds(4));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            if (eventId == EVENT_AIMED_SHOT)
            {
                DoCastVictim(SPELL_AIMED_SHOT);
                _events.Repeat(Seconds(8));
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
};

// 70797 - Belysra's Talisman
//
// The talisman is the quest's start item and its second effect is a dummy aimed at
// TARGET_UNIT_NEARBY_ENTRY, which the conditions row shipped alongside this script points
// at the Dark Scout. Two things happen here:
//
//   * the ranger's Freezing Trap (70794) is broken, which is what the quest text asks the
//     talisman to do;
//   * if no scout is around yet, the ambush is sprung from here.
//
// The second part is a deliberate deviation. Retail springs 70794 from something standing
// on the road north of the Bradshaw Mill - the trap itself already force-casts 70795, so
// the scout would arrive on its own - but nothing in this database casts 70794 and map 654
// carries no sniffed area trigger anywhere near that road. Until that placement exists,
// using the talisman is what reveals the ranger. Everything else (her line, her Aimed Shot
// and the kill credit through quest_template.RequiredNpcOrGo1) stays as authored.
class spell_gilneas_belysras_talisman : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_FREEZING_TRAP_EFFECT, SPELL_SUMMON_DARK_SCOUT });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        caster->RemoveAurasDueToSpell(SPELL_FREEZING_TRAP_EFFECT);

        // The scout was already waiting in her trap - wake her up rather than summoning a
        // second one.
        if (Creature* scout = GetHitCreature())
            if (!scout->IsInCombat())
            {
                scout->SetFacingToObject(caster);
                scout->AI()->Talk(SAY_TRAP_BROKEN);
                scout->SetReactState(REACT_AGGRESSIVE);
                scout->AI()->AttackStart(caster);
            }
    }

    void HandleAfterCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        if (player->GetQuestStatus(QUEST_LOSING_YOUR_TAIL) != QUEST_STATUS_INCOMPLETE)
            return;

        if (player->FindNearestCreature(NPC_DARK_SCOUT, DarkScoutSearchRange))
            return;

        player->CastSpell(player, SPELL_SUMMON_DARK_SCOUT, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gilneas_belysras_talisman::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
        AfterCast.Register(&spell_gilneas_belysras_talisman::HandleAfterCast);
    }
};
}

void AddSC_gilneas_chapter_3()
{
    using namespace Gilneas::Chapter3;
    RegisterCreatureAI(npc_gilneas_crash_survivor);
    RegisterCreatureAI(npc_gilneas_dark_scout);
    RegisterSpellScript(spell_gilneas_belysras_talisman);
}
