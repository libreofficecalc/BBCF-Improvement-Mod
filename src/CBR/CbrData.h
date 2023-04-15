#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <unordered_map>
#include "Metadata.h"
#include "CbrReplayFile.h"
#include <utility>  // std::forward

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

struct costWeights {

    std::array<float, 200> basic;
    std::array<float, 200> combo;
    std::array<float, 200> pressure;
    std::array<float, 200> blocking;
    std::array<std::string, 200> name;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& basic;
        ar& combo;
        ar& pressure;
        ar& blocking;
        ar& name;
    }
};

struct debugCaseIndex {

    int replayIndex;
    int caseIndex;
    float compValue;
};
struct bestCaseContainer {
    int replayIndex;
    int caseIndex;
    float compValue;
};
struct bestCaseSelector {

    std::vector <bestCaseContainer> bcc;
    float bestCompValue = 9999999;
    float combinedValue = 0;
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
    std::string caseSwitchReason = "";

    bestCaseSelector caseSelector = {};
    void CbrData::bestCaseCultivator(float bufComparison, int replayIndex, int caseIndex);
    bestCaseContainer CbrData::bestCaseSelector();
public:
    int debugCounter = 0;
    int framesActive = 0;
    costWeights costs;
    std::array<float, 200> curCosts;
    std::string debugText = "";
    std::vector<std::string> debugTextArr = {};
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& playerName& characterName& replayFiles & activeReplay & activeCase & activeFrame & invertCurCase & enabled;
    }
    CbrData();
    //player name,  characterName
    CbrData::CbrData(std::string n, std::string p1, int charId);
    void initalizeCosts();
    std::string getCharName();
    void setCharName(std::string);
    std::string getPlayerName();
    int getReplayCount();
    void setEnabled(bool);
    void setPlayerName(std::string name);
    bool getEnabled();
    void AddReplay(CbrReplayFile);
    void deleteReplays(int start, int end);
    std::vector<CbrReplayFile>* getReplayFiles();
    CbrReplayFile* getLastReplay();
    int CBRcomputeNextAction(Metadata*);
    int inverseInput(int);
    bool switchCaseCheck(Metadata*);
    bool sameOrNextCaseCheck(Metadata* curGamestate, int replayIndex, int caseIndex);
    bool inputSequenceCheck(Metadata* curGamestate, int replayIndex, int caseIndex);
    bool meterCheck(Metadata* curGamestate, int replayIndex, int caseIndex);
    void CbrData::resetCbr();
    
    float CbrData::comparisonFunction(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int CaseReplayIndex, int CaseCaseIndex, bool nextCaseCheck);
    float CbrData::comparisonFunctionDebug(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int CaseReplayIndex, int CaseCaseIndex, bool nextCaseCheck);

    void CbrData::debugPrint(Metadata* curM, int nextCI, int bestCI, int nextRI, int bestRI, std::vector<debugCaseIndex> dci, int FinalBestRI, int FinalBestCI);
    std::string debugPrintCompValues(CbrCase* nextC, Metadata* curM);
    float comboSpecificComp(Metadata* curGamestate, Metadata* caseGamestate, bool print);   
    int CbrData::makeCharacterID(std::string charName);
    int CbrData::getCharacterIndex();
    float CbrData::HelperCompMatch(Metadata* curGamestate, Metadata* caseGamestate);
    float CbrData::HelperComp(Metadata* curGamestate, Metadata* caseGamestate, Helper* curHelper, Helper* caseHelper, bool autoFail, bool opponent);
    recurseHelperMapping CbrData::findBestHelperMapping(std::vector<HelperMapping>& hMap, std::unordered_map<int, int>& usedUpCaseIndex, std::unordered_map<int, int>& usedUpCurIndex, int depth);
    bool OdCheck(Metadata* curGamestate, int replayIndex, int caseIndex);
    bool CbrData::caseValidityCheck(Metadata* curGamestate, int replayIndex, int caseIndex);
    bool CbrData::nextCaseValidityCheck(Metadata* curGamestate, int replayIndex, int caseIndex);
    void CbrData::resetReplayVariables();
    void CbrData::setCurGamestateCosts(Metadata* curGamestate);
};