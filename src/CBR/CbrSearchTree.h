#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <array>
#include <boost/serialization/shared_ptr.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <unordered_map>
#include <map>
#include "cbrData.h"
#include <fstream>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <future>

class CbrSearchTree {
    struct treePos {
        int y;
        int x;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& a, const unsigned version) {
            a& y& x;
        }
    };
    struct treeNode {             // Structure declaration
        int cbrIndex = -1;
        int cbrReplayIndex = -1;
        struct std::vector<treePos> childNodes;
        //struct std::vector<treePos> parrentNodes;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& a, const unsigned version) {
            a& cbrIndex& cbrReplayIndex & childNodes;
        }
    };       // Structure variable


    struct candidateNode {
        treePos treePos;
        float similarity;
    };

private:
    int maxComparisonNr = 3000;
    int maxComparisonNrAlternatives = 2000;
    std::vector<std::vector<treeNode>> tree;
    std::vector <float> treeCompValues;
    std::vector<std::unordered_map<int, bool>> candidates;
    std::vector<std::unordered_map<int, bool>> parrentlessCandidates;

    std::vector<candidateNode> caseList;
    std::vector<candidateNode> exploredCandidates{};

    std::vector<CbrReplayFile> deletedCases;

    friend class boost::serialization::access;


public:
    
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& tree & treeCompValues;
    }
    CbrSearchTree() {};
    float insertionCompFunc(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int replayIndex, int caseIndex, bool nextCaseCheck, costWeights& costs, std::array<float, 200>& curCosts, std::vector<CbrReplayFile>& replayFiles) {

        setCurGamestateCosts(curGamestate, costs, curCosts);
        auto comparisonResult = comparisonFunction(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts, -1, -1, -1);
        comparisonResult += comparisonFunctionSlow(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts);
        setCurGamestateCosts(caseGamestate, costs, curCosts);
        auto comparisonResult2 = comparisonFunction(caseGamestate, curGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts, -1, -1, -1);
        comparisonResult2 += comparisonFunctionSlow(caseGamestate, curGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts);

        if (comparisonResult < comparisonResult2) {
            comparisonResult = comparisonResult2;
        }

        return comparisonResult;
    }
    
    void searchInsertionSpot(Metadata* curGamestate, int& comparisonNr, std::vector<CbrReplayFile>& replayFiles, int y, int x, candidateNode& candidate, std::vector<float>& treeRadius, costWeights& costs, std::array<float, 200>& curCosts) {

        comparisonNr++;
        float cAdvance = 0.00;
        float cRadius = 0;
        cAdvance = treeRadius[y];
        if (y - 1 >= 0) { cRadius = treeRadius[y - 1]; }

        int caseIndex = tree[y][x].cbrIndex;
        int replayIndex = tree[y][x].cbrReplayIndex;
        auto caseGamestate = replayFiles[replayIndex].getCase(caseIndex)->getMetadata();
        auto& caseReplay = replayFiles[replayIndex];
        auto caseData = replayFiles[replayIndex].getCase(caseIndex);

        float dissimilarity = insertionCompFunc(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, costs, curCosts, replayFiles);

        if (dissimilarity + cRadius <= cAdvance && y != 0) {
            if (candidate.treePos.y > y || candidate.treePos.y == y && candidate.similarity < dissimilarity) {
                candidate.treePos.y = y;
                candidate.treePos.x = x;
                candidate.similarity = dissimilarity;
            }
            for (int i = 0; i < tree[y][x].childNodes.size(); i++) {
                searchInsertionSpot(curGamestate, comparisonNr, replayFiles, tree[y][x].childNodes[i].y, tree[y][x].childNodes[i].x, candidate, treeRadius, costs, curCosts);
            }
        }

        
    }
    struct replaySystemData {
        int framesActive = 0;
        int activeReplay = 0;
        int activeCase = 0;
        bool instantLearnSameReplay = false;
    };
    void searchSimilarNode(int& comparisonNr, Metadata* curGamestate, std::vector<CbrReplayFile>& replayFilesIn, int y, int x, candidateNode& bestCandidate, std::vector<candidateNode>& unexploredCandidates, std::vector<float>& treeRadius, costWeights& costs, std::array<float, 200>& curCosts, bestCaseSelector& caseSelector, replaySystemData& rsd) {
        comparisonNr++;
        float cAdvance = 0.00;
        float cRadius = 0;
        cAdvance = treeRadius[y];
        if (y - 1 >= 0) { cRadius = treeRadius[y - 1]; }

        auto& replayFiles = replayFilesIn;
        int replayIndex = 0;
        if (tree[y][x].cbrReplayIndex == -1) {
            replayFiles = deletedCases;
        }
        else {
            replayFiles = replayFilesIn;
            replayIndex = tree[y][x].cbrReplayIndex;
        }
        int caseIndex = tree[y][x].cbrIndex;
        
        auto caseGamestate = replayFiles[replayIndex].getCase(caseIndex)->getMetadata();
        auto& caseReplay = replayFiles[replayIndex];
        auto caseData = replayFiles[replayIndex].getCase(caseIndex);
        float dissimilarity = comparisonFunction(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, false, curCosts, rsd.framesActive, rsd.activeReplay, rsd.activeCase);
        dissimilarity += comparisonFunctionSlow(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, false, curCosts);
        //exploredCandidates.push_back({ y,x,dissimilarity });

        //Extra checks which disqualify a case from beeing picked, not used for the algorithm but used to not choose a case as best.
        float dissExtra = comparisonFunctionQuick(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, false, curCosts, rsd.framesActive, rsd.activeReplay, rsd.activeCase, rsd.instantLearnSameReplay);

        if ((caseIndex != 0 || y != tree.size() - 1) && bestCandidate.similarity + maxRandomDiff >= dissimilarity + dissExtra) {
            bestCaseCultivator(dissimilarity + dissExtra, replayIndex, caseIndex, caseSelector, false);
            if (bestCandidate.similarity > dissimilarity + dissExtra) {
                bestCandidate.treePos.y = y;
                bestCandidate.treePos.x = x;
                bestCandidate.similarity = dissimilarity + dissExtra;
            }
        }

        if (dissimilarity <= cAdvance) {
            
            if (y != 0) {
                treeNode* tNode = &tree[y][x];
                int iSize = tNode->childNodes.size();
                for (int i = 0; i < iSize; i++) {
                    searchSimilarNode(comparisonNr, curGamestate, replayFiles, tNode->childNodes[i].y, tNode->childNodes[i].x, bestCandidate, unexploredCandidates, treeRadius, costs, curCosts, caseSelector, rsd);
                }
            }

        }
        else {
            if (y > 0 && bestCandidate.similarity + maxRandomDiff > (dissimilarity - cAdvance)) {
                candidateNode c;
                c.treePos.y = y;
                c.treePos.x = x;
                c.similarity = dissimilarity - cRadius;
                unexploredCandidates.push_back(c);
            }
        }
    }
    
    void searchSimilarRemainingNode(int& comparisonNr, Metadata* curGamestate, std::vector<CbrReplayFile>& replayFilesIn, std::vector<std::vector<treeNode>>& tree, int y, int x, candidateNode& bestCandidate, std::vector<candidateNode>& unexploredCandidates, std::vector<float>& treeRadius, costWeights& costs, std::array<float, 200>& curCosts, bestCaseSelector& caseSelector, replaySystemData& rsd) {
        if (comparisonNr > maxComparisonNr) { return; }
        comparisonNr++;
        float cAdvance = 0.00;
        float cRadius = 0;
        cAdvance = treeRadius[y];
        if (y - 1 >= 0) { cRadius = treeRadius[y - 1]; }

        auto& replayFiles = replayFilesIn;
        int replayIndex = 0;
        if (tree[y][x].cbrReplayIndex == -1) {
            replayFiles = deletedCases;
        }
        else {
            replayFiles = replayFilesIn;
            replayIndex = tree[y][x].cbrReplayIndex;
        }
        //cAdvance = cAdvance / 2;
        int caseIndex = tree[y][x].cbrIndex;
        auto caseGamestate = replayFiles[replayIndex].getCase(caseIndex)->getMetadata();
        auto& caseReplay = replayFiles[replayIndex];
        auto caseData = replayFiles[replayIndex].getCase(caseIndex);
        float dissimilarity = comparisonFunction(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, false, curCosts, rsd.framesActive, rsd.activeReplay, rsd.activeCase);
        dissimilarity += comparisonFunctionSlow(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, false, curCosts);

        //Extra checks which disqualify a case from beeing picked, not used for the algorithm but used to not choose a case as best.
        float dissExtra = comparisonFunctionQuick(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, false, curCosts, rsd.framesActive, rsd.activeReplay, rsd.activeCase, rsd.instantLearnSameReplay);

        if (caseIndex != 0 && bestCandidate.similarity > dissimilarity + dissExtra) {
            bestCandidate.treePos.y = y;
            bestCandidate.treePos.x = x;
            bestCandidate.similarity = dissimilarity + dissExtra;
            bestCaseCultivator(bestCandidate.similarity, replayIndex, caseIndex, caseSelector, false);
        }
        else {
            if (caseIndex != 0  && bestCandidate.similarity + maxRandomDiff > dissimilarity + dissExtra) {
                bestCaseCultivator(dissimilarity + dissExtra, replayIndex, caseIndex, caseSelector, false);
            }
        }
        
        float maxSim = bestCandidate.similarity;
        if (comparisonNr > maxComparisonNrAlternatives) {
            maxSim = bestCandidate.similarity + maxRandomDiff;
        }
        if (y > 0 && maxSim > (dissimilarity - cAdvance)) {

            for (int i = 0; i < tree[y][x].childNodes.size(); i++) {
                if (comparisonNr > maxComparisonNr) { return; }
                searchSimilarRemainingNode(comparisonNr, curGamestate, replayFiles, tree, tree[y][x].childNodes[i].y, tree[y][x].childNodes[i].x, bestCandidate, unexploredCandidates, treeRadius, costs, curCosts, caseSelector, rsd);
            }
        }
    }

    void iterateRemainingNodes(int& comparisonNr, Metadata* curGamestate, std::vector<CbrReplayFile>& replayFiles, candidateNode& bestCandidate, std::vector<candidateNode>& unexploredCandidates, std::vector<float>& treeRadius, costWeights& costs, std::array<float, 200>& curCosts, bestCaseSelector& caseSelector, replaySystemData& rsd) {
        for (int i = 0; i < unexploredCandidates.size(); i++) {
            if (unexploredCandidates[i].similarity < bestCandidate.similarity) {
                for (int j = 0; j < tree[unexploredCandidates[i].treePos.y][unexploredCandidates[i].treePos.x].childNodes.size(); j++) {
                    if (comparisonNr > maxComparisonNr) { return; }
                    searchSimilarRemainingNode(comparisonNr, curGamestate, replayFiles, tree, tree[unexploredCandidates[i].treePos.y][unexploredCandidates[i].treePos.x].childNodes[j].y, tree[unexploredCandidates[i].treePos.y][unexploredCandidates[i].treePos.x].childNodes[j].x, bestCandidate, unexploredCandidates, treeRadius, costs, curCosts, caseSelector, rsd);
                }

            }
        }
    }

    void insertTreeNode(int& cbrIndex, int& cbrReplayIndex, candidateNode& candidate) {
        treeNode newNode;
        newNode.cbrIndex = cbrIndex;
        newNode.cbrReplayIndex = cbrReplayIndex;
        tree[candidate.treePos.y - 1].push_back(newNode);
        int x = tree[candidate.treePos.y - 1].size() - 1;
        tree[candidate.treePos.y][candidate.treePos.x].childNodes.push_back({ candidate.treePos.y - 1 , x });
    }
    
    void prepareSearchTree(costWeights& costs) {
        float lowestComp = costs.lowestCost;
        float levelValue = costs.combinedCost;
        candidates = {};
        treeCompValues = {};
        parrentlessCandidates = {};
        while (levelValue > lowestComp) {
            tree.push_back(std::vector<treeNode>{});
            treeCompValues.insert(treeCompValues.begin(), levelValue);
            levelValue = (levelValue) / 2;
        }
    }

    void insertFirstNode() {
        //Create starting node

        for (int i = tree.size() - 1; i >= 0; i--) {
            tree[i].push_back(treeNode{});
            int xCur = tree[i].size() - 1;
            tree[i][xCur].cbrIndex = 0;
            tree[i][xCur].cbrReplayIndex = 0;
            if (i != tree.size() - 1) {
                int xParrent = tree[i + 1].size() - 1;
                tree[i + 1][xParrent].childNodes.push_back({ i,xCur });
            }

        }
    }

    void insertNode(Metadata* curGamestate, int& cbrIndex, int& cbrReplayIndex, std::vector<CbrReplayFile>& replayFiles, costWeights& costs, std::array<float, 200>& curCosts) {
        //if the tree is empty, prepare it
        if (tree.size() == 0 || tree[tree.size()-1].size() == 0) {
            prepareSearchTree(costs);
            insertFirstNode();
            return;
        }

        auto height = treeCompValues.size() - 1;
        auto comparisonNr = 0;
        parrentlessCandidates.clear();
        candidates.clear();
        for (int i = 0; i <= height; i++) {
            parrentlessCandidates.push_back(std::unordered_map<int, bool>{});
            candidates.push_back(std::unordered_map<int, bool>{});
        }
        int lowestCandidateLevel = height;
        int lowestCreationLevel = height + 1;
        comparisonNr = 0;

        candidateNode candidate = {};
        candidate.treePos.y = 90000;
        candidate.similarity = 100000000;
        searchInsertionSpot(curGamestate, comparisonNr, replayFiles, height, 0, candidate, treeCompValues, costs, curCosts);
        if (candidate.treePos.y == 90000) {
            searchInsertionSpot(curGamestate, comparisonNr, replayFiles, height, 0, candidate, treeCompValues, costs, curCosts);
        }
        insertTreeNode(cbrIndex, cbrReplayIndex, candidate);
    
    }

    void findCaseInTree(Metadata* curGamestate, std::vector<CbrReplayFile>& replayFilesIn, costWeights& costs, std::array<float, 200>& curCosts, bestCaseSelector& caseSelector, replaySystemData& rsd) {
        if (tree.size() == 0) { return; }
        exploredCandidates.clear();
        caseList.clear();

        candidateNode bestCandidate;
        bestCandidate.similarity = 1000000;
        std::vector<candidateNode> unexploredCandidates{};
        float bestSim = 1000000;
        int bestCaseIndex = 0;
        int bestReplayIndex = 0;
        int y = 0;
        int x = 0;

        auto& replayFiles = replayFilesIn;
        /*
        //Debug function to go through the entire tree and find the actually best candidate, allows for comparison with what the algorithm finds
        int iSize = tree.size();
        for (int i = 0; i < iSize; i++) {
            int jSize = tree[i].size();
            for (int j = 0; j < jSize; j++) {
                treeNode* tNode = &tree[i][j];
                int replayIndex = 0;
                if (tNode->cbrReplayIndex == -1) {
                     replayFiles = deletedCases;
                }
                else {
                     replayFiles = replayFilesIn;
                     replayIndex = tNode->cbrReplayIndex;
                }

                auto bufferSim = comparisonFunction(curGamestate, replayFiles[replayIndex].getCase(tNode->cbrIndex)->getMetadata(), replayFiles[replayIndex], replayFiles[replayIndex].getCase(tNode->cbrIndex), replayIndex, tNode->cbrIndex, true, curCosts, -1, -1, -1, replayFiles);
                bufferSim += comparisonFunctionSlow(curGamestate, replayFiles[replayIndex].getCase(tNode->cbrIndex)->getMetadata(), replayFiles[replayIndex], replayFiles[replayIndex].getCase(tNode->cbrIndex), replayIndex, tNode->cbrIndex, true, curCosts);
                bufferSim += comparisonFunctionQuick(curGamestate, replayFiles[replayIndex].getCase(tNode->cbrIndex)->getMetadata(), replayFiles[replayIndex], replayFiles[replayIndex].getCase(tNode->cbrIndex), replayIndex, tNode->cbrIndex, false, curCosts, -1, -1, -1);
                if (bestSim > bufferSim && tNode->cbrReplayIndex != -1) {
                    bestSim = bufferSim;
                    bestCaseIndex = tNode->cbrIndex;
                    bestReplayIndex = tNode->cbrReplayIndex;
                    y = i;
                    x = j;
                }
            }

        }
        caseList.push_back({ y,x, bestSim });

        
        //Still for debugging: creates the optimal search path that could have been used to find the best node
        while (y < tree.size() - 1) {
            for (int i = 0; i < tree.size(); i++) {
                if (tree[i].size() == 0 && y < i) { y = i; }
                for (int j = 0; j < tree[i].size(); j++) {
                    for (int o = 0; o < tree[i][j].childNodes.size(); o++) {
                        if (tree[tree[i][j].childNodes[o].y][tree[i][j].childNodes[o].x].cbrIndex == bestCaseIndex && tree[tree[i][j].childNodes[o].y][tree[i][j].childNodes[o].x].cbrReplayIndex == bestReplayIndex) {
                            bestCaseIndex = tree[i][j].cbrIndex;
                            bestReplayIndex = tree[i][j].cbrReplayIndex;
                            auto val = comparisonFunction(curGamestate, replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex)->getMetadata(), replayFiles[tree[i][j].cbrReplayIndex], replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex), tree[i][j].cbrReplayIndex, tree[i][j].cbrIndex, true, curCosts, -1, -1, -1, replayFiles);
                            val += comparisonFunctionSlow(curGamestate, replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex)->getMetadata(), replayFiles[tree[i][j].cbrReplayIndex], replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex), tree[i][j].cbrReplayIndex, tree[i][j].cbrIndex, true, curCosts);
                            caseList.push_back({ i,j, val });
                            y = i;

                        }
                    }

                }
            }
        }
        */

        int comparisonNr = 0;

        searchSimilarNode(comparisonNr, curGamestate, replayFiles, tree.size() - 1, 0, bestCandidate, unexploredCandidates, treeCompValues, costs, curCosts, caseSelector, rsd);
        iterateRemainingNodes(comparisonNr, curGamestate, replayFiles, bestCandidate, unexploredCandidates, treeCompValues, costs, curCosts, caseSelector, rsd);

        if (bestCandidate.similarity > bestSim) {
            std:printf("Asd");
        }
        int cbrIndex = tree[bestCandidate.treePos.y][bestCandidate.treePos.x].cbrIndex;
        int cbrReplayIndex = tree[bestCandidate.treePos.y][bestCandidate.treePos.x].cbrReplayIndex;
        return; 
    }
    
    void deletionIndexUpdate(int deletedX, int y) {
        int size = 0;
        int yUp = y + 1;
        if (tree.size() > y + 1) { size = tree[y + 1].size(); }
        else { return; }
        for (int i = 0; i < size; i++) {
            for (int j = tree[yUp][i].childNodes.size()-1; j >= 0; j--) {
                if (tree[yUp][i].childNodes[j].x == deletedX) {
                    tree[yUp][i].childNodes.erase(tree[yUp][i].childNodes.begin() + j);
                }
                else {
                    if (tree[yUp][i].childNodes[j].x > deletedX) {
                        tree[yUp][i].childNodes[j].x--;
                    }
                }

            }
        }
    }

    bool deleteionIteration(int y, int x, int caseReplayIndex, std::vector<CbrReplayFile>& replayFiles) {
        for (int i = tree[y][x].childNodes.size()-1; i >= 0; i--) {
            int childY = tree[y][x].childNodes[i].y;
            int childX = tree[y][x].childNodes[i].x;
            deleteionIteration(childY, childX, caseReplayIndex, replayFiles);
            //if (deleteionIteration(childY, childX, caseReplayIndex, replayFiles) == true) {
            //    tree[y][x].childNodes.erase(tree[y][x].childNodes.begin() + i);
            //}
        }
        
        if (tree[y][x].cbrReplayIndex > caseReplayIndex) {
            tree[y][x].cbrReplayIndex = tree[y][x].cbrReplayIndex - 1;
        }
        else {
            if (tree[y][x].cbrReplayIndex == caseReplayIndex) {

                if (tree[y][x].childNodes.size() == 0) {
                    tree[y].erase(tree[y].begin() + x);
                    deletionIndexUpdate(x, y);
                    return true;
                }
                else {
                    deletedCases[0].getCaseBase()->push_back(std::move(replayFiles[tree[y][x].cbrReplayIndex].getCaseBase()->at(tree[y][x].cbrIndex)));
                    tree[y][x].cbrReplayIndex = -1;
                    tree[y][x].cbrIndex = deletedCases.size() - 1;
                    return false;
                }
            }
        }
    }

    void deleteReplay(int caseReplayIndex, std::vector<CbrReplayFile>& replayFiles) {
        int startY = tree.size() - 1;
        for (int i = 0; i < tree[startY].size(); i++) {
            deleteionIteration(startY, i, caseReplayIndex, replayFiles);
        }
        if (tree[tree.size() - 1].size() <= 0) {
            tree = {};
        }
        
        
 
    }
    

    void generateTreeFromOldReplay(std::vector<CbrReplayFile>& replays, costWeights& costs, std::array<float, 200>& curCosts) {
        if (tree.size() != 0) {
            return;
        }
        int replayFileAmount = replays.size();
        for (int i = 0; i < replayFileAmount; i++) {
            int caseAmount = replays.at(i).getCaseBaseLength();
            for (int j = 0; j < caseAmount; j++) {
                try {
                    insertNode(replays[i].getCase(j)->getMetadata(), j, i, replays, costs, curCosts);
                }
                catch (int i) {
                    std::cout << "exception happended!\n";
                }
                
            }
        }
    }

    void generateTreeFromOldReplayInRange(std::vector<CbrReplayFile>& replays, costWeights& costs, std::array<float, 200>& curCosts, int start, int end) {
        int replayFileAmount = end;
        for (int i = start; i < replayFileAmount; i++) {
            int caseAmount = replays.at(i).getCaseBaseLength();
            for (int j = 0; j < caseAmount; j++) {
                try {
                    insertNode(replays[i].getCase(j)->getMetadata(), j, i, replays, costs, curCosts);
                }
                catch (int i) {
                    std::cout << "exception happended!\n";
                }

            }
        }
    }

};
