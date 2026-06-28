#include "FrameAdvantage.h"

#include "Game/CharData.h"
#include "Core/interfaces.h"
#include "Game/gamestates.h"

PlayersInteractionState playersInteraction;
PlayerExtendedData player1;
PlayerExtendedData player2;

void getFrameAdvantage(const PlayerExtendedData& player1, const PlayerExtendedData& player2)
{
    bool isIdle1 = player1.isIdle();
    bool isIdle2 = player2.isIdle();
    bool isStunned = player2.isBlocking() || player2.isInHitstun();

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
    playersInteraction.inBlockstring = player.isBlocking() || player.isInHitstun();

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
        player1.updateCharData(g_interfaces.player1);
        player2.updateCharData(g_interfaces.player2);

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