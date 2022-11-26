#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <unordered_map>
#include "Metadata.h"
#include "CbrReplayFile.h"


struct HelperMapping
{
    int curHelperIndex;
    int caseHelperIndex;
    float comparisonValue;
};

struct recurseHelperMapping {
    float compValue;
    std::vector<HelperMapping*> hMaps;
};

class CbrData {
private:
    friend class boost::serialization::access;
    std::string playerName;
    std::string characterName;
    int characterIndex = -1;

    std::vector<CbrReplayFile> replayFiles;
    int activeReplay = -1;
    int activeCase = -1;
    int activeFrame = -1;
    bool invertCurCase = false;

    bool enabled = false;

    


public:
    std::string debugText = "";
    std::vector<std::string> debugTextArr = {};
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& playerName& characterName& replayFiles & activeReplay & activeCase & activeFrame & invertCurCase & enabled;
    }
    CbrData();
    //player name,  characterName
    CbrData::CbrData(std::string n, std::string p1, int charId);
    std::string getCharName();
    void setCharName(std::string);
    std::string getPlayerName();
    int getReplayCount();
    void setEnabled(bool);
    void setPlayerName(std::string name);
    bool getEnabled();
    void AddReplay(CbrReplayFile);
    void deleteReplays(int start, int end);
    CbrReplayFile* getLastReplay();
    int CBRcomputeNextAction(Metadata*);
    int inverseInput(int);
    bool switchCaseCheck(Metadata*);
    void CbrData::resetCbr();

    float CbrData::comparisonFunction(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay);
    float compRelativePosX(std::array< int, 2>, std::array< int, 2>);
    float compRelativePosY(std::array< int, 2>, std::array< int, 2>);
    float compState(std::string, std::string);
    float compNeutralState(bool, bool);
    float compAirborneState(bool, bool);
    float compWakeupState(bool, bool);
    float compBlockState(bool, bool);
    float compHitState(bool, bool);
    float compGetHitThisFrameState(bool, bool);
    float compBlockingThisFrameState(bool, bool);
    float compCrouching(bool, bool);
    float compDistanceToWall(std::array< int, 2>, std::array< int, 2>, bool, bool);
    void debugPrint(CbrCase*, CbrCase*, Metadata*, float, float, int, int, int, int);
    std::string debugPrintCompValues(CbrCase* nextC, Metadata* curM);
    float comboSpecificComp(Metadata* curGamestate, Metadata* caseGamestate);   
    float compBool(bool cur, bool cas);
    float compInt(int cur, int cas, int max);
    float compIntState(int cur, int cas);
    float compStateHash(size_t curS, size_t caseS);
    int CbrData::makeCharacterID(std::string charName);
    int CbrData::getCharacterIndex();
    float CbrData::HelperCompMatch(Metadata* curGamestate, Metadata* caseGamestate);
    float CbrData::HelperComp(Metadata* curGamestate, Metadata* caseGamestate, Helper* curHelper, Helper* caseHelper, bool autoFail, bool opponent);
    float  CbrData::compRelativePosX(int curP1, int curP2, int caseP1, int caseP2);
    float  CbrData::compRelativePosY(int curP1, int curP2, int caseP1, int caseP2);
    recurseHelperMapping CbrData::findBestHelperMapping(std::vector<HelperMapping>& hMap, std::unordered_map<int, int>& usedUpCaseIndex, std::unordered_map<int, int>& usedUpCurIndex, int depth);
    float CbrData::compHelperOrder(int curPosOpponent, int curPosChar, int curPosHelper, int casePosOpponent, int casePosChar, int casePosHelper);
    bool CbrData::caseValidityCheck(Metadata* curGamestate, int replayIndex, int caseIndex);
    bool CbrData::nextCaseValidityCheck(Metadata* curGamestate, int replayIndex, int caseIndex);
};