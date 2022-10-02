#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "CbrData.h"
#include "Metadata.h"


#define rWallDist 1850000
#define lWallDist -1850000
#define maxWallDist 3700000
//(1850000/4)
#define maxXDist 462500 
//(800000/2)
#define maxYDist 400000 

#define costXDist 1
#define costYDist 1
#define costWallDist 0.8
#define costAiState 0.2
#define costEnemyState 0.1
#define costAiNeutral 1
#define costEnemyNeutral 1
#define costAiAir 1
#define costEnemyAir 1
#define costAiWakeup 1
#define costEnemyWakeup 1
#define costAiBlocking 1
#define costEnemyBlocking 1
#define costAiHitThisFrame 1
#define costEnemyHitThisFrame 1
#define costAiBlockThisFrame 1
#define costEnemyBlockhisFrame 1
#define costAiCrouching 0.1
#define costEnemyCrouching 0.1

#define nextBestMulti 1.3
#define nextBestAdd 0.05

#define specialButton 512
#define tauntButton 256
#define DButton 128
#define CButton 64
#define BButton 32
#define AButton 16
/*
var cbrParameters = CBRParameters{
    gameStateMaxChange:       5,    //every frame during gameplay if we check the current gamestate against the currently running case and it got this much worse, we check for a new case
    betterCaseThreshold : 0.30, //when the next case in a sequence would be played, if another case exists that is this much better, switch to that case
    topSelectionThreshold : 0.00, //when a best case is found to avoid always selecting the same case in similar situation choose a random worse. This parameter determines how much worse at most.
    maxXPositionComparison : 300,  //If X position comparisons are more than this amount shifted, similarity for them is 1
    maxYPositionComparison : 200,  //If Y position comparisons are more than this amount shifted, similarity for them is 1
    maxVelocityComparison : 10,   //The max difference in velocity before the comparison function hits max dissimilarity
    curGamestateQueLength : 12,   // how many frames in a row are stored to compare against in the comparison function. Stored in aiData.curGamestate
    maxInputBufferDifference : 30,   //How many frames of input buffering can be off before reaching max dissimilarity
    maxHitstunDifference : 10,   //how many frames of difference of beeing in hitstun is allowed till max dissimilarity
    maxBlockstunDifference : 10,   //how many frames of difference of beeing in blockstun is allowed till max dissimilarity
    maxAttackStateDiff : 20,   //how many frames of beeing in an attack state are allowed till max dissimilarity
    nearWallDist : 0.13, //percent of how close compared to current stage size a character has to be, to be considered near the wall
    repetitionFrames : 60,   //amount of frames after which a case was used where it will be taxed for beeing used again. Multiplied the more a case is used.
    comboLength : 20,

    cps : comparisonParameters{
        //parameters that determine how strongly different comparison functions are evaluated
        XRelativePosition:    1.0,
        YRelativePosition : 1.0,
        xVelocityComparison : 0.25,
        yVelocityComparison : 0.25,
        inputBufferDirection : 1.0,
        inputBufferButton : 1.0,
        airborneState : 1.0,
        lyingDownState : 1.0,
        hitState : 1.0,
        blockState : 1.0,
        attackState : 1.0,
        nearWall : 0.3,
        moveID : 0.5,
        pressureMoveID : 0.8,
        getHit : 1.0,
        didHit : 1.0,
        frameAdv : 0.3,
        frameAdvInitiator : 0.1,
        comboSimilarity : 1.0,

        objectOrder : 0.3,
        caseReuse : 0.5,
        roundState : 100.0,

        helperRelativePositionX : 0.5,
        helperRelativePositionY : 0.5,
        helperXVelocityComparison : 0.25,
        helperYVelocityComparison : 0.25,

        enemyXVelocityComparison : 0.25,
        enemyYVelocityComparison : 0.25,
        enemyAirborneState : 1.0,
        enemyLyingDownState : 1.0,
        enemyHitState : 1.0,
        enemyBlockState : 1.0,
        enemyAttackState : 1.0,
        enemyMoveID : 0.5,
        enemyPressureMoveID : 0.8,

        enemyHelperRelativePositionX : 1.0,
        enemyHelperRelativePositionY : 1.0,
        enemyHelperXVelocityComparison : 0.25,
        enemyHelperYVelocityComparison : 0.25,
    },
}*/

  



CbrData::CbrData()
{
}

CbrData::CbrData(std::string n, std::string p1) :playerName(n), characterName(p1)
{
    replayFiles = {};
    activeReplay = -1;
    activeCase = -1;
    activeFrame = -1;
    enabled = true;
}

void CbrData::setEnabled(bool b) {
    enabled = b;
}
bool CbrData::getEnabled() {
    return enabled;
}

void CbrData::AddReplay(CbrReplayFile f)
{
    replayFiles.push_back(f);
}

CbrReplayFile* CbrData::getLastReplay() {
    return  &replayFiles[replayFiles.size() - 1];
}

int CbrData::CBRcomputeNextAction(Metadata curGamestate) {
    float bestComparison = 9999999;
    int bestReplay = -1;
    int bestCase = -1;
    int bestFrame = -1;

    activeFrame++;
    //if we are searching for a case for the first time or the case is done playing or the player is hit/lands a hit search for a new case
    if (activeReplay == -1 || activeFrame > replayFiles[activeReplay].getCase(activeCase).getEndIndex() || 
        curGamestate.getHitThisFrame()[0] || curGamestate.getBlockThisFrame()[0] || curGamestate.getHitThisFrame()[1] || curGamestate.getBlockThisFrame()[1]) {
        for (std::size_t i = 0; i < replayFiles.size(); ++i) {
            for (std::size_t j = 0; j < replayFiles[i].getCaseBase().size(); ++j) {
                if (i != activeReplay || j != activeCase) {
                    auto bufComparison = comparisonFunction(curGamestate, replayFiles[i].getCase(j).getMetadata());
                    if (bufComparison <= bestComparison) {
                        bestComparison = bufComparison;
                        bestReplay = i;
                        bestCase = j;
                        bestFrame = replayFiles[i].getCase(j).getStartingIndex();
                    }
                }
            }
        }

        if (activeReplay != -1 && activeCase +1 <= replayFiles[activeReplay].getCaseBaseLength() -1 ){
            auto bufComparison = comparisonFunction(curGamestate, replayFiles[activeReplay].getCase(activeCase + 1).getMetadata());
            auto bufComparison2 = comparisonFunction(curGamestate, replayFiles[bestReplay].getCase(bestCase).getMetadata());
            debugPrint(replayFiles[activeReplay].getCase(activeCase + 1), replayFiles[bestReplay].getCase(bestCase), curGamestate, bufComparison, bufComparison2, activeCase + 1, bestCase, activeReplay, bestReplay);

            if (bufComparison <= (bestComparison * nextBestMulti + nextBestAdd)) {
                bestReplay = activeReplay;
                bestComparison = bufComparison;
                bestCase = activeCase+1;
                bestFrame = replayFiles[bestReplay].getCase(bestCase).getStartingIndex();
            }

        }

        activeReplay = bestReplay;
        activeCase = bestCase;
        activeFrame = bestFrame;
    }
    if (activeFrame == -1) { return 5; }
    auto input = replayFiles[activeReplay].getInput(activeFrame);


    if (replayFiles[activeReplay].getCase(activeCase).getMetadata().getFacing() != curGamestate.getFacing()) {
        input = inverseInput(input);
    }

    return input;
}

float CbrData::comparisonFunction(Metadata curGamestate, Metadata caseGamestate) {
    float compValue = 0;
    compValue += compRelativePosX(curGamestate.getPosX(), caseGamestate.getPosX()) * costXDist;
    compValue += compRelativePosY(curGamestate.getPosY(), caseGamestate.getPosY()) * costYDist;
    compValue += compDistanceToWall(curGamestate.getPosY(), caseGamestate.getPosY(), curGamestate.getFacing(), caseGamestate.getFacing()) * costWallDist;
    compValue += compState(curGamestate.getCurrentAction()[0], caseGamestate.getCurrentAction()[0]) *costAiState;
    compValue += compState(curGamestate.getCurrentAction()[1], caseGamestate.getCurrentAction()[1]) * costEnemyState;
    compValue += compNeutralState(curGamestate.getNeutral()[0], caseGamestate.getNeutral()[0]) * costAiNeutral;
    compValue += compNeutralState(curGamestate.getNeutral()[1], caseGamestate.getNeutral()[1]) * costEnemyNeutral;
    compValue += compAirborneState(curGamestate.getAir()[0], caseGamestate.getAir()[0]) * costAiAir;
    compValue += compAirborneState(curGamestate.getAir()[1], caseGamestate.getAir()[1]) * costEnemyAir;
    compValue += compWakeupState(curGamestate.getWakeup()[0], caseGamestate.getWakeup()[0]) * costAiWakeup;
    compValue += compWakeupState(curGamestate.getWakeup()[1], caseGamestate.getWakeup()[1]) * costEnemyWakeup;
    compValue += compBlockState(curGamestate.getBlocking()[0], caseGamestate.getBlocking()[0]) * costAiBlocking;
    compValue += compBlockState(curGamestate.getBlocking()[1], caseGamestate.getBlocking()[1]) * costEnemyBlocking;
    compValue += compGetHitThisFrameState(curGamestate.getHitThisFrame()[0], caseGamestate.getHitThisFrame()[0]) * costAiHitThisFrame;
    compValue += compGetHitThisFrameState(curGamestate.getHitThisFrame()[1], caseGamestate.getHitThisFrame()[1]) * costEnemyHitThisFrame;
    compValue += compBlockingThisFrameState(curGamestate.getBlockThisFrame()[0], caseGamestate.getBlockThisFrame()[0]) * costAiBlockThisFrame;
    compValue += compBlockingThisFrameState(curGamestate.getBlockThisFrame()[1], caseGamestate.getBlockThisFrame()[1]) * costEnemyBlockhisFrame;
    compValue += compCrouching(curGamestate.getCrouching()[0], caseGamestate.getCrouching()[0]) * costAiCrouching;
    compValue += compCrouching(curGamestate.getCrouching()[1], caseGamestate.getCrouching()[1]) * costEnemyCrouching;
    
    return compValue;

}

/*    std::array< int, 2>posX;
    std::array< int, 2>posY;
    bool facing;*/
float  CbrData::compRelativePosX(std::array< int, 2>curPosX, std::array< int, 2>casePosX) {
    auto dif1 = abs(curPosX[0] - curPosX[1]);
    auto dif2 = abs(casePosX[0] - casePosX[1]);
    auto dif3 = abs(dif1 - dif2);
    auto dif4 = fmin((float)dif3 / maxXDist, 1);
    return dif4;
}

float  CbrData::compRelativePosY(std::array< int, 2>curPosX, std::array< int, 2>casePosX) {
    auto dif1 = (curPosX[0] - curPosX[1]);
    auto dif2 = (casePosX[0] - casePosX[1]);
    auto dif3 = abs(dif1 - dif2);
    auto dif4 = fmin((float)dif3 / maxYDist, 1);
    
    return dif4;
}

float  CbrData::compDistanceToWall(std::array< int, 2>curPosX, std::array< int, 2>casePosX, bool curFacing, bool caseFacing) {
    int dif1;
    int dif2;
    if (curFacing) {
        dif1 = abs(curPosX[0] - lWallDist);
    }
    else {
        dif1 = abs(curPosX[0] - rWallDist);
    }
    if (caseFacing) {
        dif2 = abs(casePosX[0] - lWallDist);
    }
    else {
        dif2 = abs(casePosX[0] - rWallDist);
    }
    
    auto dif3 = abs(dif1 - dif2);
    auto dif4 = fmin((float)dif3 / maxWallDist, 1);
    return dif4;
}

float  CbrData::compState(std::string curS, std::string caseS) {
    if (curS != caseS) {
        return 1;
    }
    return 0;
}

float CbrData::compNeutralState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float CbrData::compAirborneState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float CbrData::compWakeupState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float CbrData::compBlockState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float CbrData::compGetHitThisFrameState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float CbrData::compBlockingThisFrameState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}

float CbrData::compCrouching(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}


int CbrData::inverseInput(int input) {
    auto buffer = input;
    auto test = buffer - specialButton;
    if (test > 0) { buffer = test; }
    test = buffer - tauntButton;
    if (test > 0) { buffer = test; }
    test = buffer - DButton;
    if (test > 0) { buffer = test; }
    test = buffer - CButton;
    if (test > 0) { buffer = test; }
    test = buffer - BButton;
    if (test > 0) { buffer = test; }
    test = buffer - AButton;
    if (test > 0) { buffer = test; }

    if (buffer == 6 || buffer == 3 || buffer == 9) {
        return input - 2;
    }

    if (buffer == 4 || buffer == 7 || buffer == 1) {
        return input + 2;
    }
    return input;
}


void CbrData::debugPrint(CbrCase nextC, CbrCase bestC, Metadata curM, float nextCost, float bestCost, int nextCI, int bestCI, int nextRI, int bestRI) {
    std::string str = "";

    str += curM.PrintState();

    str += "\nNext Cost: " + std::to_string(nextCost);
    str +=  "\nReplay: " + std::to_string(nextRI)+ " - Case: " + std::to_string(nextCI) + " - Start: " + std::to_string(nextC.getStartingIndex()) + " - End: " + std::to_string(nextC.getEndIndex());
    str += "\n" + nextC.getMetadata().PrintState();

    str += "\nBest Cost: " + std::to_string(bestCost);
    str += "\nReplay: " + std::to_string(bestRI) + " - Case: " + std::to_string(bestCI) + " - Start: " + std::to_string(bestC.getStartingIndex()) + " - End: " + std::to_string(bestC.getEndIndex());
    str += "\n" + bestC.getMetadata().PrintState();

    debugText = str;

}