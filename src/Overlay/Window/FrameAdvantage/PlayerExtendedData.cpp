#include "PlayerExtendedData.h"

#include <Game/characters.h>

IdleActionToggles idleActionToggles;

namespace
{
    const std::vector<std::string> idleWords =
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

    const std::vector<std::string> airIdleWords =
    { "_NEUTRAL",
    "CmnActAirGuardPre", "CmnActAirGuardLoop", "CmnActAirGuardEnd", // Air
    "Flying_Start" }; // Izanami float start

    // --------------------------------------------------------------------------------
    bool isDoingActionInList(const char currentAction[], const std::vector<std::string>& listOfActions)
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
} // namespace


// --------------------------------------------------------------------------------
PlayerExtendedData::PlayerExtendedData()
{
    charData = charData;
    previousAction = "";
}

// --------------------------------------------------------------------------------
void PlayerExtendedData::updateCharData(const Player& player)
{
    charData = player.GetData();
}

// --------------------------------------------------------------------------------
void PlayerExtendedData::updatePreviousState()
{
    previousAction = std::string(charData->currentAction);
    previousPositionY = charData->position_y;
}

// --------------------------------------------------------------------------------
bool PlayerExtendedData::isAirborne() const
{
    // The character is "glued" to the ground (y = 0) one frame before actually
    // landing, so it isn't sufficient alone.
    return 0 < charData->position_y;
}

// --------------------------------------------------------------------------------
bool PlayerExtendedData::isMovingVertically() const
{
    return 0 != charData->offsetY_2;
}

// --------------------------------------------------------------------------------
bool PlayerExtendedData::isIdle() const
{
    if (isAirborne() || 0 < previousPositionY)
    {
        // These actions are air idle, but for the sake of computing ground frame
        // advantage, we will count them as non idle.
        if (::isDoingActionInList(charData->currentAction, airIdleWords))
        {
            // Izanami's float must be considered idle.
            if (charData->charIndex == CharIndex::CharIndex_Izanami
                && charData->isIzanamiFloating == 1)
            {
                return true;
            }

            return !isMovingVertically();
        }
    }

    // The landing post neutral tech is not actionable for 1F. It can be considered
    // part of the tech.
    const std::string currentActionString = charData->currentAction;
    if ("CmnActUkemiLandNLanding" == currentActionString
        && currentActionString != previousAction)
    {
        return false;
    }

    if (isBlocking() || isInHitstun())
    {
        return false;
    }

    if (::isDoingActionInList(charData->currentAction, idleWords))
    {
        return true;
    }

    if (idleActionToggles.ukemiStaggerHit)
    {
        if (ukemiStaggerIdle == charData->currentAction)
        {
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------
bool PlayerExtendedData::isBlocking() const
{
    return (charData->blockstun > 0 || charData->moveSpecialBlockstun > 0);
}

// --------------------------------------------------------------------------------
bool PlayerExtendedData::isInHitstun() const
{
    return (charData->hitstun > 0
        && !::isDoingActionInList(charData->currentAction, idleWords));
}