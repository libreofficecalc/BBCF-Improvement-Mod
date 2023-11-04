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
#include "CbrSearchTree.h"



#define specialButton 512
#define tauntButton 256
#define DButton 128
#define CButton 64
#define BButton 32
#define AButton 16


CbrData::CbrData()
{
    initalizeCosts(costs);
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
    initalizeCosts(costs);
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

void CbrData::AddReplay(CbrReplayFile& f)
{
    replayFiles.push_back(f);
    int lastIndex = replayFiles.size()-1;
    for (int i = 0; i < replayFiles[lastIndex].getCaseBaseLength(); i++) {
        Searchtree.insertNode(replayFiles[lastIndex].getCase(i)->getMetadata(), i, lastIndex, replayFiles, costs, curCosts);
    }
    return;
}

bestCaseSelector& CbrData::FindBestCaseInTree(Metadata* curCase) {
    static CbrSearchTree::replaySystemData rsd;
    rsd.activeCase = activeCase;
    rsd.activeReplay = activeReplay;
    rsd.framesActive = framesActive;
    Searchtree.findCaseInTree(curCase, replayFiles, costs, curCosts, caseSelector, rsd);
    return caseSelector;
}

void CbrData::deleteReplays(int start, int end) {
    if (end <= 0) { return; }
    if (start >= replayFiles.size()) { return; }
    if (end >= replayFiles.size()) { end = replayFiles.size() - 1; }
    for (int i = end; i >= start; i--) {
        Searchtree.deleteReplay(i,replayFiles);
    }
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







int CbrData::CBRcomputeNextAction(Metadata* curGamestate) {
    float bestComparison = 9999999;
    int bestReplay = -1;
    int bestCase = -1;
    int bestFrame = -1;
    int bestCompBufferCase = -2;
    int bestCompBufferReplay = -2;
    static bool transitionWaiting = false;
    static auto transitionState = curGamestate->getCurrentActionHash()[0];
    static int lastInput = 5;

    std::vector<debugCaseIndex> dci;

    framesActive++;
    
    float bufComparison = 9999999;
    activeFrame++;


    CbrReplayFile* replay = &instantLearnReplay;
    if (activeReplay != -1 && !activeInstantLearn && replayFiles.size() >= 1) {
         replay = &replayFiles[activeReplay];
    }

    //If the AI misses an attack that was meant to hit we wait untill the attack is done whiffing or untill it hits the opponent to search for the next case
    //No current case
    auto b0 = activeReplay == -1;
    //CurrentCase ending
    auto b1 = !b0 && activeFrame > replay->getCase(activeCase)->getEndIndex();
    if (curGamestate->getAttack()[0] == true && curGamestate->getNeutral()[0] != true && !transitionWaiting && (!b0 && b1 && replay->getCaseBaseLength() > activeCase + 1 && (curGamestate->getHit()[1] == false && curGamestate->getBlocking()[1] == false) && (replay->getCase(activeCase + 1)->getMetadata()->getHit()[1] == true || replay->getCase(activeCase + 1)->getMetadata()->getBlocking()[1] == true))) {
        transitionWaiting = true;
        transitionState = curGamestate->getCurrentActionHash()[0];
    }
    if (transitionWaiting == true) {
        if (curGamestate->getNeutral()[0] == true || curGamestate->getCurrentAction()[0] == "CmnActFDash" || curGamestate->getCurrentActionHash()[0] != transitionState || curGamestate->getHit()[1] || curGamestate->getBlocking()[1]) {
            transitionWaiting = false;
        }
        else {
            return lastInput;
        }
    }

    //if we are searching for a case for the first time or the case is done playing or the player is hit/lands a hit search for a new case
    if (switchCaseCheck(curGamestate, *replay)){
        caseSelector.bcc.clear();
        caseSelector.bestCompValue = 9999999;
        caseSelector.combinedValue = 0;
        setCurGamestateCosts(curGamestate, costs, curCosts);
        auto replayFileSize = replayFiles.size();

        static CbrSearchTree::replaySystemData rsd;
        rsd.activeCase = activeCase;
        rsd.activeReplay = activeReplay;
        rsd.framesActive = framesActive;
        rsd.instantLearnSameReplay = !activeInstantLearn;
        Searchtree.findCaseInTree(curGamestate, replayFiles, costs, curCosts, caseSelector, rsd);
        
        //
        auto caseBaseSize = instantLearnReplay.getCaseBase()->size();
        for (std::size_t j = 0; j < caseBaseSize; ++j) {
            //caseValidityCheck(curGamestate, i,j)
            auto caseGamestate = instantLearnReplay.getCase(j)->getMetadata();
            bufComparison = comparisonFunctionQuick(curGamestate, caseGamestate, instantLearnReplay, instantLearnReplay.getCase(j), 0, j, false, curCosts, framesActive, activeReplay, activeCase, activeInstantLearn);
            bufComparison += comparisonFunction(curGamestate, caseGamestate, instantLearnReplay, instantLearnReplay.getCase(j), 0, j, false, curCosts, framesActive, activeReplay, activeCase);
            //TODO: remove this after debugging due to massive performance hit.
            //bufComparison += HelperCompMatch(curGamestate, caseGamestate);
            //debugCaseIndex newDci{};
            //newDci.compValue = bufComparison;
            //newDci.caseIndex = j;
            //newDci.replayIndex = i;
            //updateDebugCaseIndex(dci, newDci);
            if (bufComparison <= bestComparison + maxRandomDiff) {
                //only doing helperComp if this even has a chance to be the best because it is costly
                bufComparison += comparisonFunctionSlow(curGamestate, caseGamestate, instantLearnReplay, instantLearnReplay.getCase(j), 0, j, false, curCosts);
                bestCaseCultivator(bufComparison, 0, j, caseSelector, true);

            }

        }

        CbrReplayFile* bestReplayFile;
        auto selectedCase = bestCaseSelectorFunc(caseSelector);
        bestReplayFile = &instantLearnReplay;
        bestComparison = selectedCase.compValue;
        bestReplay = selectedCase.replayIndex;
        bestCase = selectedCase.caseIndex;
        auto bestInstantLearn = selectedCase.instantLearn;
        bool bestInstantLearnSameReplay = activeInstantLearn == bestInstantLearn;
        bool nextInstantLearnSameReplay = true;
        if (bestReplay != -1) {
            if (!selectedCase.instantLearn) {
                bestReplayFile = &replayFiles[selectedCase.replayIndex];
            }
            bestFrame = bestReplayFile->getCase(selectedCase.caseIndex)->getStartingIndex();
        }

        CbrReplayFile* nextReplayFile = &instantLearnReplay;
        if (activeReplay != -1 && !activeInstantLearn) {
            nextReplayFile = &replayFiles[activeReplay];
        }
        else {
            nextReplayFile = &instantLearnReplay;
        }
        if (activeReplay != -1 && activeCase +1 <= nextReplayFile->getCaseBaseLength() -1 ){
            float bufComparison = 9999999;
            float bufComparison2 = 9999999; 
            bufComparison = comparisonFunctionQuick(curGamestate, nextReplayFile->getCase(activeCase + 1)->getMetadata(), *nextReplayFile, nextReplayFile->getCase(activeCase + 1), activeReplay, activeCase + 1, true, curCosts, framesActive, activeReplay, activeCase, nextInstantLearnSameReplay);
            bufComparison += comparisonFunction(curGamestate, nextReplayFile->getCase(activeCase + 1)->getMetadata(), *nextReplayFile, nextReplayFile->getCase(activeCase + 1), activeReplay, activeCase+1, true, curCosts, framesActive,  activeReplay, activeCase);
            bufComparison += comparisonFunctionSlow(curGamestate, nextReplayFile->getCase(activeCase + 1)->getMetadata(), *nextReplayFile, nextReplayFile->getCase(activeCase + 1), activeReplay, activeCase + 1, true, curCosts);
            if (bestReplay == -1 && bestCase == -1) {
                if (bufComparison < bestComparison) {
                    bufComparison2 = 9999999;
                }
            }
            else {
                if (!checkBestCaseBetterThanNext(bufComparison, bestComparison, nextReplayFile->getCase(activeCase + 1))) {
                    debugPrint(curGamestate, activeCase + 1, bestCase, activeReplay, bestReplay, dci, activeReplay, activeCase + 1, *nextReplayFile, *bestReplayFile, nextInstantLearnSameReplay, bestInstantLearnSameReplay);
                }
                else {
                    debugPrint(curGamestate, activeCase + 1, bestCase, activeReplay, bestReplay, dci, bestReplay, bestCase, *nextReplayFile, *bestReplayFile, nextInstantLearnSameReplay, bestInstantLearnSameReplay);
                }
                if (!checkBestCaseBetterThanNext(bufComparison, bestComparison, nextReplayFile->getCase(activeCase + 1))) {
                    debugPrint(curGamestate, activeCase + 1, bestCase, activeReplay, bestReplay, dci, activeReplay, activeCase + 1, *nextReplayFile, *bestReplayFile, nextInstantLearnSameReplay, bestInstantLearnSameReplay);
                }
                else {
                    debugPrint(curGamestate, activeCase + 1, bestCase, activeReplay, bestReplay, dci, bestReplay, bestCase, *nextReplayFile, *bestReplayFile, nextInstantLearnSameReplay, bestInstantLearnSameReplay);
                }
            }


            if (!checkBestCaseBetterThanNext(bufComparison, bestComparison, nextReplayFile->getCase(activeCase + 1))) {
                bestReplay = activeReplay;
                bestComparison = bufComparison;
                bestCase = activeCase + 1;
                bestFrame = nextReplayFile->getCase(bestCase)->getStartingIndex();
                bestInstantLearn = activeInstantLearn;
            }
        }
        else {
            if (bestCase != -1) {
                debugPrint(curGamestate, activeCase + 1, bestCase, activeReplay, bestReplay, dci, bestReplay, bestCase, *nextReplayFile, *bestReplayFile, nextInstantLearnSameReplay, bestInstantLearnSameReplay);
            }
            
        }

        activeReplay = bestReplay;
        activeCase = bestCase;
        activeFrame = bestFrame;
        activeInstantLearn = bestInstantLearn;
        
        CbrReplayFile* replay;
        replay = &instantLearnReplay;
        if (activeFrame != -1) {
            if (!activeInstantLearn) {
                replay = &replayFiles[activeReplay];
            }
            else {
                replay = &instantLearnReplay;
            }

            invertCurCase = replay->getCase(bestCase)->getMetadata()->getFacing() != curGamestate->getFacing();
            if (activeReplay != -1 && activeCase != -1) {
                replay->getCase(activeCase)->caseCooldownFrameStart = framesActive;
            }
        }
        
        
        
    }

    
    if (activeFrame == -1) { return 5; }
    replay = &instantLearnReplay;
    if (!activeInstantLearn) {
        replay = &replayFiles[activeReplay];
    }
    auto input = replay->getInput(activeFrame);
    
    //replayFiles[activeReplay].getCase(activeCase)->getMetadata().getFacing() != curGamestate->getFacing()
    if (invertCurCase) {
        input = inverseInput(input);
    }
    lastInput = input;
    return input;
}
void CbrData::resetCbr() {
    activeReplay = -1;
    activeCase = -1;
    activeFrame = -1;
    invertCurCase = false;
}

bool CbrData::switchCaseCheck(Metadata* curGamestate, CbrReplayFile& replay) {
    //No current case
    auto b0 = activeReplay == -1;
    //CurrentCase ending
    auto b1 = !b0 && activeFrame > replay.getCase(activeCase)->getEndIndex();
    //Self hit
    auto b2 = !b0 && curGamestate->getHitThisFrame()[0];
    //Self Block
    auto b3 = !b0 && curGamestate->getBlockThisFrame()[0];
    //Hitting the opponent while not buffering an input
    auto b4 = !b0 && (curGamestate->getHitThisFrame()[1] || curGamestate->getBlockThisFrame()[1]) && !replay.getCase(activeCase)->getMetadata()->getInputBufferActive();

    caseSwitchReason = "";
    if (b0) { caseSwitchReason += "No current case - "; }
    if (b1) { caseSwitchReason += "Case Ending - "; }
    if (b2) { caseSwitchReason += "Self hit - "; }
    if (b3) { caseSwitchReason += "Self Block - "; }
    if (b4) { caseSwitchReason += "HitOpponent"; }
    return b0 || b1 || b2 || b3 || b4;
}



std::string CbrData::compareRandomCases() {
    if (replayFiles.size() == 0) {
        return "NoData";
    }
    float average = 0;
    float combined = 0;
    int comparisonCounter = 0;
    float highest = 0;

    for (int i = 0; i < replayFiles.size(); i++) {
        for (int j = 0; j < replayFiles[i].getCaseBaseLength(); j++) {
            for (int o = 0; o < replayFiles.size(); o++) {
                for (int k = 0; k < replayFiles[o].getCaseBaseLength(); k++) {
                    int curReplay = i;
                    int curCase = j;
                    int caseReplay = o;
                    int caseCase = k;
                    auto val = comparisonFunctionCaseDifference(replayFiles[curReplay].getCase(curCase)->getMetadata(), replayFiles[caseReplay].getCase(caseCase)->getMetadata(), replayFiles[caseReplay], replayFiles[curReplay], replayFiles[caseReplay].getCase(caseCase), replayFiles[curReplay].getCase(curCase), caseReplay, caseCase, curReplay, curCase);
                    combined += val;
                    comparisonCounter++;
                    if (val > highest) { highest = val; }
                    average = combined / comparisonCounter;

                }
            }
        }
    }


    return "Average: " + std::to_string(average) + " - Highest: " + std::to_string(highest);
}

float CbrData::comparisonFunctionCaseDifference(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrReplayFile& curReplay, CbrCase* caseData, CbrCase* curData, int CaseReplayIndex, int CaseCaseIndex, int curReplayIndex, int curCaseIndex) {
    curCosts = costs.basic;
    auto val1 = comparisonFunction(curGamestate, caseGamestate, caseReplay, caseData, CaseReplayIndex, CaseCaseIndex, true, curCosts, framesActive, activeReplay, activeCase);
    val1 += comparisonFunctionSlow(curGamestate, caseGamestate, caseReplay, caseData, CaseReplayIndex, CaseCaseIndex, true, curCosts);
    auto val2 = comparisonFunction(caseGamestate, curGamestate, curReplay, curData, curReplayIndex, curCaseIndex, true, curCosts, framesActive, activeReplay, activeCase);
    val2 += comparisonFunctionSlow(caseGamestate, curGamestate, curReplay, curData, curReplayIndex, curCaseIndex, true, curCosts);
    
    
    return abs(val1 - val2);
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
    else {
        if (buffer == 4 || buffer == 7 || buffer == 1) {
            return input + 2;
        }
    }

    
    return input;
}




void CbrData::debugPrint(Metadata* curM, int nextCI, int bestCI, int nextRI, int bestRI, std::vector<debugCaseIndex>& dci, int FinalBestRI, int FinalBestCI, CbrReplayFile& nextReplayFile, CbrReplayFile& bestReplayFile, bool nextInstantLearnSameReplay, bool bestInstantLearnSameReplay) {
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
    if (nextRI != -1 && nextCI <= nextReplayFile.getCaseBaseLength() - 1) {
        nextCompVal = comparisonFunctionDebug(curM, nextReplayFile.getCase(nextCI)->getMetadata(), nextReplayFile, nextReplayFile.getCase(nextCI), nextRI, nextCI, true, curCosts, framesActive, activeReplay, activeCase, debugText, nextInstantLearnSameReplay);
        
        nextCompVal +=  comparisonFunctionSlowDebug(curM, nextReplayFile.getCase(nextCI)->getMetadata(), nextReplayFile, nextReplayFile.getCase(nextCI), nextRI, nextCI, true, curCosts, debugText);
        debugTextArr.push_back(debugText);
        debugText = "";


        str += "Next Cost: " + std::to_string(nextCompVal);
        str += "\nReplay: " + std::to_string(nextRI) + " - Case: " + std::to_string(nextCI) + " - Start: " + std::to_string(nextReplayFile.getCase(nextCI)->getStartingIndex()) + " - End: " + std::to_string(nextReplayFile.getCase(nextCI)->getEndIndex()) + "\n";
        //str += debugPrintCompValues(nextC, curM);
        for (int k = nextReplayFile.getCase(nextCI)->getStartingIndex(); k <= nextReplayFile.getCase(nextCI)->getEndIndex(); k++) {
            str += std::to_string(nextReplayFile.getInput(k)) + ", ";
        }
        str += "\n\n";
    }
    else {
        str += "No next Case available\n\n";
    }
    
    
    
    debugTextArr.push_back(str);
    str = "";

    auto bestCompVal = comparisonFunctionDebug(curM, bestReplayFile.getCase(bestCI)->getMetadata(), bestReplayFile, bestReplayFile.getCase(bestCI), bestRI, bestCI,false, curCosts, framesActive, activeReplay, activeCase, debugText, bestInstantLearnSameReplay);
    bestCompVal += comparisonFunctionSlowDebug(curM, bestReplayFile.getCase(bestCI)->getMetadata(), bestReplayFile, bestReplayFile.getCase(bestCI), bestRI, bestCI, false, curCosts, debugText);
    if (helperVal > 0) {
        debugText += "Helper Cost: " + std::to_string(helperVal);
    }
    debugTextArr.push_back(debugText);
    debugText = "";

    //str += "\n" + nextC->getMetadata()->PrintState();

    str += "Best Cost: " + std::to_string(bestCompVal);
    str += "\nReplay: " + std::to_string(bestRI) + " - Case: " + std::to_string(bestCI) + " - Start: " + std::to_string(bestReplayFile.getCase(bestCI)->getStartingIndex()) + " - End: " + std::to_string(bestReplayFile.getCase(bestCI)->getEndIndex()) + "\n";
    //str += debugPrintCompValues(bestC, curM);
    for (int k = bestReplayFile.getCase(bestCI)->getStartingIndex(); k <= bestReplayFile.getCase(bestCI)->getEndIndex(); k++) {
        str += std::to_string(bestReplayFile.getInput(k)) + ", ";
    }
    str += "\n\n";
    debugTextArr.push_back(str);
    str = "";

    if (bestCompVal >= nextCompVal && (nextCI != FinalBestCI || nextRI != FinalBestRI)) {
        str += "huh?\n\n";
        debugTextArr.push_back(str);
        if (nextReplayFile.getCaseBaseLength() > nextCI) {
            checkBestCaseBetterThanNext(nextCompVal, bestCompVal, nextReplayFile.getCase(nextCI));
            bestCompVal = comparisonFunctionDebug(curM, bestReplayFile.getCase(bestCI)->getMetadata(), bestReplayFile, bestReplayFile.getCase(bestCI), bestRI, bestCI, false, curCosts, framesActive, activeReplay, activeCase, debugText, bestInstantLearnSameReplay);
            auto testCompVal = comparisonFunctionQuick(curM, bestReplayFile.getCase(bestCI)->getMetadata(), bestReplayFile, bestReplayFile.getCase(bestCI), bestRI, bestCI, false, curCosts, framesActive, activeReplay, activeCase, bestInstantLearnSameReplay);
            testCompVal += comparisonFunction(curM, bestReplayFile.getCase(bestCI)->getMetadata(), bestReplayFile, bestReplayFile.getCase(bestCI), bestRI, bestCI, false, curCosts, framesActive, activeReplay, activeCase);
            testCompVal += comparisonFunctionSlow(curM, bestReplayFile.getCase(bestCI)->getMetadata(), bestReplayFile, bestReplayFile.getCase(bestCI), bestRI, bestCI, false, curCosts);
            static CbrSearchTree::replaySystemData rsd;
            rsd.activeCase = activeCase;
            rsd.activeReplay = activeReplay;
            rsd.framesActive = framesActive;
            rsd.instantLearnSameReplay = !activeInstantLearn;
            Searchtree.findCaseInTree(curM, replayFiles, costs, curCosts, caseSelector, rsd);
            testCompVal += 0;
        }
        
    }

    for (int i = 0; i < 30 && i < dci.size(); i++) {
        auto bestCompVal = comparisonFunctionDebug(curM, replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex)->getMetadata(), replayFiles[dci[i].replayIndex], replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex), dci[i].replayIndex, dci[i].caseIndex, false, curCosts, framesActive, activeReplay, activeCase, debugText, bestInstantLearnSameReplay);
        bestCompVal += comparisonFunctionSlowDebug(curM, replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex)->getMetadata(), replayFiles[dci[i].replayIndex], replayFiles[dci[i].replayIndex].getCase(dci[i].caseIndex), dci[i].replayIndex, dci[i].caseIndex, false, curCosts, debugText);
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

    str += "Cost Helper: " + std::to_string(HelperCompMatch(curM, nextC->getMetadata(), curCosts));
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

void CbrData::setInstantLearnReplay(CbrReplayFile& r)
{
    instantLearnReplay = r;
}

CbrReplayFile* CbrData::getInstantLearnReplay()
{

    return  &instantLearnReplay;

}

void CbrData::resetInstantLearning() {
    instantLearnReplay = {};
    return;
}

void CbrData::generateTreeFromOldReplay(){
    Searchtree.generateTreeFromOldReplay(replayFiles, costs, curCosts);
}

void CbrData::generateTreeFromOldReplayInRange(int start, int end) {
    Searchtree.generateTreeFromOldReplayInRange(replayFiles, costs, curCosts, start, end);
}
