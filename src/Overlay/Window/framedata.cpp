#include "framedata.h"
#include "Game/CharData.h"
#include "Core/interfaces.h"
#include "Game/gamestates.h"

IdleToggles idleToggles;
PlayersInteractionState playersInteraction;
PlayerExtendedData player1;
PlayerExtendedData player2;

const std::list<std::string> idleWords =
{ "_NEUTRAL", "CmnActStand", "CmnActStandTurn", "CmnActStand2Crouch",
"CmnActCrouch", "CmnActCrouchTurn", "CmnActCrouch2Stand",
"CmnActFWalk", "CmnActBWalk",
"CmnActFDash", "CmnActFDashStop",
"CmnActJumpLanding", "CmnActLandingStiffEnd",
"CmnActUkemiLandNLanding",
// Proxi block is triggered when an attack is closing in without being actually blocked
// If the player.blockstun is = 0, then those animations are still considered idle
"CmnActCrouchGuardPre", "CmnActCrouchGuardLoop", "CmnActCrouchGuardEnd",                 // Crouch
"CmnActCrouchHeavyGuardPre", "CmnActCrouchHeavyGuardLoop", "CmnActCrouchHeavyGuardEnd",  // Crouch Heavy
"CmnActMidGuardPre", "CmnActMidGuardLoop", "CmnActMidGuardEnd",                          // Mid
"CmnActMidHeavyGuardPre", "CmnActMidHeavyGuardLoop", "CmnActMidHeavyGuardEnd",           // Mid Heavy
"CmnActHighGuardPre", "CmnActHighGuardLoop", "CmnActHighGuardEnd",                       // High
"CmnActHighHeavyGuardPre", "CmnActHighHeavyGuardLoop", "CmnActHighHeavyGuardEnd",        // High Heavy
// Character specifics
"com3_kamae" // Mai 5xB stance
};

const std::string ukemiStaggerIdle = "CmnActUkemiStagger";

const std::list<std::string> airIdleWords =
{ "_NEUTRAL",
"CmnActAirGuardPre", "CmnActAirGuardLoop", "CmnActAirGuardEnd", // Air
"Flying_Start" }; // Izanami float

bool isDoingActionInList(const char currentAction[], const std::list<std::string>& listOfActions)
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

bool isAirborne(const PlayerExtendedData& player)
{
    // The character is "glued" to the ground (y = 0) one frame before actually landing, so it isn't sufficient alone.
    return 0 < player.charData->position_y;
}

bool isMovingVertically(const PlayerExtendedData& player)
{
    return 0 != player.charData->offsetY_2;
}

bool isIdle(const PlayerExtendedData& player)
{
    if (isAirborne(player) || 0 < player.previousPositionY)
    {
        // These actions are air idle, but for the sake of computing ground frame advantage, we will count them as non idle.
        if (isDoingActionInList(player.charData->currentAction, airIdleWords))
        {
            // Izanami's float must be considered idle.
            if (player.charData->charIndex == CharIndex::CharIndex_Izanami && player.charData->isIzanamiFloating == 1)
            {
                return true;
            }

            return !isMovingVertically(player);
        }
    }

    // The landing post neutral tech is not actionable for 1F. It can be considered part of the tech.
    const std::string currentActionString = player.charData->currentAction;
    if ("CmnActUkemiLandNLanding" == currentActionString && currentActionString != player.previousAction)
    {
        return false;
    }

    if (isBlocking(player) || isInHitstun(player))
    {
        return false;
    }

    if (isDoingActionInList(player.charData->currentAction, idleWords))
    {
        return true;
    }

    if (idleToggles.ukemiStaggerHit)
    {
        if (ukemiStaggerIdle == player.charData->currentAction)
        {
            return true;
        }
    }

    return false;
}

bool isBlocking(const PlayerExtendedData& player)
{
    if (player.charData->blockstun > 0 || player.charData->moveSpecialBlockstun > 0)
        return true;
    return false;
}

bool isInHitstun(const PlayerExtendedData& player)
{
    if (player.charData->hitstun > 0 && !isDoingActionInList(player.charData->currentAction, idleWords))
        return true;
    return false;
}

void getFrameAdvantage(const PlayerExtendedData& player1, const PlayerExtendedData& player2)
{
    bool isIdle1 = isIdle(player1);
    bool isIdle2 = isIdle(player2);
    bool isStunned = isBlocking(player2) || isInHitstun(player2);

    if (!isIdle1 && !isIdle2)
    {
        playersInteraction.started = true;
        playersInteraction.timer = 0;
    }
    if (playersInteraction.started)
    {
        if (isIdle1 && isIdle2)
        {
            playersInteraction.started = false;
            playersInteraction.frameAdvantageToDisplay = playersInteraction.timer;
        }
        if (!isIdle1)
        {
            playersInteraction.timer -= 1;
        }
        if (!isIdle2)
        {
            playersInteraction.timer += 1;
        }
    }
}

void computeGaps(const PlayerExtendedData& player, int& gapCounter, int& gapResult)
{
    playersInteraction.inBlockstring = isBlocking(player) || isInHitstun(player);

    if (playersInteraction.inBlockstring)
    {
        if (gapCounter > 0 && gapCounter <= 30)
        {
            gapResult = gapCounter;
        }
        gapCounter = 0; //resets everytime you are in block or hit stun
    }
    else if (!playersInteraction.inBlockstring)
    {
        ++gapCounter;
        gapResult = -1;
    }
}

bool hasWorldTimeMoved()
{
    if (playersInteraction.prevFrameCount < *g_gameVals.pFrameCount)
    {
        playersInteraction.prevFrameCount = *g_gameVals.pFrameCount;
        return true;
    }

    playersInteraction.prevFrameCount = *g_gameVals.pFrameCount;
    return false;
}

void computeFramedataInteractions()
{
    if (!isInMatch && !(*g_gameVals.pGameMode == GameMode_Training || *g_gameVals.pGameMode == GameMode_ReplayTheater))
        return;

    if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr())
    {
        player1.charData = g_interfaces.player1.GetData();
        player2.charData = g_interfaces.player2.GetData();

        if (hasWorldTimeMoved())
        {
            computeGaps(player1, playersInteraction.p1Gap, playersInteraction.p1GapDisplay);
            computeGaps(player2, playersInteraction.p2Gap, playersInteraction.p2GapDisplay);
            getFrameAdvantage(player1, player2);

            player1.updatePreviousState();
            player2.updatePreviousState();
        }
    }
}