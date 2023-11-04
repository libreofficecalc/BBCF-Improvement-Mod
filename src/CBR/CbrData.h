#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <unordered_map>
#include "Metadata.h"
#include "CbrReplayFile.h"
#include <utility>  // std::forward
#include "ComparisonFunction.h"
#include "CbrSearchTree.h"

class CbrData {
private:
    friend class boost::serialization::access;

    CbrSearchTree Searchtree;
    std::string playerName;
    std::string characterName;
    int characterIndex = -1;

    std::vector<CbrReplayFile> replayFiles;
    CbrReplayFile instantLearnReplay;

    int activeReplay = -1;
    int activeCase = -1;
    int activeFrame = -1;
    bool invertCurCase = false;
    bool activeInstantLearn = false;

    bool enabled = false;
    std::string caseSwitchReason = "";

    bestCaseSelector caseSelector = {};

    
public:
    int debugCounter = 0;
    int framesActive = 0;
    costWeights costs;
    std::array<float, 200> curCosts;
    std::string debugText = "";
    std::vector<std::string> debugTextArr = {};
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        switch (version) {
        case 0:
            a& playerName& characterName& replayFiles& activeReplay& activeCase& activeFrame& invertCurCase& enabled;
            break;
        case 3:
            a & playerName & characterName & replayFiles & activeReplay & activeCase & activeFrame & invertCurCase & enabled & Searchtree;
            break;
        default:
            a& playerName& characterName& replayFiles& activeReplay& activeCase& activeFrame& invertCurCase& enabled;
            break;
        }
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
    void AddReplay(CbrReplayFile&);
    bestCaseSelector& CbrData::FindBestCaseInTree(Metadata* curCase);
    void deleteReplays(int start, int end);
    std::vector<CbrReplayFile>* getReplayFiles();
    CbrReplayFile* getLastReplay();
    int CBRcomputeNextAction(Metadata*);
    int inverseInput(int);
    bool switchCaseCheck(Metadata* curGamestate, CbrReplayFile& replay);

    void CbrData::resetCbr();
    
    std::string CbrData::compareRandomCases();
    float CbrData::comparisonFunctionCaseDifference(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrReplayFile& curReplay, CbrCase* caseData, CbrCase* curData, int CaseReplayIndex, int CaseCaseIndex, int curReplayIndex, int curCaseIndex);

    //float CbrData::comparisonFunction(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int CaseReplayIndex, int CaseCaseIndex, bool nextCaseCheck);
    //float CbrData::comparisonFunctionDebug(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int CaseReplayIndex, int CaseCaseIndex, bool nextCaseCheck);
    //float CbrData::comparisonFunctionSlow(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int replayIndex, int caseIndex, bool nextCaseCheck);
    //float CbrData::comparisonFunctionSlowDebug(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int replayIndex, int caseIndex, bool nextCaseCheck);

    void CbrData::debugPrint(Metadata* curM, int nextCI, int bestCI, int nextRI, int bestRI, std::vector<debugCaseIndex>& dci, int FinalBestRI, int FinalBestCI, CbrReplayFile& nextReplayFile, CbrReplayFile& bestReplayFile, bool nextInstantLearnSameReplay, bool bestInstantLearnSameReplay);
    std::string debugPrintCompValues(CbrCase* nextC, Metadata* curM);
    float comboSpecificComp(Metadata* curGamestate, Metadata* caseGamestate, bool print);   
    int CbrData::makeCharacterID(std::string charName);
    int CbrData::getCharacterIndex();
    
    recurseHelperMapping CbrData::findBestHelperMapping(std::vector<HelperMapping>& hMap, std::unordered_map<int, int>& usedUpCaseIndex, std::unordered_map<int, int>& usedUpCurIndex, int depth);
    void CbrData::resetReplayVariables();
    void setInstantLearnReplay(CbrReplayFile&);
    CbrReplayFile* CbrData::getInstantLearnReplay();
    void resetInstantLearning();

    void CbrData::generateTreeFromOldReplay();
    void CbrData::generateTreeFromOldReplayInRange(int start, int end);
};
BOOST_CLASS_VERSION(CbrData, 3)