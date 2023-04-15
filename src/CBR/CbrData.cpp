#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <unordered_map>
#include "CbrData.h"
#include "Metadata.h"
#include <utility>  // std::forward
#include <sstream>
#include <random>
#include "CbrUtils.h"


#define rWallDist 1850000
#define lWallDist -1850000
#define maxWallDist 3700000
//(1850000/2)
#define maxXDist 462500
#define maxXDistScreen 925000
//(800000/2)
#define maxYDist 400000 

#define maxProration 10000
#define maxComboTime 200

#define minCaseCooldown 30
#define maxCaseCooldown 600
double preComputedCooldownMulti = double(1) / double(maxCaseCooldown - minCaseCooldown);

#define minBlockstun 0
#define maxBlockstun 15
double preComputedBlockstunMulti = double(1) / double(maxBlockstun - minBlockstun);
#define maxHpDiff 13000




#define rachelWindMax 40000
#define rachelWindMin 10000

#define nextBestMulti 1.3
#define nextBestAdd 0.05
#define nextBestAddInputSequence 0.2

#define maxRandomDiff 0.1

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
/*
float  compRelativePosX(std::array< int, 2>curPosX, std::array< int, 2>casePosX) {
    auto dif1 = abs(curPosX[0] - curPosX[1]);
    auto dif2 = abs(casePosX[0] - casePosX[1]);
    auto dif3 = abs(dif1 - dif2);
    auto dif4 = fmin((float)dif3 / maxXDist, 1);
    return dif4;

    float  compRelativePosY(std::array< int, 2>curPosX, std::array< int, 2>casePosX) {
    auto dif1 = (curPosX[0] - curPosX[1]);
    auto dif2 = (casePosX[0] - casePosX[1]);
    auto dif3 = abs(dif1 - dif2);
    auto dif4 = fmin((float)dif3 / maxYDist, 1);
    return dif4;
}
}*/
float RandomFloatC(float a, float b) {
    float random = ((float)rand()) / (float)RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}
int RandomInt(float a, float b) {
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(a, b); // define the range
    return distr(gen);
}


float CalcNormalizedDifference(int cur, int cas, int min, float precomputedMulti) {
    return (abs(cur - cas) - min) * precomputedMulti;
}

float compAirborneState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}

float  compRelativePosX(int curP1, int curP2, int caseP1, int caseP2) {
    auto dif1 = abs(curP1 - curP2);
    auto dif2 = abs(caseP1 - caseP2);
    auto dif3 = abs(dif1 - dif2);
    auto dif4 = (float)dif3 / maxXDist;
    //auto dif4 = fmin((float)dif3 / maxXDist, 1);
    if (dif4 > 1) {
        return 1;
    }
    return dif4;
}

float  compMaxDistanceAttack(int curP1, int curP2, int caseP1, int caseP2, int maxX) {
    if (maxX == -1) { return 0;}
    auto dif1 = abs(curP1 - curP2);
    auto dif2 = abs(caseP1 - caseP2);
    auto dif3 = dif1 - dif2 -(maxX);
    if (dif3 > 0) { 
        return 1; 
    }
    return 0;
}

float  compRelativePosY(int curP1, int curP2, int caseP1, int caseP2) {
    auto dif1 = abs(curP1 - curP2);
    auto dif2 = abs(caseP1 - caseP2);
    auto dif3 = abs(dif1 - dif2);
    //auto dif4 = fmin((float)dif3 / maxYDist, 1);
    auto dif4 = (float)dif3 / maxYDist;
    if (dif4 > 1) {
        return 1;
    }
    return dif4;
}
float compHelperOrder(int curPosOpponent, int curPosChar, int curPosHelper, int casePosOpponent, int casePosChar, int casePosHelper) {
    bool curInFront;
    bool caseInFront;
    auto curFacing = curPosChar >= curPosOpponent;
    if (curFacing) {
        curInFront = curPosChar > curPosHelper;
    }
    else {
        curInFront = curPosChar <= curPosHelper;
    }
    auto caseFacing = casePosChar >= casePosOpponent;
    if (caseFacing) {
        caseInFront = casePosChar > casePosHelper;
    }
    else {
        caseInFront = casePosChar <= casePosHelper;
    }

    return curInFront != caseInFront;

}

float  compDistanceToWall(int curPosXP1, int casePosXP1, bool curFacing, bool caseFacing) {
    int dif1;
    int dif2;
    if (curFacing) {
        dif1 = abs(curPosXP1 - lWallDist);
    }
    else {
        dif1 = abs(curPosXP1 - rWallDist);
    }
    if (caseFacing) {
        dif2 = abs(casePosXP1 - lWallDist);
    }
    else {
        dif2 = abs(casePosXP1 - rWallDist);
    }

    auto dif3 = abs(dif1 - dif2);
    //auto dif4 = fmin((float)dif3 / maxWallDist, 1);
    auto dif4 = (float)dif3 / maxWallDist;
    if (dif4 > 1) {
        return 1;
    }
    return dif4;
}

float  compState(std::string curS, std::string caseS) {
    if (curS != caseS) {
        return 1;
    }
    return 0;
}

float  compStateHash(size_t curS, size_t caseS) {
    if (curS != caseS) {
        return 1;
    }
    return 0;
}

float compInt(int cur, int cas, int max) {
    auto dif = abs(cur - cas);
    //float dif2 = fmin(((float)dif / max), 1);
    auto dif2 = (float)dif / max;
    if (dif2 > 1) {
        return 1;
    }
    return dif2;
}

float compIntState(int cur, int cas) {
    return cur != cas;
}

float compBool(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}

float compNeutralState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float compWakeupState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float compBlockState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float compBlockStun(bool cur, bool cas, int curStun, int casStun) {
    float ret = 0;
    if (cur != cas) { ret += 0.1; }
    ret += CalcNormalizedDifference(curStun, casStun, minBlockstun, preComputedBlockstunMulti);
    return ret;
}
float compHitState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float compGetHitThisFrameState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
float compBlockingThisFrameState(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}

float compCrouching(bool cur, bool cas) {
    if (cur != cas) { return 1; }
    return 0;
}
//Add a cost for reusing a case within a certain timeframe.
float compCaseCooldown(CbrCase* caseData, int framesActive) {
    auto diff = framesActive - caseData->caseCooldownFrameStart;
    if (caseData->caseCooldownFrameStart == -1 || caseData->caseCooldownFrameStart > framesActive) {
        caseData->caseCooldownFrameStart = -1;
        return 0; 
    }

    if (diff >= minCaseCooldown) {
        if (diff >= maxCaseCooldown) {
            caseData->caseCooldownFrameStart = -1;
            return 0;
        }
        //Return a cost that gets lesser the longer ago the case was replayed.
        return 1 - (diff - minCaseCooldown) * preComputedCooldownMulti;
        
    }
    //Return 2 if a  case tries to repeat itself in less than 30 frames to prevent buggy looking movement.
    return 2;
}

CbrData::CbrData()
{
    initalizeCosts();
}

CbrData::CbrData(std::string n, std::string p1, int charId) :playerName(n), characterName(p1)
{
    characterIndex = charId;
    replayFiles = {};
    activeReplay = -1;
    activeCase = -1;
    activeFrame = -1;
    enabled = true;
    framesActive = 0;
    initalizeCosts();
}
#define costXDist 0//1
#define costYDist 1//1
#define costWallDist 2//0.3
#define costWallDistCombo 2//0.5
#define costAiState 3//0.5
#define costEnemyState 4//0.15
#define costlastAiState 5//0.05
#define costlastEnemyState 6//0.05
#define costAiNeutral 7//1
#define costEnemyNeutral 8//0.2
#define costAiAir 9//1
#define costEnemyAir 10//0.2
#define costAiWakeup 11//1
#define costEnemyWakeup 12//0.2
#define costAiBlocking 13//1
#define costEnemyBlocking 14//1
#define costAiHit 15//1
#define costEnemyHit 16//1
#define costAiAttack 17//1
#define costEnemyAttack 18//0.2
#define costAiHitThisFrame 19//0.01
#define costEnemyHitThisFrame 20//0.01
#define costAiBlockThisFrame 21//0.1
#define costEnemyBlockhisFrame 22//0.1
#define costAiCrouching 23//0.1
#define costEnemyCrouching 24//0.1
#define costAiProration 25//0.05
#define costEnemyProration 26//0.05
#define costAiStarterRating 27//0.05
#define costEnemyStarterRating 28//0.05
#define costAiComboTime 29//0.05
#define costEnemyComboTime 30//0.05
#define costAiOverdriveState 31//1
#define costEnemyOverdriveState 32//0.01
#define costMatchState 33//100
#define costCaseCooldown 34//2
#define costAiHp 35//0.01
#define costEnemyHp 36//0.03

//projectilecosts
#define costHelperType 37//0.2
#define costHelperPosX 38//0.1
#define costHelperPosY 39//0.1
#define costHelperState 40//0.05
#define costHelperHit 41//0.05
#define costHelperAttack 42//0.1
#define costHelperOrder 43//0.05
#define costHelperSum 44//(costHelperType + costHelperPosX + costHelperPosY + costHelperState + costHelperHit + costHelperAttack + costHelperOrder + costHelperOrder)

//bulletSpecificData
#define costBuHeat 45//0.1

//litchiSpecific
#define costLcStaff 46//1
#define costLcStaffActive 47//0.4

//rachelSpecific
#define costWind 48//0.1
#define costLowWind 49//1

//azraelSpecific
#define costAzTopWeak 50//0.1
#define costAzBotWeak 51//1
#define costAzFireball 52//0.1
#define costAzTopWeakEnemy 53//0.1
#define costAzBotWeakEnemy 54//1
#define costAzFireballEnemy 55//0.1

//tagerSpecific
#define costTagerSpark 56//0.1
#define costTagerMagnetism 57//1
#define costTagerSparkEnemy 58//0.1
#define costTagerMagnetismEnemy 59//1

//ESSpecific
#define costEsBuff 60//0.1
#define costEsBuffEnemy 61//1

//ENu/Lambdapecific
#define costNuGravity 62//0.1
#define costNuGravityEnemy  63//1

//HazSpecific
#define costHazChains 64//0.1
#define costHazChainsEnemy  65//1

//Celica
#define costCelAnyblueLife 66//0.1
#define costCelBlueAmount  67//1
#define costCelAnyblueLifeEnemy 68//0.1

//Valk
#define costValWolfMeter 69//0.1
#define costValWolfState  70//1
#define costValWolfMeterEnemy 71//0.1
#define costValWolfStateEnemy  72//1

//Plat
#define costPlatCurItem 73//0.1
#define costPlatNextItem  74//1
#define costPlatItemNr 75//0.1
#define costPlatCurItemEnemy 76//0.1

//Relius
#define costReliusDollMeter 77//0.1
#define costReliusDollState 78//1
#define costReliusDollInactive 79//0.1
#define costReliusDollCooldown 80//0.1
#define costReliusDollMeterEnemy 81//0.1
#define costReliusDollStateEnemy  82//1
#define costReliusDollInactiveEnemy  83//0.1
#define costReliusDollCooldownEnemy  84//0.1

//Susan
#define costSusanUnlock1 85//0.1
#define costSusanUnlock1Lvl 86//0.1
#define costSusanUnlock2 87//0.1
#define costSusanUnlock2Lvl 88//0.1
#define costSusanUnlock3 89//0.1
#define costSusanUnlock3Lvl 90//0.1
#define costSusanUnlock4 91//0.1
#define costSusanUnlock4Lvl 92//0.1
#define costSusanUnlock5 93//0.1
#define costSusanUnlock5Lvl 94//0.1
#define costSusanUnlock6 95//0.1
#define costSusanUnlock6Lvl 96//0.1
#define costSusanUnlock7 97//0.1
#define costSusanUnlock7Lvl 98//0.1
#define costSusanUnlock8 99//0.1
#define costSusanUnlock8Lvl 100//0.1
#define costSusanUnlock9 101//0.1

#define costSusanUnlock1Enemy 102//0.1
#define costSusanUnlock1LvlEnemy 103//0.1
#define costSusanUnlock2Enemy 104//0.1
#define costSusanUnlock2LvlEnemy 105//0.1
#define costSusanUnlock3Enemy 106//0.1
#define costSusanUnlock3LvlEnemy 107//0.1
#define costSusanUnlock4Enemy 108//0.1
#define costSusanUnlock4LvlEnemy 109//0.1
#define costSusanUnlock5Enemy 110//0.1
#define costSusanUnlock5LvlEnemy 111//0.1
#define costSusanUnlock6Enemy 112//0.1
#define costSusanUnlock6LvlEnemy 113//0.1
#define costSusanUnlock7Enemy 114//0.1
#define costSusanUnlock7LvlEnemy 115//0.1
#define costSusanUnlock8Enemy 116//0.1
#define costSusanUnlock8LvlEnemy 117//0.1
#define costSusanUnlock9Enemy 118//0.1

//Jubei
#define costJubeiBuff 119//0.1
#define costJubeiMark 120//0.1
#define costJubeiBuffEnemy 121//0.1
#define costJubeiMarkEnemy 122//0.1

//Izanami
#define costIzanamiFloating 123//0.1
#define costIzanamiRibcage 124//0.1
#define costIzanamiShotCooldown 125//0.1
#define costIzanamiBitsStance 126//0.1

#define costIzanamiFloatingEnemy 127//0.1
#define costIzanamiRibcageEnemy 128//0.1
#define costIzanamiShotCooldownEnemy 129//0.1
#define costIzanamiBitsStanceEnemy 130//0.1

//Nine
#define costNineSpell 131//0.1
#define costNineSpellBackup 132//0.1
#define costNineSlots 133//0.1
#define costNineSlotsBackup 134//0.1

#define costNineSpellEnemy 135//0.1
#define costNineSpellBackupEnemy 136//0.1
#define costNineSlotsEnemy 137//0.1
#define costNineSlotsBackupEnemy 138//0.1

//Carl
#define costCarlDollInactive 139//0.1
#define costCarlDollMeter 140//0.1
#define costCarlDollInactiveEnemy 141//0.1
#define costCarlDollMeterEnemy 142//0.1

//Tsubaki
#define costTsubakiMeter 143//0.1
#define costTsubakiMeterEnemy 144//0.1

//Kokonoe
#define costKokoGravitronNr 145//0.1
#define costKokoAnyGraviLeft 146//0.3
#define costKokoGravitronNrEnemy 147//0.1
#define costKokoAnyGraviLeftEnemy 148//0.3

//Izayoi
#define costIzayoiState 149//0.3
#define costIzayoiStocks 150//0.8
#define costIzayoiSupermode 151//0.4
#define costIzayoiStateEnemy 152//0.3
#define costIzayoiStocksEnemy 153//0.8
#define costIzayoiSupermodeEnemy 154//0.4

//Arakune
#define costArakuneCurseMeter 155//0.1
#define costArakuneCurseActive 156//0.8
#define costArakuneCurseMeterEnemy 157//0.1
#define costArakuneCurseActiveEnemy 158//0.8

//Bang
#define costBangFRKZ1 159//0.05
#define costBangFRKZ2 160//0.05
#define costBangFRKZ3 161//0.05
#define costBangFRKZ4 162//0.05
#define costBangNailcount 163//0.4

#define costBangFRKZ1Enemy 164//0.05
#define costBangFRKZ2Enemy 165//0.05
#define costBangFRKZ3Enemy 166//0.05
#define costBangFRKZ4Enemy 167//0.05
#define costBangNailcountEnemy 168//0.4

//Amane
#define costAmaneDrillMeter 179//0.04
#define costAmaneDrillOverheat 180//0.2
#define costAmaneDrillMeterEnemy 181//0.4
#define costAmaneDrillOverheatEnemy 182//0.2

//Generic
#define costMinDistanceAttack 183//0.5

#define costNonNeutralState 184//0.3

void CbrData::initalizeCosts() {
    
    costs.name[costXDist] = "costXDist";
	costs.basic[costXDist] = 1;
    costs.combo[costXDist] = 1;
    costs.pressure[costXDist] = 1;
    costs.blocking[costXDist] = 1;

    costs.name[costYDist] = "costYDist";
	costs.basic[costYDist] = 1;
    costs.combo[costYDist] = 1;
    costs.pressure[costYDist] = 1;
    costs.blocking[costXDist] = 1;

    costs.name[costWallDist] = "costWallDist";
	costs.basic[costWallDist] = 0.3;
    costs.combo[costWallDist] = 0.6;
    costs.pressure[costWallDist] = 0.4;
    costs.blocking[costWallDist] = 0.4;

    costs.name[costAiState] = "costAiState";
	costs.basic[costAiState] = 0.1;
    costs.combo[costAiState] = 1;
    costs.pressure[costAiState] = 1;
    costs.blocking[costAiState] = 0.4;

    costs.name[costNonNeutralState] = "costNonNeutralState";
    costs.basic[costNonNeutralState] = 0.3;
    costs.combo[costNonNeutralState] = 0.4;
    costs.pressure[costNonNeutralState] = 0.4;
    costs.blocking[costNonNeutralState] = 0.0;
    

    costs.name[costEnemyState] = "costEnemyState";
	costs.basic[costEnemyState] = 0.05;
    costs.combo[costEnemyState] = 0.15;
    costs.pressure[costEnemyState] = 0.15;
    costs.blocking[costEnemyState] = 1;

    costs.name[costlastAiState] = "costlastAiState";
	costs.basic[costlastAiState] = 0.01;
    costs.combo[costlastAiState] = 0.1;
    costs.pressure[costlastAiState] = 0.15;
    costs.blocking[costlastAiState] = 0.05;

    costs.name[costlastEnemyState] = "costlastEnemyState";
	costs.basic[costlastEnemyState] = 0.01;
    costs.combo[costlastEnemyState] = 0.01;
    costs.pressure[costlastEnemyState] = 0.05;
    costs.blocking[costlastEnemyState] = 0.15;

    costs.name[costAiNeutral] = "costAiNeutral";
	costs.basic[costAiNeutral] = 1;
    costs.combo[costAiNeutral] = 1;
    costs.pressure[costAiNeutral] = 1;
    costs.blocking[costAiNeutral] = 1;

    costs.name[costEnemyNeutral] = "costEnemyNeutral";
	costs.basic[costEnemyNeutral] = 0.1;
    costs.combo[costEnemyNeutral] = 0.1;
    costs.pressure[costEnemyNeutral] = 0.1;
    costs.blocking[costEnemyNeutral] = 0.1;

    costs.name[costAiAir] = "costAiAir";
	costs.basic[costAiAir] = 0.5;
    costs.combo[costAiAir] = 0.5;
    costs.pressure[costAiAir] = 0.5;
    costs.blocking[costAiAir] = 0.5;

    costs.name[costEnemyAir] = "costEnemyAir";
	costs.basic[costEnemyAir] = 0.8;
    costs.combo[costEnemyAir] = 0.8;
    costs.pressure[costEnemyAir] = 0.8;
    costs.blocking[costEnemyAir] = 0.8;

    costs.name[costAiWakeup] = "costAiWakeup";
	costs.basic[costAiWakeup] = 1;
    costs.combo[costAiWakeup] = 1;
    costs.pressure[costAiWakeup] = 1;
    costs.blocking[costAiWakeup] = 1;

    costs.name[costEnemyWakeup] = "costEnemyWakeup";
	costs.basic[costEnemyWakeup] = 1;
    costs.combo[costEnemyWakeup] = 1;
    costs.pressure[costEnemyWakeup] = 1;
    costs.blocking[costEnemyWakeup] = 1;

    costs.name[costAiBlocking] = "costAiBlocking";
	costs.basic[costAiBlocking] = 1;
    costs.combo[costAiBlocking] = 1;
    costs.pressure[costAiBlocking] = 1;
    costs.blocking[costAiBlocking] = 1;
    
    costs.name[costEnemyBlocking] = "costEnemyBlocking";
	costs.basic[costEnemyBlocking] = 0.2;
    costs.combo[costEnemyBlocking] = 0.2;
    costs.pressure[costEnemyBlocking] = 0.2;
    costs.blocking[costEnemyBlocking] = 0.2;

    costs.name[costAiHit] = "costAiHit";
	costs.basic[costAiHit] = 1;
    costs.combo[costAiHit] = 1;
    costs.pressure[costAiHit] = 1;
    costs.blocking[costAiHit] = 1;

    costs.name[costEnemyHit] = "costEnemyHit";
	costs.basic[costEnemyHit] = 1;
    costs.combo[costEnemyHit] = 1;
    costs.pressure[costEnemyHit] = 1;
    costs.blocking[costEnemyHit] = 1;

    costs.name[costAiAttack] = "costAiAttack";
	costs.basic[costAiAttack] = 1;
    costs.combo[costAiAttack] = 1;
    costs.pressure[costAiAttack] = 1;
    costs.blocking[costAiAttack] = 1;

    costs.name[costEnemyAttack] = "costEnemyAttack";
	costs.basic[costEnemyAttack] = 0.2;
    costs.combo[costEnemyAttack] = 0.2;
    costs.pressure[costEnemyAttack] = 0.2;
    costs.blocking[costEnemyAttack] = 0.4;

    costs.name[costAiHitThisFrame] = "costAiHitThisFrame";
	costs.basic[costAiHitThisFrame] = 0.1;
    costs.combo[costAiHitThisFrame] = 0.1;
    costs.pressure[costAiHitThisFrame] = 0.1;
    costs.blocking[costAiHitThisFrame] = 0.1;

    costs.name[costEnemyHitThisFrame] = "costEnemyHitThisFrame";
	costs.basic[costEnemyHitThisFrame] = 0.1;
    costs.combo[costEnemyHitThisFrame] = 0.1;
    costs.pressure[costEnemyHitThisFrame] = 0.1;
    costs.blocking[costEnemyHitThisFrame] = 0.1;

    costs.name[costAiBlockThisFrame] = "costAiBlockThisFrame";
	costs.basic[costAiBlockThisFrame] = 0.1;
    costs.combo[costAiBlockThisFrame] = 0.1;
    costs.pressure[costAiBlockThisFrame] = 0.1;
    costs.blocking[costAiBlockThisFrame] = 0.1;

    costs.name[costEnemyBlockhisFrame] = "costEnemyBlockhisFrame";
	costs.basic[costEnemyBlockhisFrame] = 0.1;
    costs.combo[costEnemyBlockhisFrame] = 0.1;
    costs.pressure[costEnemyBlockhisFrame] = 0.1;
    costs.blocking[costEnemyBlockhisFrame] = 0.1;

    costs.name[costAiCrouching] = "costAiCrouching";
	costs.basic[costAiCrouching] = 0.1;
    costs.combo[costAiCrouching] = 0.1;
    costs.pressure[costAiCrouching] = 0.1;
    costs.blocking[costAiCrouching] = 0.1;

    costs.name[costEnemyCrouching] = "costEnemyCrouching";
	costs.basic[costEnemyCrouching] = 0.1;
    costs.combo[costEnemyCrouching] = 0.5;
    costs.pressure[costEnemyCrouching] = 0.5;
    costs.blocking[costEnemyCrouching] = 0.1;

    costs.name[costAiProration] = "costAiProration";
	costs.basic[costAiProration] = 0.1;
    costs.combo[costAiProration] = 0.1;
    costs.pressure[costAiProration] = 0.1;
    costs.blocking[costAiProration] = 0.1;

    costs.name[costEnemyProration] = "costEnemyProration";
	costs.basic[costEnemyProration] = 0.1;
    costs.combo[costEnemyProration] = 0.1;
    costs.pressure[costEnemyProration] = 0.1;
    costs.blocking[costEnemyProration] = 0.1;

    costs.name[costAiStarterRating] = "costAiStarterRating";
	costs.basic[costAiStarterRating] = 0.1;
    costs.combo[costAiStarterRating] = 0.1;
    costs.pressure[costAiStarterRating] = 0.1;
    costs.blocking[costAiStarterRating] = 0.1;

    costs.name[costEnemyStarterRating] = "costEnemyStarterRating";
	costs.basic[costEnemyStarterRating] = 0.1;
    costs.combo[costEnemyStarterRating] = 0.1;
    costs.pressure[costEnemyStarterRating] = 0.1;
    costs.blocking[costEnemyStarterRating] = 0.1;

    costs.name[costAiComboTime] = "costAiComboTime";
	costs.basic[costAiComboTime] = 0.1;
    costs.combo[costAiComboTime] = 0.1;
    costs.pressure[costAiComboTime] = 0.1;
    costs.blocking[costAiComboTime] = 0.1;

    costs.name[costEnemyComboTime] = "costEnemyComboTime";
	costs.basic[costEnemyComboTime] = 0.1;
    costs.combo[costEnemyComboTime] = 0.1;
    costs.pressure[costEnemyComboTime] = 0.1;
    costs.blocking[costEnemyComboTime] = 0.1;

    costs.name[costAiOverdriveState] = "costAiOverdriveState";
	costs.basic[costAiOverdriveState] = 1;
    costs.combo[costAiOverdriveState] = 1;
    costs.pressure[costAiOverdriveState] = 0.3;
    costs.blocking[costAiOverdriveState] = 0.5;

    costs.name[costEnemyOverdriveState] = "costEnemyOverdriveState";
	costs.basic[costEnemyOverdriveState] = 0.5;
    costs.combo[costEnemyOverdriveState] = 0.1;
    costs.pressure[costEnemyOverdriveState] = 0.5;
    costs.blocking[costEnemyOverdriveState] = 0.5;

    costs.name[costMatchState] = "costMatchState";
	costs.basic[costMatchState] = 100;
    costs.combo[costMatchState] = 100;
    costs.pressure[costMatchState] = 100;
    costs.blocking[costMatchState] = 100;

    costs.name[costCaseCooldown] = "costCaseCooldown";
	costs.basic[costCaseCooldown] = 1;
    costs.combo[costCaseCooldown] = 0.1;
    costs.pressure[costCaseCooldown] = 0.1;
    costs.blocking[costCaseCooldown] = 0.1;

    costs.name[costAiHp] = "costAiHp";
	costs.basic[costAiHp] = 0.1;
    costs.combo[costAiHp] = 0.1;
    costs.pressure[costAiHp] = 0.1;
    costs.blocking[costAiHp] = 0.1;

    costs.name[costEnemyHp] = "costEnemyHp";
	costs.basic[costEnemyHp] = 0.1;
    costs.combo[costEnemyHp] = 0.2;
    costs.pressure[costEnemyHp] = 0.2;
    costs.blocking[costEnemyHp] = 0.1;

    costs.name[costHelperType] = "costHelperType";
	costs.basic[costHelperType] = 0.2;
    costs.combo[costHelperType] = 0.2;
    costs.pressure[costHelperType] = 0.2;
    costs.blocking[costHelperType] = 0.2;
    
    costs.name[costHelperPosX] = "costHelperPosX";
	costs.basic[costHelperPosX] = 0.1;
    costs.combo[costHelperPosX] = 0.1;
    costs.pressure[costHelperPosX] = 0.1;
    costs.blocking[costHelperPosX] = 0.1;

    costs.name[costHelperPosY] = "costHelperPosY";
	costs.basic[costHelperPosY] = 0.1;
    costs.combo[costHelperPosY] = 0.1;
    costs.pressure[costHelperPosY] = 0.1;
    costs.blocking[costHelperPosY] = 0.1;

    costs.name[costHelperState] = "costHelperState";
	costs.basic[costHelperState] = 0.05;
    costs.combo[costHelperState] = 0.05;
    costs.pressure[costHelperState] = 0.05;
    costs.blocking[costHelperState] = 0.05;

    costs.name[costHelperHit] = "costHelperHit";
	costs.basic[costHelperHit] = 0.05;
    costs.combo[costHelperHit] = 0.05;
    costs.pressure[costHelperHit] = 0.05;
    costs.blocking[costHelperHit] = 0.05;

    costs.name[costHelperAttack] = "costHelperAttack";
	costs.basic[costHelperAttack] = 0.1;
    costs.combo[costHelperAttack] = 0.1;
    costs.pressure[costHelperAttack] = 0.1;
    costs.blocking[costHelperAttack] = 0.1;

    costs.name[costHelperOrder] = "costHelperOrder";
	costs.basic[costHelperOrder] = 0.05;
    costs.combo[costHelperOrder] = 0.05;
    costs.pressure[costHelperOrder] = 0.05;
    costs.blocking[costHelperOrder] = 0.05;

    costs.name[costHelperSum] = "-1";
	costs.basic[costHelperSum] = costs.basic[costHelperType] + costs.basic[costHelperPosX] + costs.basic[costHelperPosY] + costs.basic[costHelperState] + costs.basic[costHelperHit] + costs.basic[costHelperAttack] + costs.basic[costHelperOrder];
    costs.combo[costHelperSum] = costs.combo[costHelperType] + costs.combo[costHelperPosX] + costs.combo[costHelperPosY] + costs.combo[costHelperState] + costs.combo[costHelperHit] + costs.combo[costHelperAttack] + costs.combo[costHelperOrder];
    costs.pressure[costHelperSum] = costs.pressure[costHelperType] + costs.pressure[costHelperPosX] + costs.pressure[costHelperPosY] + costs.pressure[costHelperState] + costs.pressure[costHelperHit] + costs.pressure[costHelperAttack] + costs.pressure[costHelperOrder];
    costs.blocking[costHelperSum] = costs.blocking[costHelperType] + costs.blocking[costHelperPosX] + costs.blocking[costHelperPosY] + costs.blocking[costHelperState] + costs.blocking[costHelperHit] + costs.blocking[costHelperAttack] + costs.blocking[costHelperOrder];

    //bulletSpecificData
    costs.name[costBuHeat] = "costBuHeat";
	costs.basic[costBuHeat] = 0.1;
    costs.combo[costBuHeat] = 0.3;
    costs.pressure[costBuHeat] = 0.1;
    costs.blocking[costBuHeat] = 0.1;

    //litchiSpecific
    costs.name[costLcStaff] = "costLcStaff";
	costs.basic[costLcStaff] = 1;
    costs.combo[costLcStaff] = 1;
    costs.pressure[costLcStaff] = 1;
    costs.blocking[costLcStaff] = 1;

    costs.name[costLcStaffActive] = "costLcStaffActive";
	costs.basic[costLcStaffActive] = 0.4;
    costs.combo[costLcStaffActive] = 0.4;
    costs.pressure[costLcStaffActive] = 0.4;
    costs.blocking[costLcStaffActive] = 0.4;

    //rachelSpecific
    costs.name[costWind] = "costWind";
	costs.basic[costWind] = 0.1;
    costs.combo[costWind] = 0.1;
    costs.pressure[costWind] = 0.1;
    costs.blocking[costWind] = 0.1;

    costs.name[costLowWind] = "costLowWind";
	costs.basic[costLowWind] = 1;
    costs.combo[costLowWind] = 1;
    costs.pressure[costLowWind] = 1;
    costs.blocking[costLowWind] = 1;

    //azraelSpecific
    costs.name[costAzTopWeak] = "costAzTopWeak";
	costs.basic[costAzTopWeak] = 0.1;
    costs.combo[costAzTopWeak] = 0.4;
    costs.pressure[costAzTopWeak] = 0.2;
    costs.blocking[costAzTopWeak] = 0.01;

    costs.name[costAzBotWeak] = "costAzBotWeak";
	costs.basic[costAzBotWeak] = 0.1;
    costs.combo[costAzBotWeak] = 0.4;
    costs.pressure[costAzBotWeak] = 0.2;
    costs.blocking[costAzBotWeak] = 0.01;

    costs.name[costAzFireball] = "costAzFireball";
	costs.basic[costAzFireball] = 0.1;
    costs.combo[costAzFireball] = 0.1;
    costs.pressure[costAzFireball] = 0.1;
    costs.blocking[costAzFireball] = 0.01;

    costs.name[costAzTopWeakEnemy] = "costAzTopWeakEnemy";
	costs.basic[costAzTopWeakEnemy] = 0.01;
    costs.combo[costAzTopWeakEnemy] = 0.01;
    costs.pressure[costAzTopWeakEnemy] = 0.1;
    costs.blocking[costAzTopWeakEnemy] = 0.01;

    costs.name[costAzBotWeakEnemy] = "costAzBotWeakEnemy";
	costs.basic[costAzBotWeakEnemy] = 0.01;
    costs.combo[costAzBotWeakEnemy] = 0.01;
    costs.pressure[costAzBotWeakEnemy] = 0.1;
    costs.blocking[costAzBotWeakEnemy] = 0.01;

    costs.name[costAzFireballEnemy] = "costAzFireballEnemy";
	costs.basic[costAzFireballEnemy] = 0.1;
    costs.combo[costAzFireballEnemy] = 0.01;
    costs.pressure[costAzFireballEnemy] = 0.1;
    costs.blocking[costAzFireballEnemy] = 0.01;
    
    //tagerSpecific
    costs.name[costTagerSpark] = "costTagerSpark";
	costs.basic[costTagerSpark] = 0.3;
    costs.combo[costTagerSpark] = 0.3;
    costs.pressure[costTagerSpark] = 0.1;
    costs.blocking[costTagerSpark] = 0.01;

    costs.name[costTagerMagnetism] = "costTagerMagnetism";
	costs.basic[costTagerMagnetism] = 0.3;
    costs.combo[costTagerMagnetism] = 0.4;
    costs.pressure[costTagerMagnetism] = 0.4;
    costs.blocking[costTagerMagnetism] = 0.1;

    costs.name[costTagerSparkEnemy] = "costTagerSparkEnemy";
	costs.basic[costTagerSparkEnemy] = 0.3;
    costs.combo[costTagerSparkEnemy] = 0;
    costs.pressure[costTagerSparkEnemy] = 0.01;
    costs.blocking[costTagerSparkEnemy] = 0.1;

    costs.name[costTagerMagnetismEnemy] = "costTagerMagnetismEnemy";
	costs.basic[costTagerMagnetismEnemy] = 0.3;
    costs.combo[costTagerMagnetismEnemy] = 0;
    costs.pressure[costTagerMagnetismEnemy] = 0.4;
    costs.blocking[costTagerMagnetismEnemy] = 0.1;

    //Es
    costs.name[costEsBuff] = "costEsBuff";
	costs.basic[costEsBuff] = 0.1;
    costs.combo[costEsBuff] = 0.3;
    costs.pressure[costEsBuff] = 0.3;
    costs.blocking[costEsBuff] = 0.1;

    costs.name[costEsBuffEnemy] = "costEsBuffEnemy";
	costs.basic[costEsBuffEnemy] = 0.3;//TODO: not implemented yet add later
    costs.combo[costEsBuffEnemy] = 0;
    costs.pressure[costEsBuffEnemy] = 0.2;
    costs.blocking[costEsBuffEnemy] = 0.1;

    //Nu/Lambda
    costs.name[costNuGravity] = "costNuGravity";
	costs.basic[costNuGravity] = 0.1;
    costs.combo[costNuGravity] = 0.1;
    costs.pressure[costNuGravity] = 0.1;
    costs.blocking[costNuGravity] = 0.1;

    costs.name[costNuGravityEnemy] = "costNuGravityEnemy";
	costs.basic[costNuGravityEnemy] = 0.1;//TODO: not implemented yet add later
    costs.combo[costNuGravityEnemy] = 0.1;
    costs.pressure[costNuGravityEnemy] = 0.1;
    costs.blocking[costNuGravityEnemy] = 0.1;

    //Haz
    costs.name[costHazChains] = "costHazChains";
	costs.basic[costHazChains] = 0.1;
    costs.combo[costHazChains] = 0.1;
    costs.pressure[costHazChains] = 0.1;
    costs.blocking[costHazChains] = 0.1;

    costs.name[costHazChainsEnemy] = "costHazChainsEnemy";
	costs.basic[costHazChainsEnemy] = 0.1;//TODO: not implemented yet add later
    costs.combo[costHazChainsEnemy] = 0.0;
    costs.pressure[costHazChainsEnemy] = 0.0;
    costs.blocking[costHazChainsEnemy] = 0.1;

    //Celica
    costs.name[costCelAnyblueLife] = "costCelAnyblueLife";
	costs.basic[costCelAnyblueLife] = 0.2;
    costs.combo[costCelAnyblueLife] = 0.2;
    costs.pressure[costCelAnyblueLife] = 0.2;
    costs.blocking[costCelAnyblueLife] = 0.05;

    costs.name[costCelBlueAmount] = "costCelBlueAmount";
	costs.basic[costCelBlueAmount] = 0.1;
    costs.combo[costCelBlueAmount] = 0.1;
    costs.pressure[costCelBlueAmount] = 0.1;
    costs.blocking[costCelBlueAmount] = 0.1;

    costs.name[costCelAnyblueLifeEnemy] = "costCelAnyblueLifeEnemy";
	costs.basic[costCelAnyblueLifeEnemy] = 0.1;//TODO: not implemented yet add later
    costs.combo[costCelAnyblueLifeEnemy] = 0.1;
    costs.pressure[costCelAnyblueLifeEnemy] = 0.1;
    costs.blocking[costCelAnyblueLifeEnemy] = 0.1;

    //Valk
    costs.name[costValWolfMeter] = "costValWolfMeter";
	costs.basic[costValWolfMeter] = 0.1;
    costs.combo[costValWolfMeter] = 0.1;
    costs.pressure[costValWolfMeter] = 0.1;
    costs.blocking[costValWolfMeter] = 0.0;

    costs.name[costValWolfState] = "costValWolfState";
	costs.basic[costValWolfState] = 1;
    costs.combo[costValWolfState] = 1;
    costs.pressure[costValWolfState] = 1;
    costs.blocking[costValWolfState] = 0.1;

    costs.name[costValWolfMeterEnemy] = "costValWolfMeterEnemy";
	costs.basic[costValWolfMeterEnemy] = 0.1;//TODO: not implemented yet add later
    costs.combo[costValWolfMeterEnemy] = 0.1;
    costs.pressure[costValWolfMeterEnemy] = 0.1;
    costs.blocking[costValWolfMeterEnemy] = 0.1;

    costs.name[costValWolfStateEnemy] = "costValWolfStateEnemy";
	costs.basic[costValWolfStateEnemy] = 0.2;//TODO: not implemented yet add later
    costs.combo[costValWolfStateEnemy] = 0.0;
    costs.pressure[costValWolfStateEnemy] = 0.1;
    costs.blocking[costValWolfStateEnemy] = 0.4;

    //Plat
    costs.name[costPlatCurItem] = "costPlatCurItem";
	costs.basic[costPlatCurItem] = 0.3;
    costs.combo[costPlatCurItem] = 0.3;
    costs.pressure[costPlatCurItem] = 0.3;
    costs.blocking[costPlatCurItem] = 0.3;

    costs.name[costPlatNextItem] = "costPlatNextItem";
	costs.basic[costPlatNextItem] = 0.1;
    costs.combo[costPlatNextItem] = 0.01;
    costs.pressure[costPlatNextItem] = 0.1;
    costs.blocking[costPlatNextItem] = 0.01;

    costs.name[costPlatItemNr] = "costPlatItemNr";
	costs.basic[costPlatItemNr] = 0.05;
    costs.combo[costPlatItemNr] = 0.05;
    costs.pressure[costPlatItemNr] = 0.05;
    costs.blocking[costPlatItemNr] = 0.05;

    costs.name[costPlatCurItemEnemy] = "costPlatCurItemEnemy";
	costs.basic[costPlatCurItemEnemy] = 0.1;
    costs.combo[costPlatCurItemEnemy] = 0.0;
    costs.pressure[costPlatCurItemEnemy] = 0.0;
    costs.blocking[costPlatCurItemEnemy] = 0.3;

    //Relius
    costs.name[costReliusDollMeter] = "costReliusDollMeter";
	costs.basic[costReliusDollMeter] = 0.05;
    costs.combo[costReliusDollMeter] = 0.05;
    costs.pressure[costReliusDollMeter] = 0.05;
    costs.blocking[costReliusDollMeter] = 0.05;

    costs.name[costReliusDollState] = "costReliusDollState";
	costs.basic[costReliusDollState] = 0.1;
    costs.combo[costReliusDollState] = 0.1;
    costs.pressure[costReliusDollState] = 0.1;
    costs.blocking[costReliusDollState] = 0.1;

    costs.name[costReliusDollInactive] = "costReliusDollInactive";
	costs.basic[costReliusDollInactive] = 0.6;
    costs.combo[costReliusDollInactive] = 0.6;
    costs.pressure[costReliusDollInactive] = 0.6;
    costs.blocking[costReliusDollInactive] = 0.6;

    costs.name[costReliusDollCooldown] = "costReliusDollCooldown";
	costs.basic[costReliusDollCooldown] = 1;
    costs.combo[costReliusDollCooldown] = 1;
    costs.pressure[costReliusDollCooldown] = 1;
    costs.blocking[costReliusDollCooldown] = 1;

    costs.name[costReliusDollMeterEnemy] = "costReliusDollMeterEnemy";
	costs.basic[costReliusDollMeterEnemy] = 0.05;
    costs.combo[costReliusDollMeterEnemy] = 0.05;
    costs.pressure[costReliusDollMeterEnemy] = 0.05;
    costs.blocking[costReliusDollMeterEnemy] = 0.05;

    costs.name[costReliusDollStateEnemy] = "costReliusDollStateEnemy";
	costs.basic[costReliusDollStateEnemy] = 0.1;
    costs.combo[costReliusDollStateEnemy] = 0.1;
    costs.pressure[costReliusDollStateEnemy] = 0.1;
    costs.blocking[costReliusDollStateEnemy] = 0.1;

    costs.name[costReliusDollInactiveEnemy] = "costReliusDollInactiveEnemy";
	costs.basic[costReliusDollInactiveEnemy] = 0.6;
    costs.combo[costReliusDollInactiveEnemy] = 0.6;
    costs.pressure[costReliusDollInactiveEnemy] = 0.6;
    costs.blocking[costReliusDollInactiveEnemy] = 0.6;

    costs.name[costReliusDollCooldownEnemy] = "costReliusDollCooldownEnemy";
	costs.basic[costReliusDollCooldownEnemy] = 1;
    costs.combo[costReliusDollCooldownEnemy] = 1;
    costs.pressure[costReliusDollCooldownEnemy] = 1;
    costs.blocking[costReliusDollCooldownEnemy] = 1;

    //Susan
    costs.name[costSusanUnlock1] = "costSusanUnlock1";
	costs.basic[costSusanUnlock1] = 0.1;
    costs.combo[costSusanUnlock1] = 0.1;
    costs.pressure[costSusanUnlock1] = 0.1;
    costs.blocking[costSusanUnlock1] = 0.1;

    costs.name[costSusanUnlock1Lvl] = "costSusanUnlock1Lvl";
	costs.basic[costSusanUnlock1Lvl] = 0.05;
    costs.combo[costSusanUnlock1Lvl] = 0.05;
    costs.pressure[costSusanUnlock1Lvl] = 0.05;
    costs.blocking[costSusanUnlock1Lvl] = 0.05;

    costs.name[costSusanUnlock2] = "costSusanUnlock2";
	costs.basic[costSusanUnlock2] = 0.1;
    costs.combo[costSusanUnlock2] = 0.1;
    costs.pressure[costSusanUnlock2] = 0.1;
    costs.blocking[costSusanUnlock2] = 0.1;

    costs.name[costSusanUnlock2Lvl] = "costSusanUnlock2Lvl";
	costs.basic[costSusanUnlock2Lvl] = 0.05;
    costs.combo[costSusanUnlock2Lvl] = 0.05;
    costs.pressure[costSusanUnlock2Lvl] = 0.05;
    costs.blocking[costSusanUnlock2Lvl] = 0.05;

    costs.name[costSusanUnlock3] = "costSusanUnlock3";
	costs.basic[costSusanUnlock3] = 0.1;
    costs.combo[costSusanUnlock3] = 0.1;
    costs.pressure[costSusanUnlock3] = 0.1;
    costs.blocking[costSusanUnlock3] = 0.1;

    costs.name[costSusanUnlock3Lvl] = "costSusanUnlock3Lvl";
	costs.basic[costSusanUnlock3Lvl] = 0.05;
    costs.combo[costSusanUnlock3Lvl] = 0.05;
    costs.pressure[costSusanUnlock3Lvl] = 0.05;
    costs.blocking[costSusanUnlock3Lvl] = 0.05;

    costs.name[costSusanUnlock4] = "costSusanUnlock4";
	costs.basic[costSusanUnlock4] = 0.1;
    costs.combo[costSusanUnlock4] = 0.1;
    costs.pressure[costSusanUnlock4] = 0.1;
    costs.blocking[costSusanUnlock4] = 0.1;

    costs.name[costSusanUnlock4Lvl] = "costSusanUnlock4Lvl";
	costs.basic[costSusanUnlock4Lvl] = 0.05;
    costs.combo[costSusanUnlock4Lvl] = 0.05;
    costs.pressure[costSusanUnlock4Lvl] = 0.05;
    costs.blocking[costSusanUnlock4Lvl] = 0.05;

    costs.name[costSusanUnlock5] = "costSusanUnlock5";
	costs.basic[costSusanUnlock5] = 0.1;
    costs.combo[costSusanUnlock5] = 0.1;
    costs.pressure[costSusanUnlock5] = 0.1;
    costs.blocking[costSusanUnlock5] = 0.1;

    costs.name[costSusanUnlock5Lvl] = "costSusanUnlock5Lvl";
	costs.basic[costSusanUnlock5Lvl] = 0.05;
    costs.combo[costSusanUnlock5Lvl] = 0.05;
    costs.pressure[costSusanUnlock5Lvl] = 0.05;
    costs.blocking[costSusanUnlock5Lvl] = 0.05;

    costs.name[costSusanUnlock6] = "costSusanUnlock6";
	costs.basic[costSusanUnlock6] = 0.1;
    costs.combo[costSusanUnlock6] = 0.1;
    costs.pressure[costSusanUnlock6] = 0.1;
    costs.blocking[costSusanUnlock6] = 0.1;

    costs.name[costSusanUnlock6Lvl] = "costSusanUnlock6Lvl";
	costs.basic[costSusanUnlock6Lvl] = 0.05;
    costs.combo[costSusanUnlock6Lvl] = 0.05;
    costs.pressure[costSusanUnlock6Lvl] = 0.05;
    costs.blocking[costSusanUnlock6Lvl] = 0.05;

    costs.name[costSusanUnlock7] = "costSusanUnlock7";
	costs.basic[costSusanUnlock7] = 0.1;
    costs.combo[costSusanUnlock7] = 0.1;
    costs.pressure[costSusanUnlock7] = 0.1;
    costs.blocking[costSusanUnlock7] = 0.1;

    costs.name[costSusanUnlock7Lvl] = "costSusanUnlock7Lvl";
	costs.basic[costSusanUnlock7Lvl] = 0.05;
    costs.combo[costSusanUnlock7Lvl] = 0.05;
    costs.pressure[costSusanUnlock7Lvl] = 0.05;
    costs.blocking[costSusanUnlock7Lvl] = 0.05;

    costs.name[costSusanUnlock8] = "costSusanUnlock8";
	costs.basic[costSusanUnlock8] = 0.1;
    costs.combo[costSusanUnlock8] = 0.1;
    costs.pressure[costSusanUnlock8] = 0.1;
    costs.blocking[costSusanUnlock8] = 0.1;

    costs.name[costSusanUnlock8Lvl] = "costSusanUnlock8Lvl";
	costs.basic[costSusanUnlock8Lvl] = 0.05;
    costs.combo[costSusanUnlock8Lvl] = 0.05;
    costs.pressure[costSusanUnlock8Lvl] = 0.05;
    costs.blocking[costSusanUnlock8Lvl] = 0.05;

    costs.name[costSusanUnlock9] = "costSusanUnlock9";
	costs.basic[costSusanUnlock9] = 0.1;
    costs.combo[costSusanUnlock9] = 0.1;
    costs.pressure[costSusanUnlock9] = 0.1;
    costs.blocking[costSusanUnlock9] = 0.1;


    costs.name[costSusanUnlock1Enemy] = "costSusanUnlock1Enemy";
	costs.basic[costSusanUnlock1Enemy] = 0.1;
    costs.combo[costSusanUnlock1Enemy] = 0.1;
    costs.pressure[costSusanUnlock1Enemy] = 0.1;
    costs.blocking[costSusanUnlock1Enemy] = 0.1;

    costs.name[costSusanUnlock1LvlEnemy] = "costSusanUnlock1LvlEnemy";
	costs.basic[costSusanUnlock1LvlEnemy] = 0.05;
    costs.combo[costSusanUnlock1LvlEnemy] = 0.05;
    costs.pressure[costSusanUnlock1LvlEnemy] = 0.05;
    costs.blocking[costSusanUnlock1LvlEnemy] = 0.05;

    costs.name[costSusanUnlock2Enemy] = "costSusanUnlock2Enemy";
	costs.basic[costSusanUnlock2Enemy] = 0.1;
    costs.combo[costSusanUnlock2Enemy] = 0.1;
    costs.pressure[costSusanUnlock2Enemy] = 0.1;
    costs.blocking[costSusanUnlock2Enemy] = 0.1;

    costs.name[costSusanUnlock2LvlEnemy] = "costSusanUnlock2LvlEnemy";
	costs.basic[costSusanUnlock2LvlEnemy] = 0.05;
    costs.combo[costSusanUnlock2LvlEnemy] = 0.05;
    costs.pressure[costSusanUnlock2LvlEnemy] = 0.05;
    costs.blocking[costSusanUnlock2LvlEnemy] = 0.05;

    costs.name[costSusanUnlock3Enemy] = "costSusanUnlock3Enemy";
	costs.basic[costSusanUnlock3Enemy] = 0.1;
    costs.combo[costSusanUnlock3Enemy] = 0.1;
    costs.pressure[costSusanUnlock3Enemy] = 0.1;
    costs.blocking[costSusanUnlock3Enemy] = 0.1;

    costs.name[costSusanUnlock3LvlEnemy] = "costSusanUnlock3LvlEnemy";
	costs.basic[costSusanUnlock3LvlEnemy] = 0.05;
    costs.combo[costSusanUnlock3LvlEnemy] = 0.05;
    costs.pressure[costSusanUnlock3LvlEnemy] = 0.05;
    costs.blocking[costSusanUnlock3LvlEnemy] = 0.05;

    costs.name[costSusanUnlock4Enemy] = "costSusanUnlock4Enemy";
	costs.basic[costSusanUnlock4Enemy] = 0.1;
    costs.combo[costSusanUnlock4Enemy] = 0.1;
    costs.pressure[costSusanUnlock4Enemy] = 0.1;
    costs.blocking[costSusanUnlock4Enemy] = 0.1;

    costs.name[costSusanUnlock4LvlEnemy] = "costSusanUnlock4LvlEnemy";
	costs.basic[costSusanUnlock4LvlEnemy] = 0.05;
    costs.combo[costSusanUnlock4LvlEnemy] = 0.05;
    costs.pressure[costSusanUnlock4LvlEnemy] = 0.05;
    costs.blocking[costSusanUnlock4LvlEnemy] = 0.05;

    costs.name[costSusanUnlock5Enemy] = "costSusanUnlock5Enemy";
	costs.basic[costSusanUnlock5Enemy] = 0.1;
    costs.combo[costSusanUnlock5Enemy] = 0.1;
    costs.pressure[costSusanUnlock5Enemy] = 0.1;
    costs.blocking[costSusanUnlock5Enemy] = 0.1;

    costs.name[costSusanUnlock5LvlEnemy] = "costSusanUnlock5LvlEnemy";
	costs.basic[costSusanUnlock5LvlEnemy] = 0.05;
    costs.combo[costSusanUnlock5LvlEnemy] = 0.05;
    costs.pressure[costSusanUnlock5LvlEnemy] = 0.05;
    costs.blocking[costSusanUnlock5LvlEnemy] = 0.05;

    costs.name[costSusanUnlock6Enemy] = "costSusanUnlock6Enemy";
	costs.basic[costSusanUnlock6Enemy] = 0.1;
    costs.combo[costSusanUnlock6Enemy] = 0.1;
    costs.pressure[costSusanUnlock6Enemy] = 0.1;
    costs.blocking[costSusanUnlock6Enemy] = 0.1;

    costs.name[costSusanUnlock6LvlEnemy] = "costSusanUnlock6LvlEnemy";
	costs.basic[costSusanUnlock6LvlEnemy] = 0.05;
    costs.combo[costSusanUnlock6LvlEnemy] = 0.05;
    costs.pressure[costSusanUnlock6LvlEnemy] = 0.05;
    costs.blocking[costSusanUnlock6LvlEnemy] = 0.05;

    costs.name[costSusanUnlock7Enemy] = "costSusanUnlock7Enemy";
	costs.basic[costSusanUnlock7Enemy] = 0.1;
    costs.combo[costSusanUnlock7Enemy] = 0.1;
    costs.pressure[costSusanUnlock7Enemy] = 0.1;
    costs.blocking[costSusanUnlock7Enemy] = 0.1;

    costs.name[costSusanUnlock7LvlEnemy] = "costSusanUnlock7LvlEnemy";
	costs.basic[costSusanUnlock7LvlEnemy] = 0.05;
    costs.combo[costSusanUnlock7LvlEnemy] = 0.05;
    costs.pressure[costSusanUnlock7LvlEnemy] = 0.05;
    costs.blocking[costSusanUnlock7LvlEnemy] = 0.05;

    costs.name[costSusanUnlock8Enemy] = "costSusanUnlock8Enemy";
	costs.basic[costSusanUnlock8Enemy] = 0.1;
    costs.combo[costSusanUnlock8Enemy] = 0.1;
    costs.pressure[costSusanUnlock8Enemy] = 0.1;
    costs.blocking[costSusanUnlock8Enemy] = 0.1;

    costs.name[costSusanUnlock8LvlEnemy] = "costSusanUnlock8Lvl";
	costs.basic[costSusanUnlock8LvlEnemy] = 0.05;
    costs.combo[costSusanUnlock8LvlEnemy] = 0.05;
    costs.pressure[costSusanUnlock8LvlEnemy] = 0.05;
    costs.blocking[costSusanUnlock8LvlEnemy] = 0.05;

    costs.name[costSusanUnlock9Enemy] = "costSusanUnlock9Enemy";
	costs.basic[costSusanUnlock9Enemy] = 0.1;
    costs.combo[costSusanUnlock9Enemy] = 0.1;
    costs.pressure[costSusanUnlock9Enemy] = 0.1;
    costs.blocking[costSusanUnlock9Enemy] = 0.1;

    //Jubei
    costs.name[costJubeiBuff] = "costJubeiBuff";
	costs.basic[costJubeiBuff] = 0.3;
    costs.combo[costJubeiBuff] = 0.3;
    costs.pressure[costJubeiBuff] = 0.3;
    costs.blocking[costJubeiBuff] = 0.3;

    costs.name[costJubeiMark] = "costJubeiMark";
	costs.basic[costJubeiMark] = 0.1;
    costs.combo[costJubeiMark] = 0.1;
    costs.pressure[costJubeiMark] = 0.1;
    costs.blocking[costJubeiMark] = 0.1;

    costs.name[costJubeiBuffEnemy] = "costJubeiBuffEnemy";
	costs.basic[costJubeiBuffEnemy] = 0.3;
    costs.combo[costJubeiBuffEnemy] = 0.3;
    costs.pressure[costJubeiBuffEnemy] = 0.3;
    costs.blocking[costJubeiBuffEnemy] = 0.3;

    costs.name[costJubeiMarkEnemy] = "costJubeiMarkEnemy";
	costs.basic[costJubeiMarkEnemy] = 0.1;
    costs.combo[costJubeiMarkEnemy] = 0.1;
    costs.pressure[costJubeiMarkEnemy] = 0.1;
    costs.blocking[costJubeiMarkEnemy] = 0.1;

    //Izanami
    costs.name[costIzanamiFloating] = "costIzanamiFloating";
	costs.basic[costIzanamiFloating] = 0.2;
    costs.combo[costIzanamiFloating] = 0.2;
    costs.pressure[costIzanamiFloating] = 0.2;
    costs.blocking[costIzanamiFloating] = 0.2;

    costs.name[costIzanamiRibcage] = "costIzanamiRibcage";
	costs.basic[costIzanamiRibcage] = 0.6;
    costs.combo[costIzanamiRibcage] = 0.6;
    costs.pressure[costIzanamiRibcage] = 0.6;
    costs.blocking[costIzanamiRibcage] = 0.6;

    costs.name[costIzanamiShotCooldown] = "costIzanamiShotCooldown";
	costs.basic[costIzanamiShotCooldown] = 0.2;
    costs.combo[costIzanamiShotCooldown] = 0.2;
    costs.pressure[costIzanamiShotCooldown] = 0.2;
    costs.blocking[costIzanamiShotCooldown] = 0.2;

    costs.name[costIzanamiBitsStance] = "costIzanamiBitsStance";
	costs.basic[costIzanamiBitsStance] = 0.2;
    costs.combo[costIzanamiBitsStance] = 0.2;
    costs.pressure[costIzanamiBitsStance] = 0.2;
    costs.blocking[costIzanamiBitsStance] = 0.2;

    costs.name[costIzanamiFloatingEnemy] = "costIzanamiFloatingEnemy";
	costs.basic[costIzanamiFloatingEnemy] = 0.2;
    costs.combo[costIzanamiFloatingEnemy] = 0.2;
    costs.pressure[costIzanamiFloatingEnemy] = 0.2;
    costs.blocking[costIzanamiFloatingEnemy] = 0.2;

    costs.name[costIzanamiRibcageEnemy] = "costIzanamiRibcageEnemy";
	costs.basic[costIzanamiRibcageEnemy] = 0.6;
    costs.combo[costIzanamiRibcageEnemy] = 0.6;
    costs.pressure[costIzanamiRibcageEnemy] = 0.6;
    costs.blocking[costIzanamiRibcageEnemy] = 0.6;

    costs.name[costIzanamiShotCooldownEnemy] = "costIzanamiShotCooldownEnemy";
	costs.basic[costIzanamiShotCooldownEnemy] = 0.2;
    costs.combo[costIzanamiShotCooldownEnemy] = 0.2;
    costs.pressure[costIzanamiShotCooldownEnemy] = 0.2;
    costs.blocking[costIzanamiShotCooldownEnemy] = 0.2;

    costs.name[costIzanamiBitsStanceEnemy] = "costIzanamiBitsStanceEnemy";
	costs.basic[costIzanamiBitsStanceEnemy] = 0.2;
    costs.combo[costIzanamiBitsStanceEnemy] = 0.2;
    costs.pressure[costIzanamiBitsStanceEnemy] = 0.2;
    costs.blocking[costIzanamiBitsStanceEnemy] = 0.2;

    //Nine
    costs.name[costNineSpell] = "costNineSpell";
	costs.basic[costNineSpell] = 0.3;
    costs.combo[costNineSpell] = 0.3;
    costs.pressure[costNineSpell] = 0.3;
    costs.blocking[costNineSpell] = 0.3;

    costs.name[costNineSpellBackup] = "costNineSpellBackup";
	costs.basic[costNineSpellBackup] = 0.1;
    costs.combo[costNineSpellBackup] = 0.1;
    costs.pressure[costNineSpellBackup] = 0.1;
    costs.blocking[costNineSpellBackup] = 0.1;

    costs.name[costNineSlots] = "costNineSlots";
	costs.basic[costNineSlots] = 0.1;
    costs.combo[costNineSlots] = 0.1;
    costs.pressure[costNineSlots] = 0.1;
    costs.blocking[costNineSlots] = 0.1;

    costs.name[costNineSlotsBackup] = "costNineSlotsBackup";
	costs.basic[costNineSlotsBackup] = 0.01;
    costs.combo[costNineSlotsBackup] = 0.01;
    costs.pressure[costNineSlotsBackup] = 0.01;
    costs.blocking[costNineSlotsBackup] = 0.01;

    costs.name[costNineSpellEnemy] = "costNineSpellEnemy";
	costs.basic[costNineSpellEnemy] = 0.3;
    costs.combo[costNineSpellEnemy] = 0.3;
    costs.pressure[costNineSpellEnemy] = 0.3;
    costs.blocking[costNineSpellEnemy] = 0.3;

    costs.name[costNineSpellBackupEnemy] = "costNineSpellBackupEnemy";
	costs.basic[costNineSpellBackupEnemy] = 0.1;
    costs.combo[costNineSpellBackupEnemy] = 0.1;
    costs.pressure[costNineSpellBackupEnemy] = 0.1;
    costs.blocking[costNineSpellBackupEnemy] = 0.1;

    costs.name[costNineSlotsEnemy] = "costNineSlotsEnemy";
	costs.basic[costNineSlotsEnemy] = 0.1;
    costs.combo[costNineSlotsEnemy] = 0.1;
    costs.pressure[costNineSlotsEnemy] = 0.1;
    costs.blocking[costNineSlotsEnemy] = 0.1;

    costs.name[costNineSlotsBackupEnemy] = "costNineSlotsBackupEnemy";
	costs.basic[costNineSlotsBackupEnemy] = 0.01;
    costs.combo[costNineSlotsBackupEnemy] = 0.01;
    costs.pressure[costNineSlotsBackupEnemy] = 0.01;
    costs.blocking[costNineSlotsBackupEnemy] = 0.01;

    //Carl
    costs.name[costCarlDollInactive] = "costCarlDollInactive";
	costs.basic[costCarlDollInactive] = 1;
    costs.combo[costCarlDollInactive] = 1;
    costs.pressure[costCarlDollInactive] = 1;
    costs.blocking[costCarlDollInactive] = 1;

    costs.name[costCarlDollMeter] = "costCarlDollMeter";
	costs.basic[costCarlDollMeter] = 0.1;
    costs.combo[costCarlDollMeter] = 0.1;
    costs.pressure[costCarlDollMeter] = 0.1;
    costs.blocking[costCarlDollMeter] = 0.1;

    costs.name[costCarlDollInactiveEnemy] = "costCarlDollInactiveEnemy";
	costs.basic[costCarlDollInactiveEnemy] = 1;
    costs.combo[costCarlDollInactiveEnemy] = 1;
    costs.pressure[costCarlDollInactiveEnemy] = 1;
    costs.blocking[costCarlDollInactiveEnemy] = 1;

    costs.name[costCarlDollMeterEnemy] = "costCarlDollMeterEnemy";
	costs.basic[costCarlDollMeterEnemy] = 0.1;
    costs.combo[costCarlDollMeterEnemy] = 0.1;
    costs.pressure[costCarlDollMeterEnemy] = 0.1;
    costs.blocking[costCarlDollMeterEnemy] = 0.1;

    //Tsubaki
    costs.name[costTsubakiMeter] = "costTsubakiMeter";
	costs.basic[costTsubakiMeter] = 1;
    costs.combo[costTsubakiMeter] = 1;
    costs.pressure[costTsubakiMeter] = 1;
    costs.blocking[costTsubakiMeter] = 1;

    costs.name[costTsubakiMeterEnemy] = "costTsubakiMeterEnemy";
	costs.basic[costTsubakiMeterEnemy] = 1;
    costs.combo[costTsubakiMeterEnemy] = 1;
    costs.pressure[costTsubakiMeterEnemy] = 1;
    costs.blocking[costTsubakiMeterEnemy] = 1;

    //Kokonoe
    costs.name[costKokoGravitronNr] = "costKokoGravitronNr";
	costs.basic[costKokoGravitronNr] = 0.1;
    costs.combo[costKokoGravitronNr] = 0.1;
    costs.pressure[costKokoGravitronNr] = 0.1;
    costs.blocking[costKokoGravitronNr] = 0.1;

    costs.name[costKokoAnyGraviLeft] = "costKokoAnyGraviLeft";
	costs.basic[costKokoAnyGraviLeft] = 0.3;
    costs.combo[costKokoAnyGraviLeft] = 0.3;
    costs.pressure[costKokoAnyGraviLeft] = 0.3;
    costs.blocking[costKokoAnyGraviLeft] = 0.3;

    costs.name[costKokoGravitronNrEnemy] = "costKokoGravitronNrEnemy";
	costs.basic[costKokoGravitronNrEnemy] = 0.1;
    costs.combo[costKokoGravitronNrEnemy] = 0.1;
    costs.pressure[costKokoGravitronNrEnemy] = 0.1;
    costs.blocking[costKokoGravitronNrEnemy] = 0.1;

    costs.name[costKokoAnyGraviLeftEnemy] = "costKokoAnyGraviLeftEnemy";
	costs.basic[costKokoAnyGraviLeftEnemy] = 0.3;
    costs.combo[costKokoAnyGraviLeftEnemy] = 0.3;
    costs.pressure[costKokoAnyGraviLeftEnemy] = 0.3;
    costs.blocking[costKokoAnyGraviLeftEnemy] = 0.3;

    //Izayoi
    costs.name[costIzayoiState] = "costIzayoiState";
	costs.basic[costIzayoiState] = 0.3;
    costs.combo[costIzayoiState] = 0.3;
    costs.pressure[costIzayoiState] = 0.3;
    costs.blocking[costIzayoiState] = 0.3;

    costs.name[costIzayoiStocks] = "costIzayoiStocks";
	costs.basic[costIzayoiStocks] = 0.8;
    costs.combo[costIzayoiStocks] = 0.8;
    costs.pressure[costIzayoiStocks] = 0.8;
    costs.blocking[costIzayoiStocks] = 0.8;

    costs.name[costIzayoiSupermode] = "costIzayoiSupermode";
	costs.basic[costIzayoiSupermode] = 0.4;
    costs.combo[costIzayoiSupermode] = 0.4;
    costs.pressure[costIzayoiSupermode] = 0.4;
    costs.blocking[costIzayoiSupermode] = 0.4;

    costs.name[costIzayoiStateEnemy] = "costIzayoiStateEnemy";
	costs.basic[costIzayoiStateEnemy] = 0.3;
    costs.combo[costIzayoiStateEnemy] = 0.3;
    costs.pressure[costIzayoiStateEnemy] = 0.3;
    costs.blocking[costIzayoiStateEnemy] = 0.3;

    costs.name[costIzayoiStocksEnemy] = "costIzayoiStocksEnemy";
	costs.basic[costIzayoiStocksEnemy] = 0.8;
    costs.combo[costIzayoiStocksEnemy] = 0.8;
    costs.pressure[costIzayoiStocksEnemy] = 0.8;
    costs.blocking[costIzayoiStocksEnemy] = 0.8;

    costs.name[costIzayoiSupermodeEnemy] = "costIzayoiSupermodeEnemy";
	costs.basic[costIzayoiSupermodeEnemy] = 0.4;
    costs.combo[costIzayoiSupermodeEnemy] = 0.4;
    costs.pressure[costIzayoiSupermodeEnemy] = 0.4;
    costs.blocking[costIzayoiSupermodeEnemy] = 0.4;

    //Arakune
    costs.name[costArakuneCurseMeter] = "costArakuneCurseMeter";
	costs.basic[costArakuneCurseMeter] = 0.1;
    costs.combo[costArakuneCurseMeter] = 0.1;
    costs.pressure[costArakuneCurseMeter] = 0.1;
    costs.blocking[costArakuneCurseMeter] = 0.1;

    costs.name[costArakuneCurseActive] = "costArakuneCurseActive";
	costs.basic[costArakuneCurseActive] = 0.8;
    costs.combo[costArakuneCurseActive] = 0.8;
    costs.pressure[costArakuneCurseActive] = 0.8;
    costs.blocking[costArakuneCurseActive] = 0.8;

    costs.name[costArakuneCurseMeterEnemy] = "costArakuneCurseMeterEnemy";
	costs.basic[costArakuneCurseMeterEnemy] = 0.1;
    costs.combo[costArakuneCurseMeterEnemy] = 0.1;
    costs.pressure[costArakuneCurseMeterEnemy] = 0.1;
    costs.blocking[costArakuneCurseMeterEnemy] = 0.1;

    costs.name[costArakuneCurseActiveEnemy] = "costArakuneCurseActiveEnemy";
	costs.basic[costArakuneCurseActiveEnemy] = 0.8;
    costs.combo[costArakuneCurseActiveEnemy] = 0.8;
    costs.pressure[costArakuneCurseActiveEnemy] = 0.8;
    costs.blocking[costArakuneCurseActiveEnemy] = 0.8;

    //Bang
    costs.name[costBangFRKZ1] = "costBangFRKZ1";
	costs.basic[costBangFRKZ1] = 0.05;
    costs.combo[costBangFRKZ1] = 0.05;
    costs.pressure[costBangFRKZ1] = 0.05;
    costs.blocking[costBangFRKZ1] = 0.05;

    costs.name[costBangFRKZ2] = "costBangFRKZ2";
	costs.basic[costBangFRKZ2] = 0.05;
    costs.combo[costBangFRKZ2] = 0.05;
    costs.pressure[costBangFRKZ2] = 0.05;
    costs.blocking[costBangFRKZ2] = 0.05;

    costs.name[costBangFRKZ3] = "costBangFRKZ3";
	costs.basic[costBangFRKZ3] = 0.05;
    costs.combo[costBangFRKZ3] = 0.05;
    costs.pressure[costBangFRKZ3] = 0.05;
    costs.blocking[costBangFRKZ3] = 0.05;

    costs.name[costBangFRKZ4] = "costBangFRKZ4";
	costs.basic[costBangFRKZ4] = 0.05;
    costs.combo[costBangFRKZ4] = 0.05;
    costs.pressure[costBangFRKZ4] = 0.05;
    costs.blocking[costBangFRKZ4] = 0.05;

    costs.name[costBangNailcount] = "costBangNailcount";
	costs.basic[costBangNailcount] = 0.4;
    costs.combo[costBangNailcount] = 0.4;
    costs.pressure[costBangNailcount] = 0.4;
    costs.blocking[costBangNailcount] = 0.4;

    costs.name[costBangFRKZ1Enemy] = "costBangFRKZ1Enemy";
	costs.basic[costBangFRKZ1Enemy] = 0.05;
    costs.combo[costBangFRKZ1Enemy] = 0.05;
    costs.pressure[costBangFRKZ1Enemy] = 0.05;
    costs.blocking[costBangFRKZ1Enemy] = 0.05;

    costs.name[costBangFRKZ2Enemy] = "costBangFRKZ2Enemy";
	costs.basic[costBangFRKZ2Enemy] = 0.05;
    costs.combo[costBangFRKZ2Enemy] = 0.05;
    costs.pressure[costBangFRKZ2Enemy] = 0.05;
    costs.blocking[costBangFRKZ2Enemy] = 0.05;

    costs.name[costBangFRKZ3Enemy] = "costBangFRKZ3Enemy";
	costs.basic[costBangFRKZ3Enemy] = 0.05;
    costs.combo[costBangFRKZ3Enemy] = 0.05;
    costs.pressure[costBangFRKZ3Enemy] = 0.05;
    costs.blocking[costBangFRKZ3Enemy] = 0.05;

    costs.name[costBangFRKZ4Enemy] = "costBangFRKZ4Enemy";
	costs.basic[costBangFRKZ4Enemy] = 0.05;
    costs.combo[costBangFRKZ4Enemy] = 0.05;
    costs.pressure[costBangFRKZ4Enemy] = 0.05;
    costs.blocking[costBangFRKZ4Enemy] = 0.05;

    costs.name[costBangNailcountEnemy] = "costBangNailcountEnemy";
	costs.basic[costBangNailcountEnemy] = 0.4;
    costs.combo[costBangNailcountEnemy] = 0.4;
    costs.pressure[costBangNailcountEnemy] = 0.4;
    costs.blocking[costBangNailcountEnemy] = 0.4;

    costs.name[costMinDistanceAttack] = "costMinDistanceAttack";
    costs.basic[costMinDistanceAttack] = 0.5;
    costs.combo[costMinDistanceAttack] = 0.5;
    costs.pressure[costMinDistanceAttack] = 0.5;
    costs.blocking[costMinDistanceAttack] = 0.5;
}


void CbrData::setEnabled(bool b) {
    enabled = b;
}
void CbrData::setPlayerName(std::string name) {
    playerName = name;
}
bool CbrData::getEnabled() {
    return enabled;
}
int CbrData::getCharacterIndex() {
    return characterIndex;
}

void CbrData::AddReplay(CbrReplayFile f)
{
    replayFiles.push_back(f);
}


void CbrData::deleteReplays(int start, int end) {
    if (end <= 0) { return; }
    if (start >= replayFiles.size()) { return; }
    replayFiles.erase(std::next(replayFiles.begin(), std::max(start,0)), std::next(replayFiles.begin(), std::min(end+1, (int)replayFiles.size())));
}
CbrReplayFile* CbrData::getLastReplay() {
    return  &replayFiles[replayFiles.size() - 1];
}
std::vector<CbrReplayFile>* CbrData::getReplayFiles(){
    return  &replayFiles;
}
void CbrData::setCharName(std::string s) {
    characterName = s;
}


void updateDebugCaseIndex(std::vector<debugCaseIndex>& dci, debugCaseIndex newDci) {
    for (int i = 0; i < dci.size(); i++) {
        
        if (i == 0 && newDci.compValue <= dci[i].compValue) {
            dci.insert(dci.begin(), newDci);
            return;
        }
        if (newDci.compValue <= dci[i].compValue) {
            dci.insert(dci.begin() + i, newDci);
            return;
        }
    }
    dci.insert(dci.end(), newDci);
    return;
}

bool checkBestCaseBetterThanNext(float bufComparison, float bestComparison, CbrCase* nextCase) {

    if (nextCase->getInputBufferSequence()) {
        return (bufComparison <= ((bestComparison * nextBestMulti) + nextBestAdd + nextBestAddInputSequence));
    }
    return (bufComparison <= (bestComparison * nextBestMulti + nextBestAdd));
    
}
bool randomDifferenceSelection(float bufComparison, float bestComparison) {
    auto diff = bufComparison - bestComparison;
    
    if (diff < maxRandomDiff * -1) {
        return true;
    }
    if (diff > maxRandomDiff) {
        return false;
    }

    //if the new case is better than the old, give the old case a chance to stay depending on how much better the new case is
    if (diff < 0){
        diff = abs(diff) * -5 + 0.5; // map the difference to a scale of 0 to 0.5 for easier probability calc.
        auto fl = RandomFloatC(0, 1);
        if (fl <= diff) { return false; }
        else { return true; }
    }
    //If the old case is better than the new case, give the new case a chance to be selected anyway.
    if (diff >= 0 ) {
        diff = diff * -5 + 0.5; // map the difference to a scale of 0 to 0.5 for easier probability calc.
        auto fl = RandomFloatC(0, 1);
        if (fl > diff) { return false; }
        else { return true; }
    }
}
void CbrData::bestCaseCultivator(float bufComparison, int replayIndex, int caseIndex) {
    auto diff = bufComparison - caseSelector.bestCompValue;
    if (bufComparison == 0) { bufComparison = 0.001; }
    if (diff < maxRandomDiff * -1) {
        caseSelector.bestCompValue = bufComparison;
        caseSelector.combinedValue = 1/bufComparison;
        caseSelector.bcc.clear();
        bestCaseContainer b{ replayIndex, caseIndex, bufComparison };
        caseSelector.bcc.push_back(b);
    }
    else {
        if (diff <= maxRandomDiff) {
            if (diff < 0) {
                caseSelector.bestCompValue = bufComparison;
            }
            caseSelector.combinedValue += 1/bufComparison;
            bestCaseContainer b{ replayIndex, caseIndex, bufComparison };
            caseSelector.bcc.push_back(b);
        }
    }
    

}
bestCaseContainer CbrData::bestCaseSelector() {
    int combinedPropability = caseSelector.combinedValue;
    float multi = static_cast<float>(100) / combinedPropability;
    float probSave = 0;
    float rand = RandomFloat(0, 100);
    for (int i = 0; i < caseSelector.bcc.size(); i++) {
        float max = (1/caseSelector.bcc[i].compValue) * multi;
        auto test = max + probSave;
        probSave += max;
        if (test > rand) {
            return caseSelector.bcc[i];
            break;
        }
    }
    bestCaseContainer b = { -1,-1,-1 };
    return b;

}
int CbrData::CBRcomputeNextAction(Metadata* curGamestate) {
    float bestComparison = 9999999;
    int bestReplay = -1;
    int bestCase = -1;
    int bestFrame = -1;
    int bestCompBufferCase = -2;
    int bestCompBufferReplay = -2;

    std::vector<debugCaseIndex> dci;

    framesActive++;
    
    float bufComparison = 9999999;
    activeFrame++;
    //if we are searching for a case for the first time or the case is done playing or the player is hit/lands a hit search for a new case
    if (switchCaseCheck(curGamestate)){
        caseSelector.bcc.clear();
        caseSelector.bestCompValue = 9999999;
        caseSelector.combinedValue = 0;
        setCurGamestateCosts(curGamestate);
        auto replayFileSize = replayFiles.size();
        for (std::size_t i = 0; i < replayFileSize; ++i) {
            auto caseBaseSize = replayFiles[i].getCaseBase()->size();
            for (std::size_t j = 0; j < caseBaseSize; ++j) {
                //caseValidityCheck(curGamestate, i,j)
                auto caseGamestate = replayFiles[i].getCase(j)->getMetadata();
                bufComparison = comparisonFunction(curGamestate, caseGamestate, replayFiles[i], replayFiles[i].getCase(j), i, j, false);
                //TODO: remove this after debugging due to massive performance hit.
                //bufComparison += HelperCompMatch(curGamestate, caseGamestate);
                //debugCaseIndex newDci{};
                //newDci.compValue = bufComparison;
                //newDci.caseIndex = j;
                //newDci.replayIndex = i;
                //updateDebugCaseIndex(dci, newDci);
                if (bufComparison <= bestComparison + maxRandomDiff) {
                    //only doing helperComp if this even has a chance to be the best because it is costly
                    bufComparison += HelperCompMatch(curGamestate, caseGamestate);

                    bestCaseCultivator(bufComparison, i, j);
                    /*old selection
                    if (randomDifferenceSelection(bufComparison, bestComparison)) {
                        if (bufComparison == bestComparison ) {
                            if (bestCompBufferCase == (j - 1) && bestCompBufferReplay == i) {
                                bestCompBufferCase = j;
                                bestFrame = replayFiles[i].getCase(j)->getStartingIndex();
                            }
                            else if(RandomInt(0,1)==1){
                                bestComparison = bufComparison;
                                bestReplay = i;
                                bestCase = j;
                                bestFrame = replayFiles[i].getCase(j)->getStartingIndex();
                                bestCompBufferCase = j;
                                bestCompBufferReplay = i;
                            }
                     
                                
                        }
                        else {
                            bestComparison = bufComparison;
                            bestReplay = i;
                            bestCase = j;
                            bestFrame = replayFiles[i].getCase(j)->getStartingIndex();
                            bestCompBufferCase = j;
                            bestCompBufferReplay = i;
                        }
                    }*/

                }
                
            }
        }
        auto selectedCase = bestCaseSelector();
        bestComparison = selectedCase.compValue;
        bestReplay = selectedCase.replayIndex;
        bestCase = selectedCase.caseIndex;
        if (bestReplay != -1) {
            bestFrame = replayFiles[selectedCase.replayIndex].getCase(selectedCase.caseIndex)->getStartingIndex();
        }
        if (activeReplay != -1 && activeCase +1 <= replayFiles[activeReplay].getCaseBaseLength() -1 ){
            float bufComparison = 9999999;
            float bufComparison2 = 9999999; 
            bufComparison = comparisonFunction(curGamestate, replayFiles[activeReplay].getCase(activeCase + 1)->getMetadata(), replayFiles[activeReplay], replayFiles[activeReplay].getCase(activeCase + 1), activeReplay, activeCase+1, true);
            bufComparison += HelperCompMatch(curGamestate, replayFiles[activeReplay].getCase(activeCase + 1)->getMetadata());
            if (bestReplay == -1 && bestCase == -1) {
                if (bufComparison < bestComparison) {
                    bufComparison2 = 9999999;
                }
            }
            else {
                if (checkBestCaseBetterThanNext(bufComparison, bestComparison, replayFiles[activeReplay].getCase(activeCase + 1))) {
                    debugPrint(curGamestate, activeCase + 1, bestCase, activeReplay, bestReplay, dci, activeReplay, activeCase + 1);
                }
                else {
                    debugPrint(curGamestate, activeCase + 1, bestCase, activeReplay, bestReplay, dci, bestReplay, bestCase);
                }
            }


            if (checkBestCaseBetterThanNext(bufComparison, bestComparison, replayFiles[activeReplay].getCase(activeCase + 1))) {
                bestReplay = activeReplay;
                bestComparison = bufComparison;
                bestCase = activeCase + 1;
                bestFrame = replayFiles[bestReplay].getCase(bestCase)->getStartingIndex();
            }
        }
        else {
            if (bestCase != -1) {
                debugPrint(curGamestate, activeCase + 1, bestCase, activeReplay, bestReplay, dci, bestReplay, bestCase);
            }
            
        }

        activeReplay = bestReplay;
        activeCase = bestCase;
        activeFrame = bestFrame;
        
        if (activeFrame != -1) {
            invertCurCase = replayFiles[bestReplay].getCase(bestCase)->getMetadata()->getFacing() != curGamestate->getFacing();
        }
        
        if (activeReplay != -1 && activeCase != -1 && activeFrame != -1) {
            replayFiles[activeReplay].getCase(activeCase)->caseCooldownFrameStart = framesActive;
        }
        
    }

    

    if (activeFrame == -1) { return 5; }
    auto input = replayFiles[activeReplay].getInput(activeFrame);
    
    //replayFiles[activeReplay].getCase(activeCase)->getMetadata().getFacing() != curGamestate->getFacing()
    if (invertCurCase) {
        input = inverseInput(input);
    }

    return input;
}
void CbrData::resetCbr() {
    activeReplay = -1;
    activeCase = -1;
    activeFrame = -1;
    invertCurCase = false;
}

bool CbrData::switchCaseCheck(Metadata* curGamestate) {
    //No current case
    auto b0 = activeReplay == -1;
    //CurrentCase ending
    auto b1 = !b0 && activeFrame > replayFiles[activeReplay].getCase(activeCase)->getEndIndex();
    //Self hit
    auto b2 = !b0 && curGamestate->getHitThisFrame()[0];
    //Self Block
    auto b3 = !b0 && curGamestate->getBlockThisFrame()[0];
    //Hitting the opponent while not buffering an input
    auto b4 = !b0 && (curGamestate->getHitThisFrame()[1] || curGamestate->getBlockThisFrame()[1]) && !replayFiles[activeReplay].getCase(activeCase)->getMetadata()->getInputBufferActive();

    caseSwitchReason = "";
    if (b0) { caseSwitchReason += "No current case - "; }
    if (b1) { caseSwitchReason += "Case Ending - "; }
    if (b2) { caseSwitchReason += "Self hit - "; }
    if (b3) { caseSwitchReason += "Self Block - "; }
    if (b4) { caseSwitchReason += "HitOpponent"; }
    return b0 || b1 || b2 || b3 || b4;
}

bool CbrData::sameOrNextCaseCheck(Metadata* curGamestate, int replayIndex, int caseIndex) {
    return replayIndex == activeReplay && (caseIndex == activeCase || caseIndex == activeCase + 1);
}
bool CbrData::inputSequenceCheck(Metadata* curGamestate, int replayIndex, int caseIndex) {
    return caseIndex != 0 && replayFiles[replayIndex].getCase(caseIndex)->getInputBufferSequence() == true;
}
bool CbrData::meterCheck(Metadata* curGamestate, int replayIndex, int caseIndex) {
    return replayFiles[replayIndex].getCase(caseIndex)->heatConsumed > curGamestate->heatMeter[0];
}
bool CbrData::OdCheck(Metadata* curGamestate, int replayIndex, int caseIndex) {
    return replayFiles[replayIndex].getCase(caseIndex)->overDriveConsumed != 0 && curGamestate->overdriveMeter[0] != 100000;
}
bool CbrData::caseValidityCheck(Metadata* curGamestate, int replayIndex, int caseIndex) {
     //The case beeing checked isent the same as the one that was just played.
    auto check1 = sameOrNextCaseCheck(curGamestate, replayIndex, caseIndex); //The case beeing checked isent the same as the one that was just played.
    auto check2 = inputSequenceCheck(curGamestate, replayIndex, caseIndex);//The case isent starting in the middle of a input sequence
    auto check3 = nextCaseValidityCheck(curGamestate, replayIndex, caseIndex);//Meter and OD checks

    return check1 && check2 && check3;
}
//Checks if the next case can be used.
bool CbrData::nextCaseValidityCheck(Metadata* curGamestate, int replayIndex, int caseIndex) {
    auto check1 = meterCheck(curGamestate, replayIndex, caseIndex);//Has enough meter to use a super
    auto check2 = OdCheck(curGamestate, replayIndex, caseIndex);//Can use overdrive
    return check1 && check2;
}

void print_all(std::stringstream& ss) {
    // base case
}

template <class T, class... Ts>
void print_all(std::stringstream& ss, T const& first, Ts const&... rest) {
    ss << first << ", ";

    print_all(ss, rest...);
}

template<class F, class ...Args>
auto debug_call(F func, const char* str, std::string* debugText, float multi, Args... args) ->
decltype(std::forward<F>(func)(std::forward<Args>(args)...))
{
    
    if constexpr (std::is_same<decltype(std::forward<F>(func)(std::forward<Args>(args)...)), void>::value) {

        std::stringstream ss;
        ((ss << args << ", ") << ...);
        ss << str << "\n";

        func(args...);
    }
    else {
        auto res = func(args...)* multi;
        if (res != 0) {
            std::stringstream ss;
            //((ss << args), ...);
            ss << res << " = ";
            print_all(ss, args...);
            ss << str << "\n";
            *debugText += ss.str();
        }
        
        return  res;
    }
}
#define REFLECT_INVOKE(func, ...) (debug_call)(func, #func "(" #__VA_ARGS__ ") ", &debugText, __VA_ARGS__)

template<class F, class ...Args>
auto pure_call(F func, const char* str, float multi, Args... args) ->
decltype(std::forward<F>(func)(std::forward<Args>(args)...))
{
    if constexpr (std::is_same<decltype(std::forward<F>(func)(std::forward<Args>(args)...)), void>::value) {
        func(args...);
    }
    else {
        auto res = func(args...) * multi;
        return  res;
    }
}
#define PURE_INVOKE(func, ...) (pure_call)(func, #func "(" #__VA_ARGS__ ") ", __VA_ARGS__)

float CbrData::comparisonFunction(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int replayIndex, int caseIndex, bool nextCaseCheck) {
    float compValue = 0;

    bool buffVal = false;
    if (!nextCaseCheck) {
        buffVal = sameOrNextCaseCheck(curGamestate, replayIndex, caseIndex);
        if (buffVal) {
            //debugText += "sameOrNextCaseCheck Fail\n";
            buffVal = false;
            compValue += 999;
        }
        buffVal = inputSequenceCheck(curGamestate, replayIndex, caseIndex);
        if (buffVal) {
            //debugText += "inputSequenceCheck Fail\n";
            buffVal = false;
            compValue += 999;
        }
    }
    buffVal = meterCheck(curGamestate, replayIndex, caseIndex);
    if (buffVal) {
        //debugText += "meterCheck Fail\n";
        buffVal = false;
        compValue += 999;
    }
    buffVal = OdCheck(curGamestate, replayIndex, caseIndex);
    if (buffVal) {
        //debugText += "OdCheck Fail\n";
        buffVal = false;
        compValue += 999;
    }
    if (compValue > 0) {
        return compValue;
    }

    compValue += PURE_INVOKE(compInt, curCosts[costAiHp], curGamestate->healthMeter[0], caseGamestate->healthMeter[0], maxHpDiff);
    compValue += PURE_INVOKE(compInt, curCosts[costEnemyHp], curGamestate->healthMeter[1], caseGamestate->healthMeter[1], maxHpDiff);
    //compValue += compCaseCooldown(caseData, framesActive) * costCaseCooldown;
    compValue += PURE_INVOKE(compCaseCooldown, curCosts[costCaseCooldown], caseData, framesActive);

    compValue +=  PURE_INVOKE(compRelativePosX, curCosts[costXDist], curGamestate->getPosX()[0], curGamestate->getPosX()[1], caseGamestate->getPosX()[0], caseGamestate->getPosX()[1]);
    //compRelativePosX(curGamestate->getPosX(), caseGamestate->getPosX()) * costXDist;
    compValue += PURE_INVOKE(compRelativePosY, curCosts[costYDist], curGamestate->getPosY()[0], curGamestate->getPosY()[1], caseGamestate->getPosY()[0], caseGamestate->getPosY()[1]);
    //compValue += compRelativePosY(curGamestate->getPosY(), caseGamestate->getPosY()) * costYDist;
    
    compValue += PURE_INVOKE(compMaxDistanceAttack, curCosts[costMinDistanceAttack], curGamestate->getPosX()[0], curGamestate->getPosX()[1], caseGamestate->getPosX()[0], caseGamestate->getPosX()[1], caseGamestate->hitMinX);
    //testvalue += PURE_INVOKE(compMaxDistanceAttack, curCosts[costMinDistanceAttack], curGamestate->getPosY()[0], curGamestate->getPosY()[1], caseGamestate->getPosY()[0], caseGamestate->getPosY()[1], caseGamestate->hitMinY);
    
    float testvalue = 0;
    testvalue += PURE_INVOKE(compStateHash, curCosts[costAiState], curGamestate->getCurrentActionHash()[0], caseGamestate->getCurrentActionHash()[0]);
    //compValue += compStateHash(curGamestate->getCurrentActionHash()[0], caseGamestate->getCurrentActionHash()[0]) * costAiState;
    if (testvalue != 0 && !curGamestate->getNeutral()[0]) {
        testvalue += curCosts[costNonNeutralState];
    } 
    compValue += testvalue;
    compValue += PURE_INVOKE(compStateHash, curCosts[costEnemyState], curGamestate->getCurrentActionHash()[1], caseGamestate->getCurrentActionHash()[1]);
    //compValue += compStateHash(curGamestate->getCurrentActionHash()[1], caseGamestate->getCurrentActionHash()[1]) * costEnemyState;
    compValue += PURE_INVOKE(compStateHash, curCosts[costlastAiState], curGamestate->getLastActionHash()[0], caseGamestate->getLastActionHash()[0]);
    //compValue += compStateHash(curGamestate->getLastActionHash()[0], caseGamestate->getLastActionHash()[0]) * costlastAiState;
    compValue += PURE_INVOKE(compStateHash, curCosts[costlastEnemyState], curGamestate->getLastActionHash()[1], caseGamestate->getLastActionHash()[1]);
    //compValue += compStateHash(curGamestate->getLastActionHash()[1], caseGamestate->getLastActionHash()[1]) * costlastEnemyState;
    compValue += PURE_INVOKE(compNeutralState, curCosts[costAiNeutral], curGamestate->getNeutral()[0], caseGamestate->getNeutral()[0]);
    //compValue += compNeutralState(curGamestate->getNeutral()[0], caseGamestate->getNeutral()[0]) * costAiNeutral;
    compValue += PURE_INVOKE(compNeutralState, curCosts[costEnemyNeutral], curGamestate->getNeutral()[1], caseGamestate->getNeutral()[1]);
   //compValue += compNeutralState(curGamestate->getNeutral()[1], caseGamestate->getNeutral()[1]) * costEnemyNeutral;
    compValue += PURE_INVOKE(compBool, curCosts[costAiAttack], curGamestate->getAttack()[0], caseGamestate->getAttack()[0]);
    //compValue += compBool(curGamestate->getAttack()[0], caseGamestate->getAttack()[0]) * costAiAttack;
    compValue += PURE_INVOKE(compBool, curCosts[costEnemyAttack], curGamestate->getAttack()[1], caseGamestate->getAttack()[1]);
    //compValue += compBool(curGamestate->getAttack()[1], caseGamestate->getAttack()[1]) * costEnemyAttack;
    compValue += PURE_INVOKE(compAirborneState, curCosts[costAiAir], curGamestate->getAir()[0], caseGamestate->getAir()[0]);
    //compValue += compAirborneState(curGamestate->getAir()[0], caseGamestate->getAir()[0]) * costAiAir;
    compValue += PURE_INVOKE(compAirborneState, curCosts[costEnemyAir], curGamestate->getAir()[1], caseGamestate->getAir()[1]);
    //compValue += compAirborneState(curGamestate->getAir()[1], caseGamestate->getAir()[1]) * costEnemyAir;
    compValue += PURE_INVOKE(compWakeupState, curCosts[costAiWakeup], curGamestate->getWakeup()[0], caseGamestate->getWakeup()[0]);
    //compValue += compWakeupState(curGamestate->getWakeup()[0], caseGamestate->getWakeup()[0]) * costAiWakeup;
    compValue += PURE_INVOKE(compWakeupState, curCosts[costEnemyWakeup], curGamestate->getWakeup()[1], caseGamestate->getWakeup()[1]);
    //compValue += compWakeupState(curGamestate->getWakeup()[1], caseGamestate->getWakeup()[1]) * costEnemyWakeup;
    compValue += PURE_INVOKE(compBlockState, curCosts[costAiBlocking], curGamestate->getBlocking()[0], caseGamestate->getBlocking()[0]);
    //compValue += compBlockState(curGamestate->getBlocking()[0], caseGamestate->getBlocking()[0]) * costAiBlocking;
    compValue += PURE_INVOKE(compBlockStun, curCosts[costEnemyBlocking], curGamestate->getBlocking()[1], caseGamestate->getBlocking()[1], curGamestate->getBlockstun()[1], caseGamestate->getBlockstun()[1]);
    //compValue += compBlockState(curGamestate->getBlocking()[1], caseGamestate->getBlocking()[1]) * costEnemyBlocking;
    compValue += PURE_INVOKE(compHitState, curCosts[costAiHit], curGamestate->getHit()[0], caseGamestate->getHit()[0]);
    //compValue += compHitState(curGamestate->getHit()[0], caseGamestate->getHit()[0]) * costAiHit;
    compValue += PURE_INVOKE(compHitState, curCosts[costEnemyHit], curGamestate->getHit()[1], caseGamestate->getHit()[1]);
    //compValue += compHitState(curGamestate->getHit()[1], caseGamestate->getHit()[1]) * costEnemyHit;
    compValue += PURE_INVOKE(compGetHitThisFrameState, curCosts[costAiHitThisFrame], curGamestate->getHitThisFrame()[0], caseGamestate->getHitThisFrame()[0]);
    //compValue += compGetHitThisFrameState(curGamestate->getHitThisFrame()[0], caseGamestate->getHitThisFrame()[0]) * costAiHitThisFrame;
    compValue += PURE_INVOKE(compGetHitThisFrameState, curCosts[costEnemyHitThisFrame], curGamestate->getHitThisFrame()[1], caseGamestate->getHitThisFrame()[1]);
    //compValue += compGetHitThisFrameState(curGamestate->getHitThisFrame()[1], caseGamestate->getHitThisFrame()[1]) * costEnemyHitThisFrame;
    compValue += PURE_INVOKE(compBlockingThisFrameState, curCosts[costAiBlockThisFrame], curGamestate->getBlockThisFrame()[0], caseGamestate->getBlockThisFrame()[0]);
    //compValue += compBlockingThisFrameState(curGamestate->getBlockThisFrame()[0], caseGamestate->getBlockThisFrame()[0]) * costAiBlockThisFrame;
    compValue += PURE_INVOKE(compBlockingThisFrameState, curCosts[costEnemyBlockhisFrame], curGamestate->getBlockThisFrame()[1], caseGamestate->getBlockThisFrame()[1]);
    //compValue += compBlockingThisFrameState(curGamestate->getBlockThisFrame()[1], caseGamestate->getBlockThisFrame()[1]) * costEnemyBlockhisFrame;
    compValue += PURE_INVOKE(compCrouching, curCosts[costAiCrouching], curGamestate->getCrouching()[0], caseGamestate->getCrouching()[0]);
    //compValue += compCrouching(curGamestate->getCrouching()[0], caseGamestate->getCrouching()[0]) * costAiCrouching;
    compValue += PURE_INVOKE(compCrouching, curCosts[costEnemyCrouching], curGamestate->getCrouching()[1], caseGamestate->getCrouching()[1]);
    //compValue += compCrouching(curGamestate->getCrouching()[1], caseGamestate->getCrouching()[1]) * costEnemyCrouching;
    compValue += PURE_INVOKE(compIntState, curCosts[costMatchState], curGamestate->matchState, caseGamestate->matchState);
    //compValue += compIntState(curGamestate->matchState, caseGamestate->matchState) * costMatchState;
    compValue += PURE_INVOKE(compBool, curCosts[costAiOverdriveState], curGamestate->overdriveTimeleft[0] > 0, caseGamestate->overdriveTimeleft[0] > 0);
    //compValue += compBool(curGamestate->overdriveTimeleft[0] > 0, caseGamestate->overdriveTimeleft[0] > 0) * costAiOverdriveState;
    compValue += PURE_INVOKE(compBool, curCosts[costEnemyOverdriveState], curGamestate->overdriveTimeleft[1] > 0, caseGamestate->overdriveTimeleft[1] > 0);
    //compValue += compBool(curGamestate->overdriveTimeleft[1] > 0, caseGamestate->overdriveTimeleft[1] > 0) * costEnemyOverdriveState;



    if (curGamestate->getHit()[0] == true) {
        compValue += PURE_INVOKE(compInt, curCosts[costAiProration], curGamestate->getComboProration()[0], caseGamestate->getComboProration()[0], maxProration);
        //compValue += compInt(curGamestate->getComboProration()[0], caseGamestate->getComboProration()[0], maxProration) * costAiProration;
        compValue += PURE_INVOKE(compIntState, curCosts[costAiStarterRating], curGamestate->getStarterRating()[0], caseGamestate->getStarterRating()[0]);
        //compValue += compIntState(curGamestate->getStarterRating()[0], caseGamestate->getStarterRating()[0]) * costAiStarterRating;
        compValue += PURE_INVOKE(compInt, curCosts[costAiComboTime], curGamestate->getComboTime()[0], caseGamestate->getComboTime()[0], maxComboTime);
        //compValue += compInt(curGamestate->getComboTime()[0], caseGamestate->getComboTime()[0], maxComboTime) * costAiComboTime;

    }
    if (curGamestate->getHit()[1] == true) {
        compValue += PURE_INVOKE(compInt, curCosts[costEnemyProration], curGamestate->getComboProration()[1], caseGamestate->getComboProration()[1], maxProration);
        //compValue += compInt(curGamestate->getComboProration()[1], caseGamestate->getComboProration()[1], maxProration) * costEnemyProration;
        compValue += PURE_INVOKE(compIntState, curCosts[costEnemyStarterRating], curGamestate->getStarterRating()[1], caseGamestate->getStarterRating()[1]);
        //compValue += compIntState(curGamestate->getStarterRating()[1], caseGamestate->getStarterRating()[1]) * costEnemyStarterRating;
        compValue += PURE_INVOKE(compInt, curCosts[costEnemyComboTime], curGamestate->getComboTime()[1], caseGamestate->getComboTime()[1], maxComboTime);
        //compValue += compInt(curGamestate->getComboTime()[1], caseGamestate->getComboTime()[1], maxComboTime) * costEnemyComboTime;
        compValue += PURE_INVOKE(compDistanceToWall, curCosts[costWallDistCombo], curGamestate->getPosX()[0], caseGamestate->getPosX()[0], curGamestate->getFacing(), caseGamestate->getFacing());
        //compValue += compDistanceToWall(curGamestate->getPosX(), caseGamestate->getPosX(), curGamestate->getFacing(), caseGamestate->getFacing()) * costWallDistCombo;
    }
    else {
        compValue += PURE_INVOKE(compDistanceToWall, curCosts[costWallDist], curGamestate->getPosX()[0], caseGamestate->getPosX()[0], curGamestate->getFacing(), caseGamestate->getFacing());
        //compValue += compDistanceToWall(curGamestate->getPosX(), caseGamestate->getPosX(), curGamestate->getFacing(), caseGamestate->getFacing()) * costWallDist;
    }
 
    switch (caseReplay.getCharIds()[0])
    {
    case 21://bullet
        compValue += PURE_INVOKE(compInt, curCosts[costBuHeat], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 2);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 2) * costBuHeat; //heat
        break;
    case 22://Azrael
        compValue += PURE_INVOKE(compBool, curCosts[costAzTopWeak], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costAzBotWeak], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costAzFireball], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compIntState(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.3;
        break;
    case 6://litchi
        if (curGamestate->CharSpecific1[0] == 17 || caseGamestate->CharSpecific1[0] == 17) {
            compValue += PURE_INVOKE(compIntState, curCosts[costLcStaff], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
            //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * costLcStaff;
        }
        else {
            compValue += PURE_INVOKE(compIntState, curCosts[costLcStaff], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
            //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * costLcStaffActive;
        }
        break;
    case 3://rachel
        if (curGamestate->CharSpecific1[0] < rachelWindMin) {
            compValue += PURE_INVOKE(compInt, curCosts[costLowWind], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], rachelWindMax);
            //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], rachelWindMax) * costLowWind;
        }
        else {
            compValue += PURE_INVOKE(compInt, curCosts[costWind], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], rachelWindMax);
            //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], rachelWindMax) * costWind;
        }
        break;
    case 5://Tager//tagerSpecific
        compValue += PURE_INVOKE(compBool, curCosts[costTagerSpark], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costTagerMagnetism], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.2;
        break;
    case 33://ES
        compValue += PURE_INVOKE(compBool, curCosts[costEsBuff], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.2;
        break;
    case 11://Nu
        compValue += PURE_INVOKE(compBool, curCosts[costNuGravity], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.1;
        break;
    case 27://Lambda
        compValue += PURE_INVOKE(compBool, curCosts[costNuGravity], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.1;
        break;
    case 13://Hazama
        compValue += PURE_INVOKE(compInt, curCosts[costHazChains], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 2);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 2) * 0.2;
        break;
    case 26://Celica
        compValue += PURE_INVOKE(compBool, curCosts[costCelAnyblueLife], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0] >0, caseGamestate->CharSpecific1[0] > 0) * 0.2;
        compValue += PURE_INVOKE(compInt, curCosts[costCelBlueAmount], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000) * 1;
        break;
    case 16://Valk
        compValue += PURE_INVOKE(compInt, curCosts[costValWolfMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costValWolfState], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compIntState(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 1;
        break;
    case 17://Plat
        compValue += PURE_INVOKE(compIntState, curCosts[costPlatCurItem], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.3;
        compValue += PURE_INVOKE(compInt, curCosts[costPlatNextItem], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0], 5);
        //compValue += compInt(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0],5) * 0.01;
        compValue += PURE_INVOKE(compIntState, curCosts[costPlatItemNr], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compIntState(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.05;
        break;
    case 18://Relius
        compValue += PURE_INVOKE(compInt, curCosts[costReliusDollMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000) * 0.5;//Doll meter
        compValue += PURE_INVOKE(compIntState, curCosts[costReliusDollState], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compIntState(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.1; //Doll State
        compValue += PURE_INVOKE(compBool, curCosts[costReliusDollInactive], curGamestate->CharSpecific2[0] != 0, caseGamestate->CharSpecific2[0] != 0);
        //compValue += compBool(curGamestate->CharSpecific2[0]!=0, caseGamestate->CharSpecific2[0] != 0) * 0.6; //Doll State
        compValue += PURE_INVOKE(compBool, curCosts[costReliusDollCooldown], curGamestate->CharSpecific3[0] != 0, caseGamestate->CharSpecific3[0] != 0);
        //compValue += compBool(curGamestate->CharSpecific3[0] != 0, caseGamestate->CharSpecific3[0] != 0) * 1; //Doll Cooldown
        break;
    case 32://Susanoo
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock1], curGamestate->CharSpecific1[0] > 0, caseGamestate->CharSpecific1[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific1[0]>0, caseGamestate->CharSpecific1[0] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock1Lvl], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock2], curGamestate->CharSpecific2[0] > 0, caseGamestate->CharSpecific2[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific2[0] > 0, caseGamestate->CharSpecific2[0] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock2Lvl], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compIntState(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock3], curGamestate->CharSpecific3[0] > 0, caseGamestate->CharSpecific3[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific3[0] > 0, caseGamestate->CharSpecific3[0] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock3Lvl], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compIntState(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock4], curGamestate->CharSpecific4[0] > 0, caseGamestate->CharSpecific4[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific4[0] > 0, caseGamestate->CharSpecific4[0] > 0) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock4Lvl], curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]);
        //compValue += compIntState(curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock5], curGamestate->CharSpecific5[0] > 0, caseGamestate->CharSpecific5[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific5[0] > 0, caseGamestate->CharSpecific5[0] > 0) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock5Lvl], curGamestate->CharSpecific5[0], caseGamestate->CharSpecific5[0]);
        //compValue += compIntState(curGamestate->CharSpecific5[0], caseGamestate->CharSpecific5[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock6], curGamestate->CharSpecific6[0] > 0, caseGamestate->CharSpecific6[0]);
        //compValue += compBool(curGamestate->CharSpecific6[0] > 0, caseGamestate->CharSpecific6[0] > 0) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock6Lvl], curGamestate->CharSpecific6[0], caseGamestate->CharSpecific6[0]);
        //compValue += compIntState(curGamestate->CharSpecific6[0], caseGamestate->CharSpecific6[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock7], curGamestate->CharSpecific7[0] > 0, caseGamestate->CharSpecific7[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific7[0] > 0, caseGamestate->CharSpecific7[0] > 0) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock7Lvl], curGamestate->CharSpecific7[0], caseGamestate->CharSpecific7[0]);
        //compValue += compIntState(curGamestate->CharSpecific7[0], caseGamestate->CharSpecific7[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock8], curGamestate->CharSpecific8[0] > 0, caseGamestate->CharSpecific8[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific8[0] > 0, caseGamestate->CharSpecific8[0] > 0) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock8Lvl], curGamestate->CharSpecific8[0], caseGamestate->CharSpecific8[0]);
        //compValue += compIntState(curGamestate->CharSpecific8[0], caseGamestate->CharSpecific8[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock9], curGamestate->CharSpecific9[0], caseGamestate->CharSpecific9[0]);
        //compValue += compIntState(curGamestate->CharSpecific9[0], caseGamestate->CharSpecific9[0]) * 0.1;
        break;
    case 35://Jubei
        compValue += PURE_INVOKE(compBool, curCosts[costJubeiBuff], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.3;
        compValue += PURE_INVOKE(compBool, curCosts[costJubeiMark], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.1;
        break;
    case 31://Izanami
        compValue += PURE_INVOKE(compBool, curCosts[costIzanamiFloating], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.2;
        compValue += PURE_INVOKE(compBool, curCosts[costIzanamiShotCooldown], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.2;
        compValue += PURE_INVOKE(compBool, curCosts[costIzanamiRibcage], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compBool(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.2;
        compValue += PURE_INVOKE(compBool, curCosts[costIzanamiBitsStance], curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]);
        //compValue += compBool(curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]) * 0.2;
        break;
    case 29://Nine
        compValue += PURE_INVOKE(compIntState, curCosts[costNineSpell], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.3;
        compValue += PURE_INVOKE(compIntState, curCosts[costNineSpellBackup], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compIntState(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costNineSlots], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compIntState(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costNineSlotsBackup], curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]);
        //compValue += compIntState(curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]) * 0.05;
        break;
    case 9://Carl
        compValue += PURE_INVOKE(compBool, curCosts[costCarlDollInactive], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 1;
        compValue += PURE_INVOKE(compInt, curCosts[costCarlDollMeter], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0], 5000);
        //compValue += compInt(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0], 5000) * 0.1;
        break;
    case 12://Tsubaki
        compValue += PURE_INVOKE(compInt, curCosts[costTsubakiMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 5);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0],5) * 1;
        break;
    case 24://Kokonoe
        compValue += PURE_INVOKE(compInt, curCosts[costKokoGravitronNr], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 8);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 8) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costKokoAnyGraviLeft], curGamestate->CharSpecific1[0] > 0, caseGamestate->CharSpecific1[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific1[0] > 0, caseGamestate->CharSpecific1[0] > 0) * 0.3;
        break;
    case 19://Izayoi
        compValue += PURE_INVOKE(compBool, curCosts[costIzayoiState], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.3;
        compValue += PURE_INVOKE(compInt, curCosts[costIzayoiStocks], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0], 8);
        //compValue += compInt(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0],8) * 0.8;
        compValue += PURE_INVOKE(compBool, curCosts[costIzayoiSupermode], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compBool(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.4;
        break;
    case 7://Arakune
        compValue += PURE_INVOKE(compInt, curCosts[costArakuneCurseMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 6000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 6000) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costArakuneCurseActive], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.8;
        break;
    case 8://Bang
        compValue += PURE_INVOKE(compBool, curCosts[costBangFRKZ1], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costBangFRKZ2], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costBangFRKZ3], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compBool(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costBangFRKZ4], curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]);
        //compValue += compBool(curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]) * 0.05;
        compValue += PURE_INVOKE(compInt, curCosts[costBangNailcount], curGamestate->CharSpecific5[0], caseGamestate->CharSpecific5[0], 12);
        //compValue += compInt(curGamestate->CharSpecific5[0], caseGamestate->CharSpecific5[0], 12) * 0.4;
        break;
    case 20://Amane
        //Amane
        compValue += PURE_INVOKE(compInt, curCosts[costAmaneDrillMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 6000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0],6000) * 0.4;
        compValue += PURE_INVOKE(compBool, curCosts[costAmaneDrillOverheat], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.2;
        break;
    default:
        break;
    }
    switch (caseReplay.getCharIds()[1])
    {
    case 22://Azrael
        compValue += PURE_INVOKE(compBool, curCosts[costAzTopWeakEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costAzBotWeakEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costAzFireballEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compIntState(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.3;
        break;
    case 5://Tager
        compValue += PURE_INVOKE(compBool, curCosts[costTagerSparkEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costTagerMagnetismEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.2;
        break;
    case 13://Hazama
        compValue += PURE_INVOKE(compInt, curCosts[costHazChainsEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 2);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 2) * 0.2;
        break;
    case 16://Valk
        compValue += PURE_INVOKE(compInt, curCosts[costValWolfMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 10000) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costValWolfStateEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compIntState(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 1;
        break;
    case 17://Plat
        compValue += PURE_INVOKE(compIntState, curCosts[costPlatCurItemEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compIntState(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.3;
        //compValue += PURE_INVOKE(compInt, curCosts[0.01, curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 5);
        //compValue += compInt(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 5) * 0.01;
        //compValue += PURE_INVOKE(compIntState, curCosts[0.05, curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compIntState(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.05;
        break;
    case 18://Relius
        compValue += PURE_INVOKE(compInt, curCosts[costReliusDollMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 10000) * 0.1;//Doll meter
        compValue += PURE_INVOKE(compIntState, curCosts[costReliusDollStateEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compIntState(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1; //Doll State
        compValue += PURE_INVOKE(compBool, curCosts[costReliusDollInactiveEnemy], curGamestate->CharSpecific2[1] != 0, caseGamestate->CharSpecific2[1] != 0);
        //compValue += compBool(curGamestate->CharSpecific2[1] != 0, caseGamestate->CharSpecific2[1] != 0) * 0.6; //Doll State
        compValue += PURE_INVOKE(compBool, curCosts[costReliusDollCooldownEnemy], curGamestate->CharSpecific3[1] != 0, caseGamestate->CharSpecific3[1] != 0);
        //compValue += compBool(curGamestate->CharSpecific3[1] != 0, caseGamestate->CharSpecific3[1] != 0) * 1; //Doll Cooldown
        break;
    case 32://Susanoo
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock1Enemy], curGamestate->CharSpecific1[1] > 0, caseGamestate->CharSpecific1[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific1[1] > 0, caseGamestate->CharSpecific1[1] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock1LvlEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compIntState(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock2Enemy], curGamestate->CharSpecific2[1] > 0, caseGamestate->CharSpecific2[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific2[1] > 0, caseGamestate->CharSpecific2[1] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock2LvlEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compIntState(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock3Enemy], curGamestate->CharSpecific3[1] > 0, caseGamestate->CharSpecific3[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific3[1] > 0, caseGamestate->CharSpecific3[1] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock3LvlEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compIntState(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock4Enemy], curGamestate->CharSpecific4[1] > 0, caseGamestate->CharSpecific4[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific4[1] > 0, caseGamestate->CharSpecific4[1] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock4LvlEnemy], curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]);
        //compValue += compIntState(curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock5Enemy], curGamestate->CharSpecific5[1] > 0, caseGamestate->CharSpecific5[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific5[1] > 0, caseGamestate->CharSpecific5[1] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock5LvlEnemy], curGamestate->CharSpecific5[1], caseGamestate->CharSpecific5[1]);
        //compValue += compIntState(curGamestate->CharSpecific5[1], caseGamestate->CharSpecific5[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock6Enemy], curGamestate->CharSpecific6[1] > 0, caseGamestate->CharSpecific6[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific6[1] > 0, caseGamestate->CharSpecific6[1] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock6LvlEnemy], curGamestate->CharSpecific6[1], caseGamestate->CharSpecific6[1]);
        //compValue += compIntState(curGamestate->CharSpecific6[1], caseGamestate->CharSpecific6[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock7Enemy], curGamestate->CharSpecific7[1] > 0, caseGamestate->CharSpecific7[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific7[1] > 0, caseGamestate->CharSpecific7[1] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock7LvlEnemy], curGamestate->CharSpecific7[1], caseGamestate->CharSpecific7[1]);
        //compValue += compIntState(curGamestate->CharSpecific7[1], caseGamestate->CharSpecific7[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock8Enemy], curGamestate->CharSpecific8[1] > 0, caseGamestate->CharSpecific8[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific8[1] > 0, caseGamestate->CharSpecific8[1] > 0) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costSusanUnlock8LvlEnemy], curGamestate->CharSpecific8[1], caseGamestate->CharSpecific8[1]);
        //compValue += compIntState(curGamestate->CharSpecific8[1], caseGamestate->CharSpecific8[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costSusanUnlock9Enemy], curGamestate->CharSpecific9[0], caseGamestate->CharSpecific9[0]);
        break;
    case 35://Jubei
        compValue += PURE_INVOKE(compBool, curCosts[costJubeiBuffEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.3;//Buff
        compValue += PURE_INVOKE(compBool, curCosts[costJubeiMarkEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1;//Mark
        break;
    case 31://Izanami
        compValue += PURE_INVOKE(compBool, curCosts[costIzanamiFloatingEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.2;
        compValue += PURE_INVOKE(compBool, curCosts[costIzanamiRibcageEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 1;
        compValue += PURE_INVOKE(compBool, curCosts[costIzanamiShotCooldownEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compBool(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.2;
        compValue += PURE_INVOKE(compBool, curCosts[costIzanamiBitsStanceEnemy], curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]);
        //compValue += compBool(curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]) * 0.2;
        break;
    case 29://Nine
        compValue += PURE_INVOKE(compIntState, curCosts[costNineSpellEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compIntState(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.3;
        compValue += PURE_INVOKE(compIntState, curCosts[costNineSpellBackupEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compIntState(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costNineSlotsEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compIntState(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.1;
        compValue += PURE_INVOKE(compIntState, curCosts[costNineSlotsBackupEnemy], curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]);
        //compValue += compIntState(curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]) * 0.05;
        break;
    case 9://Carl
        compValue += PURE_INVOKE(compBool, curCosts[costCarlDollInactiveEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 1;
        compValue += PURE_INVOKE(compInt, curCosts[costCarlDollMeterEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 5000);
        //compValue += compInt(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 5000) * 0.1;
        break;
    case 12://Tsubaki
        compValue += PURE_INVOKE(compInt, curCosts[costTsubakiMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 5);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 5) * 0.2;
        break;
    case 24://Kokonoe
        compValue += PURE_INVOKE(compInt, curCosts[costKokoGravitronNrEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 8);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 8) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costKokoAnyGraviLeft], curGamestate->CharSpecific1[1] > 0, caseGamestate->CharSpecific1[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific1[1]>0, caseGamestate->CharSpecific1[1]>0) * 0.1;
        break;
    case 19://Izayoi
        compValue += PURE_INVOKE(compBool, curCosts[costIzayoiStateEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.1;
        compValue += PURE_INVOKE(compInt, curCosts[costIzayoiStocksEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 8);
        //compValue += compInt(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 8) * 0.2;
        compValue += PURE_INVOKE(compBool, curCosts[costIzayoiSupermodeEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compBool(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.1;
        break;
    case 7://Arakune
        compValue += PURE_INVOKE(compInt, curCosts[costArakuneCurseMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 6000);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1],6000) * 0.1;
        compValue += PURE_INVOKE(compBool, curCosts[costArakuneCurseActiveEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.8;
        break;
    case 8://Bang
        compValue += PURE_INVOKE(compBool, curCosts[costBangFRKZ1Enemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1] , caseGamestate->CharSpecific1[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costBangFRKZ2Enemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costBangFRKZ3Enemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compBool(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.05;
        compValue += PURE_INVOKE(compBool, curCosts[costBangFRKZ4Enemy], curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]);
        //compValue += compBool(curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]) * 0.05;
        compValue += PURE_INVOKE(compInt, curCosts[costBangNailcount], curGamestate->CharSpecific5[1], caseGamestate->CharSpecific5[1], 12);
        //compValue += compInt(curGamestate->CharSpecific5[1], caseGamestate->CharSpecific5[1], 12) * 0.1;
        break;
    case 20://Amane
        compValue += PURE_INVOKE(compInt, curCosts[costAmaneDrillMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 6000);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 6000) * 0.4;
        compValue += PURE_INVOKE(compBool, curCosts[costAmaneDrillOverheatEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1;
        break;
    default:
        break;
    }
    return compValue;

}	

float CbrData::comparisonFunctionDebug(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int replayIndex, int caseIndex, bool nextCaseCheck) {
    float compValue = 0;
    bool buffVal = false;
    if (!nextCaseCheck) {
        buffVal = sameOrNextCaseCheck(curGamestate, replayIndex, caseIndex);
        if (buffVal) {
            debugText += "sameOrNextCaseCheck Fail\n";
            buffVal = false;
            compValue += 999;
        }
        buffVal = inputSequenceCheck(curGamestate, replayIndex, caseIndex);
        if (buffVal) {
            debugText += "inputSequenceCheck Fail\n";
            buffVal = false;
            compValue += 999;
        }
    }
    buffVal = meterCheck(curGamestate, replayIndex, caseIndex);
    if (buffVal) {
        debugText += "meterCheck Fail\n";
        buffVal = false;
        compValue += 999;
    }
    buffVal = OdCheck(curGamestate, replayIndex, caseIndex);
    if (buffVal) {
        debugText += "OdCheck Fail\n";
        buffVal = false;
        compValue += 999;
    }
    if (compValue > 0) {
        return compValue;
    }

    compValue += REFLECT_INVOKE(compInt, curCosts[costAiHp], curGamestate->healthMeter[0], caseGamestate->healthMeter[0], maxHpDiff);
    compValue += REFLECT_INVOKE(compInt, curCosts[costEnemyHp], curGamestate->healthMeter[1], caseGamestate->healthMeter[1], maxHpDiff);
    //compValue += compCaseCooldown(caseData, framesActive) * costCaseCooldown;
    compValue += REFLECT_INVOKE(compCaseCooldown, curCosts[costCaseCooldown], caseData, framesActive);

    compValue += REFLECT_INVOKE(compRelativePosX, curCosts[costXDist], curGamestate->getPosX()[0], curGamestate->getPosX()[1], caseGamestate->getPosX()[0], caseGamestate->getPosX()[1]);
    //compRelativePosX(curGamestate->getPosX(), caseGamestate->getPosX()) * costXDist;
    compValue += REFLECT_INVOKE(compRelativePosY, curCosts[costYDist], curGamestate->getPosY()[0], curGamestate->getPosY()[1], caseGamestate->getPosY()[0], caseGamestate->getPosY()[1]);
    //compValue += compRelativePosY(curGamestate->getPosY(), caseGamestate->getPosY()) * costYDist;
    compValue += REFLECT_INVOKE(compMaxDistanceAttack, curCosts[costMinDistanceAttack], curGamestate->getPosX()[0], curGamestate->getPosX()[1], caseGamestate->getPosX()[0], caseGamestate->getPosX()[1], caseGamestate->hitMinX);
    //compValue += REFLECT_INVOKE(compMaxDistanceAttack, curCosts[costMinDistanceAttack], curGamestate->getPosY()[0], curGamestate->getPosY()[1], caseGamestate->getPosY()[0], caseGamestate->getPosY()[1], caseGamestate->hitMinY);
    float testvalue = 0;
    testvalue += REFLECT_INVOKE(compState, curCosts[costAiState], curGamestate->getCurrentAction()[0], caseGamestate->getCurrentAction()[0]);
    //compValue += compStateHash(curGamestate->getCurrentActionHash()[0], caseGamestate->getCurrentActionHash()[0]) * costAiState;
    if (testvalue != 0 && !curGamestate->getNeutral()[0]) {
        testvalue += curCosts[costNonNeutralState];
    }
    compValue += testvalue;
    //compValue += compStateHash(curGamestate->getCurrentActionHash()[0], caseGamestate->getCurrentActionHash()[0]) * costAiState;
    compValue += REFLECT_INVOKE(compState, curCosts[costEnemyState], curGamestate->getCurrentAction()[1], caseGamestate->getCurrentAction()[1]);
    //compValue += compStateHash(curGamestate->getCurrentActionHash()[1], caseGamestate->getCurrentActionHash()[1]) * costEnemyState;
    compValue += REFLECT_INVOKE(compState, curCosts[costlastAiState], curGamestate->getLastAction()[0], caseGamestate->getLastAction()[0]);
    //compValue += compStateHash(curGamestate->getLastActionHash()[0], caseGamestate->getLastActionHash()[0]) * costlastAiState;
    compValue += REFLECT_INVOKE(compState, curCosts[costlastEnemyState], curGamestate->getLastAction()[1], caseGamestate->getLastAction()[1]);
    //compValue += compStateHash(curGamestate->getLastActionHash()[1], caseGamestate->getLastActionHash()[1]) * costlastEnemyState;
    compValue += REFLECT_INVOKE(compNeutralState, curCosts[costAiNeutral], curGamestate->getNeutral()[0], caseGamestate->getNeutral()[0]);
    //compValue += compNeutralState(curGamestate->getNeutral()[0], caseGamestate->getNeutral()[0]) * costAiNeutral;
    compValue += REFLECT_INVOKE(compNeutralState, curCosts[costEnemyNeutral], curGamestate->getNeutral()[1], caseGamestate->getNeutral()[1]);
    //compValue += compNeutralState(curGamestate->getNeutral()[1], caseGamestate->getNeutral()[1]) * costEnemyNeutral;
    compValue += REFLECT_INVOKE(compBool, curCosts[costAiAttack], curGamestate->getAttack()[0], caseGamestate->getAttack()[0]);
    //compValue += compBool(curGamestate->getAttack()[0], caseGamestate->getAttack()[0]) * costAiAttack;
    compValue += REFLECT_INVOKE(compBool, curCosts[costEnemyAttack], curGamestate->getAttack()[1], caseGamestate->getAttack()[1]);
    //compValue += compBool(curGamestate->getAttack()[1], caseGamestate->getAttack()[1]) * costEnemyAttack;
    compValue += REFLECT_INVOKE(compAirborneState, curCosts[costAiAir], curGamestate->getAir()[0], caseGamestate->getAir()[0]);
    //compValue += compAirborneState(curGamestate->getAir()[0], caseGamestate->getAir()[0]) * costAiAir;
    compValue += REFLECT_INVOKE(compAirborneState, curCosts[costEnemyAir], curGamestate->getAir()[1], caseGamestate->getAir()[1]);
    //compValue += compAirborneState(curGamestate->getAir()[1], caseGamestate->getAir()[1]) * costEnemyAir;
    compValue += REFLECT_INVOKE(compWakeupState, curCosts[costAiWakeup], curGamestate->getWakeup()[0], caseGamestate->getWakeup()[0]);
    //compValue += compWakeupState(curGamestate->getWakeup()[0], caseGamestate->getWakeup()[0]) * costAiWakeup;
    compValue += REFLECT_INVOKE(compWakeupState, curCosts[costEnemyWakeup], curGamestate->getWakeup()[1], caseGamestate->getWakeup()[1]);
    //compValue += compWakeupState(curGamestate->getWakeup()[1], caseGamestate->getWakeup()[1]) * costEnemyWakeup;
    compValue += REFLECT_INVOKE(compBlockState, curCosts[costAiBlocking], curGamestate->getBlocking()[0], caseGamestate->getBlocking()[0]);
    //compValue += compBlockState(curGamestate->getBlocking()[0], caseGamestate->getBlocking()[0]) * costAiBlocking;
    compValue += REFLECT_INVOKE(compBlockStun, curCosts[costEnemyBlocking], curGamestate->getBlocking()[1], caseGamestate->getBlocking()[1], curGamestate->getBlockstun()[1], caseGamestate->getBlockstun()[1]);
    //compValue += compBlockState(curGamestate->getBlocking()[1], caseGamestate->getBlocking()[1]) * costEnemyBlocking;
    compValue += REFLECT_INVOKE(compHitState, curCosts[costAiHit], curGamestate->getHit()[0], caseGamestate->getHit()[0]);
    //compValue += compHitState(curGamestate->getHit()[0], caseGamestate->getHit()[0]) * costAiHit;
    compValue += REFLECT_INVOKE(compHitState, curCosts[costEnemyHit], curGamestate->getHit()[1], caseGamestate->getHit()[1]);
    //compValue += compHitState(curGamestate->getHit()[1], caseGamestate->getHit()[1]) * costEnemyHit;
    compValue += REFLECT_INVOKE(compGetHitThisFrameState, curCosts[costAiHitThisFrame], curGamestate->getHitThisFrame()[0], caseGamestate->getHitThisFrame()[0]);
    //compValue += compGetHitThisFrameState(curGamestate->getHitThisFrame()[0], caseGamestate->getHitThisFrame()[0]) * costAiHitThisFrame;
    compValue += REFLECT_INVOKE(compGetHitThisFrameState, curCosts[costEnemyHitThisFrame], curGamestate->getHitThisFrame()[1], caseGamestate->getHitThisFrame()[1]);
    //compValue += compGetHitThisFrameState(curGamestate->getHitThisFrame()[1], caseGamestate->getHitThisFrame()[1]) * costEnemyHitThisFrame;
    compValue += REFLECT_INVOKE(compBlockingThisFrameState, curCosts[costAiBlockThisFrame], curGamestate->getBlockThisFrame()[0], caseGamestate->getBlockThisFrame()[0]);
    //compValue += compBlockingThisFrameState(curGamestate->getBlockThisFrame()[0], caseGamestate->getBlockThisFrame()[0]) * costAiBlockThisFrame;
    compValue += REFLECT_INVOKE(compBlockingThisFrameState, curCosts[costEnemyBlockhisFrame], curGamestate->getBlockThisFrame()[1], caseGamestate->getBlockThisFrame()[1]);
    //compValue += compBlockingThisFrameState(curGamestate->getBlockThisFrame()[1], caseGamestate->getBlockThisFrame()[1]) * costEnemyBlockhisFrame;
    compValue += REFLECT_INVOKE(compCrouching, curCosts[costAiCrouching], curGamestate->getCrouching()[0], caseGamestate->getCrouching()[0]);
    //compValue += compCrouching(curGamestate->getCrouching()[0], caseGamestate->getCrouching()[0]) * costAiCrouching;
    compValue += REFLECT_INVOKE(compCrouching, curCosts[costEnemyCrouching], curGamestate->getCrouching()[1], caseGamestate->getCrouching()[1]);
    //compValue += compCrouching(curGamestate->getCrouching()[1], caseGamestate->getCrouching()[1]) * costEnemyCrouching;
    compValue += REFLECT_INVOKE(compIntState, curCosts[costMatchState], curGamestate->matchState, caseGamestate->matchState);
    //compValue += compIntState(curGamestate->matchState, caseGamestate->matchState) * costMatchState;
    compValue += REFLECT_INVOKE(compBool, curCosts[costAiOverdriveState], curGamestate->overdriveTimeleft[0] > 0, caseGamestate->overdriveTimeleft[0] > 0);
    //compValue += compBool(curGamestate->overdriveTimeleft[0] > 0, caseGamestate->overdriveTimeleft[0] > 0) * costAiOverdriveState;
    compValue += REFLECT_INVOKE(compBool, curCosts[costEnemyOverdriveState], curGamestate->overdriveTimeleft[1] > 0, caseGamestate->overdriveTimeleft[1] > 0);
    //compValue += compBool(curGamestate->overdriveTimeleft[1] > 0, caseGamestate->overdriveTimeleft[1] > 0) * costEnemyOverdriveState;



    if (curGamestate->getHit()[0] == true) {
        compValue += REFLECT_INVOKE(compInt, curCosts[costAiProration], curGamestate->getComboProration()[0], caseGamestate->getComboProration()[0], maxProration);
        //compValue += compInt(curGamestate->getComboProration()[0], caseGamestate->getComboProration()[0], maxProration) * costAiProration;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costAiStarterRating], curGamestate->getStarterRating()[0], caseGamestate->getStarterRating()[0]);
        //compValue += compIntState(curGamestate->getStarterRating()[0], caseGamestate->getStarterRating()[0]) * costAiStarterRating;
        compValue += REFLECT_INVOKE(compInt, curCosts[costAiComboTime], curGamestate->getComboTime()[0], caseGamestate->getComboTime()[0], maxComboTime);
        //compValue += compInt(curGamestate->getComboTime()[0], caseGamestate->getComboTime()[0], maxComboTime) * costAiComboTime;

    }
    if (curGamestate->getHit()[1] == true) {
        compValue += REFLECT_INVOKE(compInt, curCosts[costEnemyProration], curGamestate->getComboProration()[1], caseGamestate->getComboProration()[1], maxProration);
        //compValue += compInt(curGamestate->getComboProration()[1], caseGamestate->getComboProration()[1], maxProration) * costEnemyProration;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costEnemyStarterRating], curGamestate->getStarterRating()[1], caseGamestate->getStarterRating()[1]);
        //compValue += compIntState(curGamestate->getStarterRating()[1], caseGamestate->getStarterRating()[1]) * costEnemyStarterRating;
        compValue += REFLECT_INVOKE(compInt, curCosts[costEnemyComboTime], curGamestate->getComboTime()[1], caseGamestate->getComboTime()[1], maxComboTime);
        //compValue += compInt(curGamestate->getComboTime()[1], caseGamestate->getComboTime()[1], maxComboTime) * costEnemyComboTime;
        compValue += REFLECT_INVOKE(compDistanceToWall, curCosts[costWallDistCombo], curGamestate->getPosX()[0], caseGamestate->getPosX()[0], curGamestate->getFacing(), caseGamestate->getFacing());
        //compValue += compDistanceToWall(curGamestate->getPosX(), caseGamestate->getPosX(), curGamestate->getFacing(), caseGamestate->getFacing()) * costWallDistCombo;
    }
    else {
        compValue += REFLECT_INVOKE(compDistanceToWall, curCosts[costWallDist], curGamestate->getPosX()[0], caseGamestate->getPosX()[0], curGamestate->getFacing(), caseGamestate->getFacing());
        //compValue += compDistanceToWall(curGamestate->getPosX(), caseGamestate->getPosX(), curGamestate->getFacing(), caseGamestate->getFacing()) * costWallDist;
    }

    switch (caseReplay.getCharIds()[0])
    {
    case 21://bullet
        compValue += REFLECT_INVOKE(compInt, curCosts[costBuHeat], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 2);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 2) * costBuHeat; //heat
        break;
    case 22://Azrael
        compValue += REFLECT_INVOKE(compBool, curCosts[costAzTopWeak], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costAzBotWeak], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costAzFireball], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compIntState(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.3;
        break;
    case 6://litchi
        if (curGamestate->CharSpecific1[0] == 17 || caseGamestate->CharSpecific1[0] == 17) {
            compValue += REFLECT_INVOKE(compIntState, curCosts[costLcStaff], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
            //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * costLcStaff;
        }
        else {
            compValue += REFLECT_INVOKE(compIntState, curCosts[costLcStaff], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
            //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * costLcStaffActive;
        }
        break;
    case 3://rachel
        if (curGamestate->CharSpecific1[0] < rachelWindMin) {
            compValue += REFLECT_INVOKE(compInt, curCosts[costLowWind], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], rachelWindMax);
            //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], rachelWindMax) * costLowWind;
        }
        else {
            compValue += REFLECT_INVOKE(compInt, curCosts[costWind], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], rachelWindMax);
            //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], rachelWindMax) * costWind;
        }
        break;
    case 5://Tager//tagerSpecific
        compValue += REFLECT_INVOKE(compBool, curCosts[costTagerSpark], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costTagerMagnetism], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.2;
        break;
    case 33://ES
        compValue += REFLECT_INVOKE(compBool, curCosts[costEsBuff], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.2;
        break;
    case 11://Nu
        compValue += REFLECT_INVOKE(compBool, curCosts[costNuGravity], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.1;
        break;
    case 27://Lambda
        compValue += REFLECT_INVOKE(compBool, curCosts[costNuGravity], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.1;
        break;
    case 13://Hazama
        compValue += REFLECT_INVOKE(compInt, curCosts[costHazChains], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 2);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 2) * 0.2;
        break;
    case 26://Celica
        compValue += REFLECT_INVOKE(compBool, curCosts[costCelAnyblueLife], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0] >0, caseGamestate->CharSpecific1[0] > 0) * 0.2;
        compValue += REFLECT_INVOKE(compInt, curCosts[costCelBlueAmount], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000) * 1;
        break;
    case 16://Valk
        compValue += REFLECT_INVOKE(compInt, curCosts[costValWolfMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costValWolfState], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compIntState(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 1;
        break;
    case 17://Plat
        compValue += REFLECT_INVOKE(compIntState, curCosts[costPlatCurItem], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.3;
        compValue += REFLECT_INVOKE(compInt, curCosts[costPlatNextItem], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0], 5);
        //compValue += compInt(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0],5) * 0.01;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costPlatItemNr], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compIntState(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.05;
        break;
    case 18://Relius
        compValue += REFLECT_INVOKE(compInt, curCosts[costReliusDollMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 10000) * 0.5;//Doll meter
        compValue += REFLECT_INVOKE(compIntState, curCosts[costReliusDollState], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compIntState(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.1; //Doll State
        compValue += REFLECT_INVOKE(compBool, curCosts[costReliusDollInactive], curGamestate->CharSpecific2[0] != 0, caseGamestate->CharSpecific2[0] != 0);
        //compValue += compBool(curGamestate->CharSpecific2[0]!=0, caseGamestate->CharSpecific2[0] != 0) * 0.6; //Doll State
        compValue += REFLECT_INVOKE(compBool, curCosts[costReliusDollCooldown], curGamestate->CharSpecific3[0] != 0, caseGamestate->CharSpecific3[0] != 0);
        //compValue += compBool(curGamestate->CharSpecific3[0] != 0, caseGamestate->CharSpecific3[0] != 0) * 1; //Doll Cooldown
        break;
    case 32://Susanoo
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock1], curGamestate->CharSpecific1[0] > 0, caseGamestate->CharSpecific1[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific1[0]>0, caseGamestate->CharSpecific1[0] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock1Lvl], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock2], curGamestate->CharSpecific2[0] > 0, caseGamestate->CharSpecific2[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific2[0] > 0, caseGamestate->CharSpecific2[0] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock2Lvl], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compIntState(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock3], curGamestate->CharSpecific3[0] > 0, caseGamestate->CharSpecific3[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific3[0] > 0, caseGamestate->CharSpecific3[0] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock3Lvl], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compIntState(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock4], curGamestate->CharSpecific4[0] > 0, caseGamestate->CharSpecific4[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific4[0] > 0, caseGamestate->CharSpecific4[0] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock4Lvl], curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]);
        //compValue += compIntState(curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock5], curGamestate->CharSpecific5[0] > 0, caseGamestate->CharSpecific5[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific5[0] > 0, caseGamestate->CharSpecific5[0] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock5Lvl], curGamestate->CharSpecific5[0], caseGamestate->CharSpecific5[0]);
        //compValue += compIntState(curGamestate->CharSpecific5[0], caseGamestate->CharSpecific5[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock6], curGamestate->CharSpecific6[0] > 0, caseGamestate->CharSpecific6[0]);
        //compValue += compBool(curGamestate->CharSpecific6[0] > 0, caseGamestate->CharSpecific6[0] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock6Lvl], curGamestate->CharSpecific6[0], caseGamestate->CharSpecific6[0]);
        //compValue += compIntState(curGamestate->CharSpecific6[0], caseGamestate->CharSpecific6[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock7], curGamestate->CharSpecific7[0] > 0, caseGamestate->CharSpecific7[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific7[0] > 0, caseGamestate->CharSpecific7[0] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock7Lvl], curGamestate->CharSpecific7[0], caseGamestate->CharSpecific7[0]);
        //compValue += compIntState(curGamestate->CharSpecific7[0], caseGamestate->CharSpecific7[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock8], curGamestate->CharSpecific8[0] > 0, caseGamestate->CharSpecific8[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific8[0] > 0, caseGamestate->CharSpecific8[0] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock8Lvl], curGamestate->CharSpecific8[0], caseGamestate->CharSpecific8[0]);
        //compValue += compIntState(curGamestate->CharSpecific8[0], caseGamestate->CharSpecific8[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock9], curGamestate->CharSpecific9[0], caseGamestate->CharSpecific9[0]);
        //compValue += compIntState(curGamestate->CharSpecific9[0], caseGamestate->CharSpecific9[0]) * 0.1;
        break;
    case 35://Jubei
        compValue += REFLECT_INVOKE(compBool, curCosts[costJubeiBuff], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.3;
        compValue += REFLECT_INVOKE(compBool, curCosts[costJubeiMark], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.1;
        break;
    case 31://Izanami
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzanamiFloating], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.2;
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzanamiShotCooldown], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.2;
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzanamiRibcage], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compBool(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.2;
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzanamiBitsStance], curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]);
        //compValue += compBool(curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]) * 0.2;
        break;
    case 29://Nine
        compValue += REFLECT_INVOKE(compIntState, curCosts[costNineSpell], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compIntState(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.3;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costNineSpellBackup], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compIntState(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costNineSlots], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compIntState(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costNineSlotsBackup], curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]);
        //compValue += compIntState(curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]) * 0.05;
        break;
    case 9://Carl
        compValue += REFLECT_INVOKE(compBool, curCosts[costCarlDollInactive], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 1;
        compValue += REFLECT_INVOKE(compInt, curCosts[costCarlDollMeter], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0], 5000);
        //compValue += compInt(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0], 5000) * 0.1;
        break;
    case 12://Tsubaki
        compValue += REFLECT_INVOKE(compInt, curCosts[costTsubakiMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 5);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0],5) * 1;
        break;
    case 24://Kokonoe
        compValue += REFLECT_INVOKE(compInt, curCosts[costKokoGravitronNr], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 8);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 8) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costKokoAnyGraviLeft], curGamestate->CharSpecific1[0] > 0, caseGamestate->CharSpecific1[0] > 0);
        //compValue += compBool(curGamestate->CharSpecific1[0] > 0, caseGamestate->CharSpecific1[0] > 0) * 0.3;
        break;
    case 19://Izayoi
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzayoiState], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.3;
        compValue += REFLECT_INVOKE(compInt, curCosts[costIzayoiStocks], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0], 8);
        //compValue += compInt(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0],8) * 0.8;
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzayoiSupermode], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compBool(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.4;
        break;
    case 7://Arakune
        compValue += REFLECT_INVOKE(compInt, curCosts[costArakuneCurseMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 6000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 6000) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costArakuneCurseActive], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.8;
        break;
    case 8://Bang
        compValue += REFLECT_INVOKE(compBool, curCosts[costBangFRKZ1], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]);
        //compValue += compBool(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costBangFRKZ2], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costBangFRKZ3], curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]);
        //compValue += compBool(curGamestate->CharSpecific3[0], caseGamestate->CharSpecific3[0]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costBangFRKZ4], curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]);
        //compValue += compBool(curGamestate->CharSpecific4[0], caseGamestate->CharSpecific4[0]) * 0.05;
        compValue += REFLECT_INVOKE(compInt, curCosts[costBangNailcount], curGamestate->CharSpecific5[0], caseGamestate->CharSpecific5[0], 12);
        //compValue += compInt(curGamestate->CharSpecific5[0], caseGamestate->CharSpecific5[0], 12) * 0.4;
        break;
    case 20://Amane
        //Amane
        compValue += REFLECT_INVOKE(compInt, curCosts[costAmaneDrillMeter], curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0], 6000);
        //compValue += compInt(curGamestate->CharSpecific1[0], caseGamestate->CharSpecific1[0],6000) * 0.4;
        compValue += REFLECT_INVOKE(compBool, curCosts[costAmaneDrillOverheat], curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]);
        //compValue += compBool(curGamestate->CharSpecific2[0], caseGamestate->CharSpecific2[0]) * 0.2;
        break;
    default:
        break;
    }
    switch (caseReplay.getCharIds()[1])
    {
    case 22://Azrael
        compValue += REFLECT_INVOKE(compBool, curCosts[costAzTopWeakEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costAzBotWeakEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costAzFireballEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compIntState(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.3;
        break;
    case 5://Tager
        compValue += REFLECT_INVOKE(compBool, curCosts[costTagerSparkEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costTagerMagnetismEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.2;
        break;
    case 13://Hazama
        compValue += REFLECT_INVOKE(compInt, curCosts[costHazChainsEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 2);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 2) * 0.2;
        break;
    case 16://Valk
        compValue += REFLECT_INVOKE(compInt, curCosts[costValWolfMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 10000) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costValWolfStateEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compIntState(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 1;
        break;
    case 17://Plat
        compValue += REFLECT_INVOKE(compIntState, curCosts[costPlatCurItemEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compIntState(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.3;
        //compValue += REFLECT_INVOKE(compInt, curCosts[0.01, curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 5);
        //compValue += compInt(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 5) * 0.01;
        //compValue += REFLECT_INVOKE(compIntState, curCosts[0.05, curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compIntState(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.05;
        break;
    case 18://Relius
        compValue += REFLECT_INVOKE(compInt, curCosts[costReliusDollMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 10000);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 10000) * 0.1;//Doll meter
        compValue += REFLECT_INVOKE(compIntState, curCosts[costReliusDollStateEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compIntState(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1; //Doll State
        compValue += REFLECT_INVOKE(compBool, curCosts[costReliusDollInactiveEnemy], curGamestate->CharSpecific2[1] != 0, caseGamestate->CharSpecific2[1] != 0);
        //compValue += compBool(curGamestate->CharSpecific2[1] != 0, caseGamestate->CharSpecific2[1] != 0) * 0.6; //Doll State
        compValue += REFLECT_INVOKE(compBool, curCosts[costReliusDollCooldownEnemy], curGamestate->CharSpecific3[1] != 0, caseGamestate->CharSpecific3[1] != 0);
        //compValue += compBool(curGamestate->CharSpecific3[1] != 0, caseGamestate->CharSpecific3[1] != 0) * 1; //Doll Cooldown
        break;
    case 32://Susanoo
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock1Enemy], curGamestate->CharSpecific1[1] > 0, caseGamestate->CharSpecific1[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific1[1] > 0, caseGamestate->CharSpecific1[1] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock1LvlEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compIntState(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock2Enemy], curGamestate->CharSpecific2[1] > 0, caseGamestate->CharSpecific2[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific2[1] > 0, caseGamestate->CharSpecific2[1] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock2LvlEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compIntState(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock3Enemy], curGamestate->CharSpecific3[1] > 0, caseGamestate->CharSpecific3[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific3[1] > 0, caseGamestate->CharSpecific3[1] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock3LvlEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compIntState(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock4Enemy], curGamestate->CharSpecific4[1] > 0, caseGamestate->CharSpecific4[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific4[1] > 0, caseGamestate->CharSpecific4[1] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock4LvlEnemy], curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]);
        //compValue += compIntState(curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock5Enemy], curGamestate->CharSpecific5[1] > 0, caseGamestate->CharSpecific5[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific5[1] > 0, caseGamestate->CharSpecific5[1] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock5LvlEnemy], curGamestate->CharSpecific5[1], caseGamestate->CharSpecific5[1]);
        //compValue += compIntState(curGamestate->CharSpecific5[1], caseGamestate->CharSpecific5[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock6Enemy], curGamestate->CharSpecific6[1] > 0, caseGamestate->CharSpecific6[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific6[1] > 0, caseGamestate->CharSpecific6[1] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock6LvlEnemy], curGamestate->CharSpecific6[1], caseGamestate->CharSpecific6[1]);
        //compValue += compIntState(curGamestate->CharSpecific6[1], caseGamestate->CharSpecific6[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock7Enemy], curGamestate->CharSpecific7[1] > 0, caseGamestate->CharSpecific7[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific7[1] > 0, caseGamestate->CharSpecific7[1] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock7LvlEnemy], curGamestate->CharSpecific7[1], caseGamestate->CharSpecific7[1]);
        //compValue += compIntState(curGamestate->CharSpecific7[1], caseGamestate->CharSpecific7[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock8Enemy], curGamestate->CharSpecific8[1] > 0, caseGamestate->CharSpecific8[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific8[1] > 0, caseGamestate->CharSpecific8[1] > 0) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costSusanUnlock8LvlEnemy], curGamestate->CharSpecific8[1], caseGamestate->CharSpecific8[1]);
        //compValue += compIntState(curGamestate->CharSpecific8[1], caseGamestate->CharSpecific8[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costSusanUnlock9Enemy], curGamestate->CharSpecific9[0], caseGamestate->CharSpecific9[0]);
        break;
    case 35://Jubei
        compValue += REFLECT_INVOKE(compBool, curCosts[costJubeiBuffEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.3;//Buff
        compValue += REFLECT_INVOKE(compBool, curCosts[costJubeiMarkEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1;//Mark
        break;
    case 31://Izanami
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzanamiFloatingEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.2;
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzanamiRibcageEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzanamiShotCooldownEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compBool(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.2;
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzanamiBitsStanceEnemy], curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]);
        //compValue += compBool(curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]) * 0.2;
        break;
    case 29://Nine
        compValue += REFLECT_INVOKE(compIntState, curCosts[costNineSpellEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compIntState(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.3;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costNineSpellBackupEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compIntState(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costNineSlotsEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compIntState(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.1;
        compValue += REFLECT_INVOKE(compIntState, curCosts[costNineSlotsBackupEnemy], curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]);
        //compValue += compIntState(curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]) * 0.05;
        break;
    case 9://Carl
        compValue += REFLECT_INVOKE(compBool, curCosts[costCarlDollInactiveEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 1;
        compValue += REFLECT_INVOKE(compInt, curCosts[costCarlDollMeterEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 5000);
        //compValue += compInt(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 5000) * 0.1;
        break;
    case 12://Tsubaki
        compValue += REFLECT_INVOKE(compInt, curCosts[costTsubakiMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 5);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 5) * 0.2;
        break;
    case 24://Kokonoe
        compValue += REFLECT_INVOKE(compInt, curCosts[costKokoGravitronNrEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 8);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 8) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costKokoAnyGraviLeft], curGamestate->CharSpecific1[1] > 0, caseGamestate->CharSpecific1[1] > 0);
        //compValue += compBool(curGamestate->CharSpecific1[1]>0, caseGamestate->CharSpecific1[1]>0) * 0.1;
        break;
    case 19://Izayoi
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzayoiStateEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]) * 0.1;
        compValue += REFLECT_INVOKE(compInt, curCosts[costIzayoiStocksEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 8);
        //compValue += compInt(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1], 8) * 0.2;
        compValue += REFLECT_INVOKE(compBool, curCosts[costIzayoiSupermodeEnemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compBool(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.1;
        break;
    case 7://Arakune
        compValue += REFLECT_INVOKE(compInt, curCosts[costArakuneCurseMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 6000);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1],6000) * 0.1;
        compValue += REFLECT_INVOKE(compBool, curCosts[costArakuneCurseActiveEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.8;
        break;
    case 8://Bang
        compValue += REFLECT_INVOKE(compBool, curCosts[costBangFRKZ1Enemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1]);
        //compValue += compBool(curGamestate->CharSpecific1[1] , caseGamestate->CharSpecific1[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costBangFRKZ2Enemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costBangFRKZ3Enemy], curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]);
        //compValue += compBool(curGamestate->CharSpecific3[1], caseGamestate->CharSpecific3[1]) * 0.05;
        compValue += REFLECT_INVOKE(compBool, curCosts[costBangFRKZ4Enemy], curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]);
        //compValue += compBool(curGamestate->CharSpecific4[1], caseGamestate->CharSpecific4[1]) * 0.05;
        compValue += REFLECT_INVOKE(compInt, curCosts[costBangNailcount], curGamestate->CharSpecific5[1], caseGamestate->CharSpecific5[1], 12);
        //compValue += compInt(curGamestate->CharSpecific5[1], caseGamestate->CharSpecific5[1], 12) * 0.1;
        break;
    case 20://Amane
        compValue += REFLECT_INVOKE(compInt, curCosts[costAmaneDrillMeterEnemy], curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 6000);
        //compValue += compInt(curGamestate->CharSpecific1[1], caseGamestate->CharSpecific1[1], 6000) * 0.4;
        compValue += REFLECT_INVOKE(compBool, curCosts[costAmaneDrillOverheatEnemy], curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]);
        //compValue += compBool(curGamestate->CharSpecific2[1], caseGamestate->CharSpecific2[1]) * 0.1;
        break;
    default:
        break;
    }
    return compValue;
}


int CbrData::makeCharacterID(std::string charName) {
    if (charName == "es") { return 0; };//es
    if (charName == "ny") { return 1; };//nu
    if (charName == "ma") { return 2; };// mai
    if (charName == "tb") { return 3; };// tsubaki
    if (charName == "rg") { return 4; };//ragna
    if (charName == "hz") { return 5; };//hazama
    if (charName == "jn") { return 6; };//jin
    if (charName == "mu") { return 7; };//mu
    if (charName == "no") { return 8; };//noel
    if (charName == "mk") { return 9; };//makoto
    if (charName == "rc") { return 10; };//rachel
    if (charName == "vh") { return 11; };//valk
    if (charName == "tk") { return 12; };//taokaka
    if (charName == "pt") { return 13; };//platinum
    if (charName == "tg") { return 14; };//tager
    if (charName == "rl") { return 15; };//relius
    if (charName == "lc") { return 16; };//litchi
    if (charName == "iz") { return 17; };//izayoi
    if (charName == "ar") { return 18; };//arakune
    if (charName == "am") { return 19; };//amane
    if (charName == "bn") { return 20; };//bang
    if (charName == "bl") { return 21; };//bullet
    if (charName == "ca") { return 22; };//carl
    if (charName == "az") { return 23; };//azrael
    if (charName == "ha") { return 24; };//hakumen
    if (charName == "kg") { return 25; };//kagura
    if (charName == "kk") { return 26; };//koko
    if (charName == "rm") { return 27; };//lambda
    if (charName == "hb") { return 28; };//hibiki
    if (charName == "tm") { return 29; };//terumi
    if (charName == "ph") { return 30; };//nine
    if (charName == "ce") { return 31; };//Celica
    if (charName == "nt") { return 32; };//naoto
    if (charName == "mi") { return 33; };//izanami
    if (charName == "su") { return 34; };//susan
    if (charName == "jb") { return 35; };//jubei
    return -1;
}

void CbrData::setCurGamestateCosts(Metadata* curGamestate) {
    curCosts = costs.basic;
    if (curGamestate->getHit()[1] == true) { curCosts = costs.combo; }
    if (curGamestate->getBlocking()[0] == true) { curCosts = costs.blocking; }
    if (curGamestate->getBlocking()[1] == true) { curCosts = costs.pressure; }
}

float CbrData::HelperCompMatch(Metadata* curGamestate, Metadata* caseGamestate) {
    
    auto &p1HelpersCur = curGamestate->getPlayerHelpers(0);
    auto& p1HelpersCase = caseGamestate->getPlayerHelpers(0);
    float completeCost = 0;
    int curSize = p1HelpersCur.size();
    int caseSize = p1HelpersCase.size();
    bool skip = false;
    float costBuffer = 9999;
    int indexBuffer = -1;


    if (curSize <= 0 && caseSize <= 0) {
        completeCost += 0;
        skip = true;
    }
    if (skip == false && curSize <= 0 && caseSize > 0) {
        completeCost += curCosts[costHelperSum] * caseSize;
        skip = true;
    }
    if (skip == false && curSize <= 0 && caseSize > 0) {
        completeCost += curCosts[costHelperSum] * caseSize;
        skip = true;
    }

    static std::array<bool, 10> checkArr{};
    static std::array<bool, 10> checkArrP2{};
    if (!skip) {
        bool resetNeeded = false;
        for (int i = 0; i < curSize; i++) {
            costBuffer = 9999;
            indexBuffer = -1;
            for (int j = 0; j < caseSize; j++) {
                if (checkArr[j] == false) {
                    auto cost = HelperComp(curGamestate, caseGamestate, p1HelpersCur[i].get(), p1HelpersCase[j].get(), false, false);
                    if (cost < costBuffer) {
                        costBuffer = cost;
                        indexBuffer = j;
                    }
                }
            }
            if (indexBuffer == -1) {
                costBuffer = HelperComp(curGamestate, caseGamestate, nullptr, nullptr, true, false);
            }
            else {
                checkArr[indexBuffer] = true;
            }
            completeCost += costBuffer;
        }

        for (int i = 0; i < p1HelpersCase.size(); i++) {
            if (checkArr[i] == false) {
                completeCost += HelperComp(curGamestate, caseGamestate, nullptr, nullptr, true, false);
            }
        }
        for (int i = 0; i < 10; i++) {
            checkArr[i] = false;
        }
    }
    

    auto&p2HelpersCur = curGamestate->getPlayerHelpers(1);
    auto& p2HelpersCase = caseGamestate->getPlayerHelpers(1);
    curSize = p2HelpersCur.size();
    caseSize = p2HelpersCase.size();
    skip = false;
    if (curSize <= 0 && caseSize <= 0) {
        completeCost += 0;
        skip = true;
    }
    if (skip == false && curSize <= 0 && caseSize > 0) {
        completeCost += curCosts[costHelperSum] * caseSize;
        skip = true;
    }
    if (skip == false && curSize <= 0 && caseSize > 0) {
        completeCost += curCosts[costHelperSum] * caseSize;
        skip = true;
    }

    if (!skip) {
        for (int i = 0; i < p2HelpersCur.size(); i++) {
            costBuffer = 9999;
            indexBuffer = -1;
            for (int j = 0; j < p2HelpersCase.size(); j++) {
                if (checkArrP2[j] == false) {
                    auto cost = HelperComp(curGamestate, caseGamestate, p2HelpersCur[i].get(), p2HelpersCase[j].get(), false, true);
                    if (cost < costBuffer) {
                        costBuffer = cost;
                        indexBuffer = j;
                    }
                }
            }
            if (indexBuffer == -1) {
                costBuffer = HelperComp(curGamestate, caseGamestate, nullptr, nullptr, true, false);
            }
            else {
                checkArrP2[indexBuffer] = true;
            }
            completeCost += costBuffer;
        }

        for (int i = 0; i < p2HelpersCase.size(); i++) {
            if (checkArrP2[i] == false) {
                completeCost += HelperComp(curGamestate, caseGamestate, nullptr, nullptr, true, true);
            }
        }
        for (int i = 0; i < 10; i++) {
            checkArrP2[i] = false;
        }
    }
    
    
    return 0;
}
/*
float HelperCompMatchOld(Metadata* curGamestate, Metadata* caseGamestate) {
    auto curHelpers = curGamestate->getHelpers()[0];
    auto caseHelpers = caseGamestate->getHelpers()[0];
    float ret = 0;
    int curSize = curHelpers.size();
    int caseSize = caseHelpers.size();
    bool opponent = false;
    bool skip = false;
    if (curSize <= 0 && caseSize <= 0) {
        ret += 0;
        skip = true;
    }
    if (skip == false && curSize <= 0 && caseSize >0) {
        ret += curCosts[costHelperSum] * caseSize;
        skip = true;
    }
    if (skip == false && curSize <= 0 && caseSize > 0) {
        ret += curCosts[costHelperSum] * caseSize;
        skip = true;
    }

    if (skip == false) {
        std::vector<HelperMapping> hMap;

        for (int i = 0; i < curHelpers.size() + 1; ++i) {

            for (int j = 0; j < caseHelpers.size() + 1; ++j) {
                HelperMapping hM{};

                if (j == caseHelpers.size()) {
                    hM.caseHelperIndex = -1;
                }
                else {
                    hM.caseHelperIndex = j;
                }
                if (i == curHelpers.size()) {
                    hM.curHelperIndex = -1;
                }
                else {
                    hM.curHelperIndex = i;
                }
                if (hM.caseHelperIndex == -1 || hM.curHelperIndex == -1) {
                    if (hM.caseHelperIndex == -1 && hM.curHelperIndex == -1) {
                        hM.comparisonValue = 0;
                    }
                    else {
                        Helper* standin = nullptr;
                        if (hM.curHelperIndex == -1) {
                            hM.comparisonValue = HelperComp(curGamestate, caseGamestate, standin, standin, true, opponent);
                        }
                        else {
                            hM.comparisonValue = HelperComp(curGamestate, caseGamestate, curHelpers[i].get(), standin, true, opponent);
                        }
                    }
                }
                else {
                    hM.comparisonValue = HelperComp(curGamestate, caseGamestate, curHelpers[i].get(), caseHelpers[j].get(), false, opponent);
                }

                hMap.push_back(hM);
            }
        }
        std::unordered_map<int, int> usedUpCaseIndex = {};
        std::unordered_map<int, int> usedUpCurIndex = {};
        auto result1 = findBestHelperMapping(hMap, usedUpCaseIndex, usedUpCurIndex, 0);
        ret += result1.compValue;
    }
    

    //do it again for opponent
    curHelpers = curGamestate->getHelpers()[1];
    caseHelpers = caseGamestate->getHelpers()[1];
    opponent = true;
    curSize = curHelpers.size();
    caseSize = caseHelpers.size();
    std::vector<HelperMapping> hMap = {};
    skip = false;
    if (curSize <= 0 && caseSize <= 0) {
        ret += 0;
        skip = true;
    }
    if (skip == false && curSize <= 0 && caseSize > 0) {
        ret += curCosts[costHelperSum] * caseSize;
        skip = true;
    }
    if (skip == false && curSize <= 0 && caseSize > 0) {
        ret += curCosts[costHelperSum] * caseSize;
        skip = true;
    }
    if (skip == false) {
        for (int i = 0; i < curHelpers.size() + 1; ++i) {

            for (int j = 0; j < caseHelpers.size() + 1; ++j) {
                HelperMapping hM{};

                if (j == caseHelpers.size()) {
                    hM.caseHelperIndex = -1;
                }
                else {
                    hM.caseHelperIndex = j;
                }
                if (i == curHelpers.size()) {
                    hM.curHelperIndex = -1;
                }
                else {
                    hM.curHelperIndex = i;
                }
                if (hM.caseHelperIndex == -1 || hM.curHelperIndex == -1) {
                    if (hM.caseHelperIndex == -1 && hM.curHelperIndex == -1) {
                        hM.comparisonValue = 0;
                    }
                    else {
                        Helper* standin = nullptr;
                        if (hM.curHelperIndex == -1) {
                            hM.comparisonValue = HelperComp(curGamestate, caseGamestate, standin, standin, true, opponent);
                        }
                        else {
                            hM.comparisonValue = HelperComp(curGamestate, caseGamestate, curHelpers[i].get(), standin, true, opponent);
                        }
                        
                    }
                }
                else {
                    hM.comparisonValue = HelperComp(curGamestate, caseGamestate, curHelpers[i].get(), caseHelpers[j].get(), false, opponent);
                }

                hMap.push_back(hM);
            }
        }
        std::unordered_map<int, int> usedUpCaseIndex = {};
        std::unordered_map<int, int> usedUpCurIndex = {};
        auto result2 = findBestHelperMapping(hMap, usedUpCaseIndex, usedUpCurIndex, 0);
        ret += result2.compValue;
    }
    return ret;
}
*/

recurseHelperMapping CbrData::findBestHelperMapping(std::vector<HelperMapping>& hMap, std::unordered_map<int, int>& usedUpCaseIndex, std::unordered_map<int, int>& usedUpCurIndex, int depth) {
    auto startingIndex = -2;
    float bestCompVal = -1;
    depth++;
    recurseHelperMapping ret;

    for (int i = 0; i < hMap.size(); ++i) {
        auto& val = hMap[i];
        auto found = usedUpCaseIndex.find(val.caseHelperIndex);
        auto ok = (found != usedUpCaseIndex.end() && found->second <= depth);
        found = usedUpCurIndex.find(val.curHelperIndex);
        auto ok2 = (found != usedUpCurIndex.end() && found->second <= depth);
        auto addable = (!ok || (val.caseHelperIndex == -1 && val.curHelperIndex != -1)) && (!ok2 || val.curHelperIndex == -1);
        if (addable && (val.caseHelperIndex == startingIndex || startingIndex == -2)) {
            startingIndex = val.caseHelperIndex;

            usedUpCaseIndex[val.caseHelperIndex] = depth;
            usedUpCurIndex[val.curHelperIndex] = depth;
            auto recRet = findBestHelperMapping(hMap, usedUpCaseIndex, usedUpCurIndex, depth);
            auto compVal = recRet.compValue;
            if (compVal < 0) { compVal = 0; }
            compVal += val.comparisonValue;
            if (compVal < bestCompVal || bestCompVal == -1) {
                bestCompVal = compVal;
                ret.hMaps.push_back(&val);
                ret.hMaps.insert(ret.hMaps.end(), recRet.hMaps.begin(), recRet.hMaps.end());
            }
        }
    }
    ret.compValue = bestCompVal;
    return ret;
}

float CbrData::HelperComp(Metadata* curGamestate, Metadata* caseGamestate, Helper* curHelper, Helper* caseHelper, bool autoFail, bool opponent) {
    float compValue = 0;
    double multiplier = 1;

    if (curHelper != nullptr && curHelper->proximityScale) {
        auto distance = fmin((float)abs(curGamestate->getPosX()[!opponent] - curHelper->posX) / maxXDistScreen, 1);
        multiplier = fmax(-0.01731221 + (0.9791619 - -0.01731221) / (1 + pow((distance / 0.364216), (4.017896))), 0) ;
    }

    if (autoFail) {
        return curCosts[costHelperSum] * multiplier;
    }
    compValue += compStateHash(curHelper->typeHash, caseHelper->typeHash) * curCosts[costHelperType];
    compValue += compRelativePosX(curGamestate->getPosX()[0], curHelper->posX, caseGamestate->getPosX()[0], caseHelper->posX) * curCosts[costHelperPosX];
    compValue += compRelativePosY(curGamestate->getPosY()[0], curHelper->posY, caseGamestate->getPosY()[0], caseHelper->posY) * curCosts[costHelperPosY];
    compValue += compStateHash(curHelper->currentActionHash, caseHelper->currentActionHash) * curCosts[costHelperState];
    compValue += compBool(curHelper->hit, caseHelper->hit) * curCosts[costHelperHit];
    compValue += compBool(curHelper->attack, caseHelper->attack) * curCosts[costHelperAttack];
    compValue += compHelperOrder(curGamestate->getPosX()[1], curGamestate->getPosX()[0], curHelper->posX, caseGamestate->getPosX()[1], caseGamestate->getPosX()[0], caseHelper->posX) * curCosts[costHelperOrder];
    compValue += compHelperOrder(curGamestate->getPosX()[0], curGamestate->getPosX()[1], curHelper->posX, caseGamestate->getPosX()[0], caseGamestate->getPosX()[1], caseHelper->posX) * curCosts[costHelperOrder];
    compValue = compValue * multiplier;

    return compValue;
}

/*    std::array< int, 2>posX;
    std::array< int, 2>posY;
    bool facing;*/




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


void CbrData::debugPrint(Metadata* curM, int nextCI, int bestCI, int nextRI, int bestRI, std::vector<debugCaseIndex> dci, int FinalBestRI, int FinalBestCI) {
    debugCounter++;
    std::string str = "\n--------------------------------\n";

    str += "SelectedReplay: " + std::to_string(FinalBestRI) + " - SelectedCase: " + std::to_string(FinalBestCI)+"\n\n";

    str += "CI: " + std::to_string(debugCounter)  + "FA: " + std::to_string(framesActive) + "\n";
    str += caseSwitchReason + "\n";
    str += curM->PrintState();
    debugTextArr.push_back(str);
    str = "";

    float nextCompVal = -1;
    float helperVal = -1;
    if (nextRI != -1 && nextCI <= replayFiles[nextRI].getCaseBaseLength() - 1) {
        nextCompVal = comparisonFunctionDebug(curM, replayFiles[nextRI].getCase(nextCI)->getMetadata(), replayFiles[nextRI], replayFiles[nextRI].getCase(nextCI), nextRI, nextCI, true);
        auto helperVal = HelperCompMatch(curM, replayFiles[nextRI].getCase(nextCI)->getMetadata());
        nextCompVal += helperVal;
        if (helperVal > 0) {
            debugText += "Helper Cost: " + std::to_string(helperVal);
        }
        debugTextArr.push_back(debugText);
        debugText = "";


        str += "Next Cost: " + std::to_string(nextCompVal);
        str += "\nReplay: " + std::to_string(nextRI) + " - Case: " + std::to_string(nextCI) + " - Start: " + std::to_string(replayFiles[nextRI].getCase(nextCI)->getStartingIndex()) + " - End: " + std::to_string(replayFiles[nextRI].getCase(nextCI)->getEndIndex()) + "\n";
        //str += debugPrintCompValues(nextC, curM);
        for (int k = replayFiles[nextRI].getCase(nextCI)->getStartingIndex(); k <= replayFiles[nextRI].getCase(nextCI)->getEndIndex(); k++) {
            str += std::to_string(replayFiles[nextRI].getInput(k)) + ", ";
        }
        str += "\n\n";
    }
    else {
        str += "No next Case available\n\n";
    }
    
    
    
    debugTextArr.push_back(str);
    str = "";

    auto bestCompVal = comparisonFunctionDebug(curM, replayFiles[bestRI].getCase(bestCI)->getMetadata(), replayFiles[bestRI], replayFiles[bestRI].getCase(bestCI), bestRI, bestCI,false);
    helperVal = HelperCompMatch(curM, replayFiles[bestRI].getCase(bestCI)->getMetadata());
    bestCompVal += helperVal;
    if (helperVal > 0) {
        debugText += "Helper Cost: " + std::to_string(helperVal);
    }
    debugTextArr.push_back(debugText);
    debugText = "";

    //str += "\n" + nextC->getMetadata()->PrintState();

    str += "Best Cost: " + std::to_string(bestCompVal);
    str += "\nReplay: " + std::to_string(bestRI) + " - Case: " + std::to_string(bestCI) + " - Start: " + std::to_string(replayFiles[bestRI].getCase(bestCI)->getStartingIndex()) + " - End: " + std::to_string(replayFiles[bestRI].getCase(bestCI)->getEndIndex()) + "\n";
    //str += debugPrintCompValues(bestC, curM);
    for (int k = replayFiles[bestRI].getCase(bestCI)->getStartingIndex(); k <= replayFiles[bestRI].getCase(bestCI)->getEndIndex(); k++) {
        str += std::to_string(replayFiles[bestRI].getInput(k)) + ", ";
    }
    str += "\n\n";
    debugTextArr.push_back(str);
    str = "";


    for (int i = 0; i < 30 && i < dci.size(); i++) {
        auto bestCompVal = comparisonFunctionDebug(curM, replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex)->getMetadata(), replayFiles[dci[i].replayIndex], replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex), dci[i].replayIndex, dci[i].caseIndex, false);
        helperVal = HelperCompMatch(curM, replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex)->getMetadata());
        bestCompVal += helperVal;
        if (helperVal > 0) {
            debugText += "Helper Cost: " + std::to_string(helperVal);
        }
        debugTextArr.push_back(debugText);
        debugText = "";

        //str += "\n" + nextC->getMetadata()->PrintState();

        str += "Ordered Cost: " + std::to_string(bestCompVal);
        str += "\nReplay: " + std::to_string(dci[i].replayIndex) + " - Case: " + std::to_string(dci[i].caseIndex) + " - Start: " + std::to_string(replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex)->getStartingIndex()) + " - End: " + std::to_string(replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex)->getEndIndex()) + "\n";
        //str += debugPrintCompValues(bestC, curM);
        for (int k = replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex)->getStartingIndex(); k <= replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex)->getEndIndex(); k++) {
            str += std::to_string(replayFiles[dci[i].replayIndex].getInput(k)) + ", ";
        }
        str += "\n\n";
        debugTextArr.push_back(str);
        str = "";
    }
    

}

std::string CbrData::debugPrintCompValues(CbrCase* nextC, Metadata* curM) {
    std::string str = "";
    //str += "Cost PosX: " + std::to_string(compRelativePosX(curM->getPosX(), nextC->getMetadata()->getPosX()) * costXDist) + " - ";
    str += " PosX: " + std::to_string(nextC->getMetadata()->getPosX()[0]) + " - " + std::to_string(nextC->getMetadata()->getPosX()[1]) + " - " + std::to_string(abs(nextC->getMetadata()->getPosX()[0] - nextC->getMetadata()->getPosX()[1])) + "\n";
    //str += "Cost PosY: " + std::to_string(compRelativePosY(curM->getPosY(), nextC->getMetadata()->getPosY()) * costYDist) + " - ";
    str += " PosY: " + std::to_string(nextC->getMetadata()->getPosY()[0]) + " - " + std::to_string(nextC->getMetadata()->getPosY()[1]) + " - " + std::to_string(abs(nextC->getMetadata()->getPosY()[0] - nextC->getMetadata()->getPosY()[1])) + "\n";

    str += "Cost CurAction: " + std::to_string(compStateHash(curM->getCurrentActionHash()[0], nextC->getMetadata()->getCurrentActionHash()[0]) * costAiState) + " - " + std::to_string(compStateHash(curM->getCurrentActionHash()[1], nextC->getMetadata()->getCurrentActionHash()[1]) * costEnemyState);
    str += " CurAction: " + nextC->getMetadata()->getCurrentAction()[0] + " - " + nextC->getMetadata()->getCurrentAction()[1] + "\n";

    str += "Cost LastAction: " + std::to_string(compStateHash(curM->getLastActionHash()[0], nextC->getMetadata()->getLastActionHash()[0]) * costlastAiState) + " - " + std::to_string(compStateHash(curM->getLastActionHash()[1], nextC->getMetadata()->getLastActionHash()[1]) * costlastEnemyState);
    str += " LastAction: " + nextC->getMetadata()->getCurrentAction()[0] + " - " + nextC->getMetadata()->getCurrentAction()[1] + "\n";

    str += "Cost Air: " + std::to_string(compAirborneState(curM->getAir()[0], nextC->getMetadata()->getAir()[0]) * costAiAir) + " - " + std::to_string(compAirborneState(curM->getAir()[1], nextC->getMetadata()->getAir()[1]) * costEnemyAir);
    str += " Air: " + std::to_string(nextC->getMetadata()->getAir()[0]) + " - " + std::to_string(nextC->getMetadata()->getAir()[1]) + "\n";

    str += "Cost Attack: " + std::to_string(compBool(curM->getAttack()[0], nextC->getMetadata()->getAttack()[0]) * costAiAttack) + " - " + std::to_string(compBool(curM->getAttack()[1], nextC->getMetadata()->getAttack()[1]) * costEnemyAttack);
    str += " Attack: " + std::to_string(nextC->getMetadata()->getAttack()[0]) + " - " + std::to_string(nextC->getMetadata()->getAttack()[1]) + "\n";

    str += "Cost Crouching: " + std::to_string(compCrouching(curM->getCrouching()[0], nextC->getMetadata()->getCrouching()[0]) * costAiCrouching) + " - " + std::to_string(compCrouching(curM->getCrouching()[1], nextC->getMetadata()->getCrouching()[1]) * costEnemyCrouching);
    str += " Crouching: " + std::to_string(nextC->getMetadata()->getCrouching()[0]) + " - " + std::to_string(nextC->getMetadata()->getCrouching()[1]) + "\n";

    str += "Cost Neutral: " + std::to_string(compNeutralState(curM->getNeutral()[0], nextC->getMetadata()->getNeutral()[0]) * costAiNeutral) + " - " + std::to_string(compNeutralState(curM->getNeutral()[1], nextC->getMetadata()->getNeutral()[1]) * costEnemyNeutral);
    str += " Neutral: " + std::to_string(nextC->getMetadata()->getNeutral()[0]) + " - " + std::to_string(nextC->getMetadata()->getNeutral()[1]) + "\n";
    //str += "AtkType: " + std::to_string(attackType[0]) + " - " + std::to_string(attackType[1]) + "\n";

    str += "Cost Wakeup: " + std::to_string(compWakeupState(curM->getWakeup()[0], nextC->getMetadata()->getWakeup()[0]) * costAiWakeup) + " - " + std::to_string(compWakeupState(curM->getWakeup()[1], nextC->getMetadata()->getWakeup()[1]) * costEnemyWakeup);
    str += " Wakeup: " + std::to_string(nextC->getMetadata()->getWakeup()[0]) + " - " + std::to_string(nextC->getMetadata()->getWakeup()[1]) + "\n";

    str += "Cost Blocking: " + std::to_string(compBlockState(curM->getBlocking()[0], nextC->getMetadata()->getBlocking()[0]) * costAiBlocking) + " - " + std::to_string(compBlockState(curM->getBlocking()[1], nextC->getMetadata()->getBlocking()[1]) * costEnemyBlocking);
    str += " Blocking: " + std::to_string(nextC->getMetadata()->getBlocking()[0]) + " - " + std::to_string(nextC->getMetadata()->getBlocking()[1]) + "\n";

    str += "Cost BlockingTF: " + std::to_string(compBlockingThisFrameState(curM->getBlockThisFrame()[0], nextC->getMetadata()->getBlockThisFrame()[0]) * costAiBlockThisFrame) + " - " + std::to_string(compBlockingThisFrameState(curM->getBlockThisFrame()[1], nextC->getMetadata()->getBlockThisFrame()[1]) * costEnemyBlockhisFrame);
    str += " BlockingTF: " + std::to_string(nextC->getMetadata()->getBlockThisFrame()[0]) + " - " + std::to_string(nextC->getMetadata()->getBlockThisFrame()[1]) + "\n";

    str += "Cost Hit: " + std::to_string(compHitState(curM->getHit()[0], nextC->getMetadata()->getHit()[0]) * costAiHit) + " - " + std::to_string(compHitState(curM->getHit()[1], nextC->getMetadata()->getHit()[1]) * costEnemyHit);
    str += " Hit: " + std::to_string(nextC->getMetadata()->getHit()[0]) + " - " + std::to_string(nextC->getMetadata()->getHit()[1]) + "\n";

    str += "Cost HitTF: " + std::to_string(compGetHitThisFrameState(curM->getHitThisFrame()[0], nextC->getMetadata()->getHitThisFrame()[0]) * costAiHitThisFrame) + " - " + std::to_string(compGetHitThisFrameState(curM->getHitThisFrame()[1], nextC->getMetadata()->getHitThisFrame()[1]) * costEnemyHitThisFrame);
    str += " HitTF: " + std::to_string(nextC->getMetadata()->getHitThisFrame()[0]) + " - " + std::to_string(nextC->getMetadata()->getHitThisFrame()[1]) + "\n";

    str += "Cost RoundState: " + std::to_string(compIntState(curM->matchState, nextC->getMetadata()->matchState) * costMatchState);
    str += " RoundState: " + std::to_string(nextC->getMetadata()->matchState) + "\n";

    str += "Cost ODState: " + std::to_string(compBool(curM->overdriveTimeleft[0] > 0, nextC->getMetadata()->overdriveTimeleft[0] > 0) * costAiOverdriveState) + " - " + std::to_string(compBool(curM->overdriveTimeleft[1] > 0, nextC->getMetadata()->overdriveTimeleft[1] > 0) * costEnemyOverdriveState);
    str += " ODState: " + std::to_string(nextC->getMetadata()->overdriveTimeleft[0]) + " - " + std::to_string(nextC->getMetadata()->overdriveTimeleft[1]) + "\n";

    if (curM->getHit()[0] == true) {

        str += "Cost comboPror: " + std::to_string(compInt(curM->getComboProration()[0], nextC->getMetadata()->getComboProration()[0], maxProration) * costAiProration);
        str += " comboPror: " + std::to_string(nextC->getMetadata()->getComboProration()[0]) + "\n";

        str += "Cost starterRating: " + std::to_string(compIntState(curM->getStarterRating()[0], nextC->getMetadata()->getStarterRating()[0]) * costAiStarterRating);
        str += " starterRating: " + std::to_string(nextC->getMetadata()->getStarterRating()[0]) + "\n";

        str += "Cost starterRating: " + std::to_string(compInt(curM->getComboTime()[0], nextC->getMetadata()->getComboTime()[0], maxComboTime) * costAiComboTime);
        str += " comboTime: " + std::to_string(nextC->getMetadata()->getComboTime()[0]) + "\n";

    }
    if (curM->getHit()[1] == true) {

        str += "Cost OPcomboPror: " + std::to_string(compInt(curM->getComboProration()[1], nextC->getMetadata()->getComboProration()[1], maxProration) * costEnemyProration);
        str += " OPcomboPror: " + std::to_string(nextC->getMetadata()->getComboProration()[1]) + "\n";

        str += "Cost OPstarterRating: " + std::to_string(compIntState(curM->getStarterRating()[1], nextC->getMetadata()->getStarterRating()[1]) * costEnemyStarterRating);
        str += " OPstarterRating: " + std::to_string(nextC->getMetadata()->getStarterRating()[1]) + "\n";

        str += "Cost OPstarterRating: " + std::to_string(compInt(curM->getComboTime()[1], nextC->getMetadata()->getComboTime()[1], maxComboTime) * costEnemyComboTime);
        str += " OPcomboTime: " + std::to_string(nextC->getMetadata()->getComboTime()[1]) + "\n";

        //str += "Cost DistanceToWallCombo: " + std::to_string(compDistanceToWall(curM->getPosX(), nextC->getMetadata()->getPosX(), curM->getFacing(), nextC->getMetadata()->getFacing()) * costWallDistCombo);
        str += " DistanceToWallCombo: " + std::to_string(nextC->getMetadata()->getFacing()) + "\n";
    }
    else {
        //str += "Cost DistanceToWall: " + std::to_string(compDistanceToWall(curM->getPosX(), nextC->getMetadata()->getPosX(), curM->getFacing(), nextC->getMetadata()->getFacing()) * costWallDist);
        str += " DistanceToWall: " + std::to_string(nextC->getMetadata()->getFacing()) + "\n";
    }

    str += "Cost Helper: " + std::to_string(HelperCompMatch(curM, nextC->getMetadata()));
    str += " DistanceToWall: " + std::to_string(nextC->getMetadata()->getFacing()) + "\n";
    //bufComparison += ;

    switch (characterIndex)
    {
    default:
        break;
    }


    return str;
}

std::string CbrData::getCharName() {
    return characterName;
}

std::string CbrData::getPlayerName() {
    return playerName;
}

int CbrData::getReplayCount() {
    return replayFiles.size();
}

void CbrData::resetReplayVariables() {
    activeReplay = -1;
    activeCase = -1;
    activeFrame = -1;
    invertCurCase = false;
    framesActive = 0;

    auto replayFileSize = replayFiles.size();
    for (std::size_t i = 0; i < replayFileSize; ++i) {
        auto caseBaseSize = replayFiles[i].getCaseBase()->size();
        for (std::size_t j = 0; j < caseBaseSize; ++j) {
            auto caseData = replayFiles[i].getCase(j);
            caseData->caseCooldownFrameStart = -1;
        }
    }
}