#include "CharData.h"
#include "Core/interfaces.h"
#include "Game/gamestates.h"

const std::list<std::string> idleWords =
    { "_NEUTRAL", "CmnActStand", "CmnActStandTurn", "CmnActStand2Crouch",
    "CmnActCrouch", "CmnActCrouchTurn", "CmnActCrouch2Stand",
    "CmnActFWalk", "CmnActBWalk",
    // Proxi block is triggered when an attack is closing in
    // If the player.hitstun is = 0, then those animations will still be considered idle
    "CmnActMidGuardPre", "CmnActMidHeavyGuardPre",
    "CmnActMidGuardEnd", "CmnActMidHeavyGuardEnd",
    "CmnActCrouchGuardEnd", "CmnActCrouchHeavyGuardLoop",
    "CmnActAirGuardEnd",
    // Character specifics
    "com3_kamae" // Mai 5xB stance
};

/*
const std::list<std::string> hitWords =
    { "CmnActHitStandLv1", "CmnActHitStandLv2", "CmnActHitStandLv3", "CmnActHitStandLv4", "CmnActHitStandLv5",
    "CmnActHitStandLowLv1", "CmnActHitStandLowLv1", "CmnActHitStandLowLv1", "CmnActHitStandLowLv1", "CmnActHitStandLowLv1", // not of size 21
    "CmnActHitCrouchLv1", "CmnActHitCrouchLv2", "CmnActHitCrouchLv3", "CmnActHitCrouchLv4", "CmnActHitCrouchLv5",
    "CmnActLockWait", // Thrown
    "CmnActFDownUpper", // Swept
    "CmnActKirinomiUpper", // Projected diagonally
    "CmnActVDownUpper",
    "CmnActStaggerLoop"
};

const std::list<std::string> guardWords =
    { "CmnActMidGuardLoop", "CmnActMidHeavyGuardLoop",
    "CmnActCrouchGuardLoop", "CmnActCrouchHeavyGuardLoop",
    "CmnActAirGuardLoop"};
*/

bool CharData::isDoingActionInList(const char currentAction[], const std::list<std::string> &listOfActions)
{
    const std::string currentActionString = currentAction;

    for (auto word : listOfActions)
    {
        if (currentActionString == word)
        {
            return true;
        }
    }
    return false;
}

bool CharData::isIdle(CharData& player)
{
    if (player.hitstun == 0 && (isDoingActionInList(player.currentAction, idleWords) || player.hitstunLeftover == 0))
    {
        return true;
    }
    return false;
}

bool CharData::isBlocking(CharData& player)
{
    if (player.hitstun > 0 && player.hitstunLeftover == 0) // not reliable after an emergency tech...
        return true;
    return false;
}

bool CharData::isInHitstun(CharData& player)
{
    // hitstun alone does not cover air cases (reaches 0 even when the opponent is still in actual hitstun)
    // hitstunLeftover reflects hitstun in the air as well, except after an emergency tech.
    if (player.hitstunLeftover > 0 || !isDoingActionInList(player.currentAction, idleWords))
    {
        return true;
    }
    return false;
}

void CharData::getFrameAdvantage(CharData& player1, CharData& player2, PlayersInteractionState& interaction)
{
    bool isIdle1 = isIdle(player1);
    bool isIdle2 = isIdle(player2);
    bool isStunned = isBlocking(player2) || isInHitstun(player2);
    int frameAdvantage;

    if (!isIdle1 && !isIdle2 && !interaction.started)
    {
        interaction.started = true;
        interaction.timer = 0;
    }
    if (interaction.started)
    {
        if (isIdle1 && isIdle2)
        {
            interaction.started = false;
            frameAdvantage = interaction.timer;
        }
        if (!isIdle1)
        {
            --interaction.timer;
        }
        if (!isIdle2)
        {
            ++interaction.timer;
        }
    }
}

void CharData::computeFramedataInteractions() //how to call every frame
{
    if (!(isInMatch && (*g_gameVals.pGameMode == GameMode_Training || *g_gameVals.pGameMode == GameMode_ReplayTheater)))
        return;

    PlayersInteractionState interaction;
    getFrameAdvantage(*g_interfaces.player1.GetData(), *g_interfaces.player2.GetData(), interaction);
}