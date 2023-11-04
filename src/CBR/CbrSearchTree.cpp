#include "CbrSearchTree.h"


float CbrSearchTree::insertionCompFunc(Metadata* curGamestate, Metadata* caseGamestate, CbrReplayFile& caseReplay, CbrCase* caseData, int replayIndex, int caseIndex, bool nextCaseCheck, costWeights& costs, std::array<float, 200>& curCosts, std::vector<CbrReplayFile>& replayFiles) {

    setCurGamestateCosts(curGamestate, costs, curCosts);
    auto comparisonResult = comparisonFunction(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts, -1, -1, -1, replayFiles);
    comparisonResult += comparisonFunctionSlow(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts);
    setCurGamestateCosts(caseGamestate, costs, curCosts);
    auto comparisonResult2 = comparisonFunction(caseGamestate, curGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts, -1, -1, -1, replayFiles);
    comparisonResult2 += comparisonFunctionSlow(caseGamestate, curGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts);

    if (comparisonResult < comparisonResult2) {
        comparisonResult = comparisonResult2;
    }

    return comparisonResult;
}

void CbrSearchTree::searchInsertionSpot(Metadata* curGamestate, int& comparisonNr, std::vector<CbrReplayFile> replayFiles, int y, int x, candidateNode& candidate, std::vector<float>& treeRadius, costWeights& costs, std::array<float, 200>& curCosts) {
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
            searchInsertionSpot(curGamestate, comparisonNr, replayFiles, y, x, candidate, treeRadius, costs, curCosts);
        }
    }
}



void CbrSearchTree::searchSimilarNode(int& comparisonNr, Metadata* curGamestate, std::vector<CbrReplayFile> replayFiles, int y, int x, candidateNode& bestCandidate, std::vector<candidateNode>& unexploredCandidates, std::vector<float>& treeRadius, costWeights& costs, std::array<float, 200>& curCosts) {
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
    float dissimilarity = comparisonFunction(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts, -1, -1, -1, replayFiles);
    dissimilarity += comparisonFunctionSlow(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts);
    //exploredCandidates.push_back({ y,x,dissimilarity });

    //Extra checks which disqualify a case from beeing picked, not used for the algorithm but used to not choose a case as best.
    float dissExtra = comparisonFunctionQuick(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, false, curCosts, -1, -1, -1, replayFiles);

    if (bestCandidate.similarity > dissimilarity + dissExtra) {
        bestCandidate.treePos.y = y;
        bestCandidate.treePos.x = x;
        bestCandidate.similarity = dissimilarity + dissExtra;
    }

    if (dissimilarity <= cAdvance) {

        if (y != 0) {
            for (int i = 0; i < tree[y][x].childNodes.size(); i++) {
                searchSimilarNode(comparisonNr, curGamestate, replayFiles, tree[y][x].childNodes[i].y, tree[y][x].childNodes[i].x, bestCandidate, unexploredCandidates, treeRadius, costs, curCosts);
            }
        }

    }
    else {
        if (y > 0 && bestCandidate.similarity > (dissimilarity - cAdvance)) {
            candidateNode c;
            c.treePos.y = y;
            c.treePos.x = x;
            c.similarity = dissimilarity - cRadius;
            unexploredCandidates.push_back(c);
        }
    }



}

void CbrSearchTree::searchSimilarRemainingNode(int& comparisonNr, Metadata* curGamestate, std::vector<CbrReplayFile> replayFiles, std::vector<std::vector<treeNode>>& tree, int y, int x, candidateNode& bestCandidate, std::vector<candidateNode>& unexploredCandidates, std::vector<float>& treeRadius, costWeights& costs, std::array<float, 200>& curCosts) {
    comparisonNr++;
    float cAdvance = 0.00;
    float cRadius = 0;
    cAdvance = treeRadius[y];
    if (y - 1 >= 0) { cRadius = treeRadius[y - 1]; }

    //cAdvance = cAdvance / 2;
    int caseIndex = tree[y][x].cbrIndex;
    int replayIndex = tree[y][x].cbrReplayIndex;
    auto caseGamestate = replayFiles[replayIndex].getCase(caseIndex)->getMetadata();
    auto& caseReplay = replayFiles[replayIndex];
    auto caseData = replayFiles[replayIndex].getCase(caseIndex);
    float dissimilarity = comparisonFunction(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts, -1, -1, -1, replayFiles);
    dissimilarity += comparisonFunctionSlow(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, true, curCosts);

    //Extra checks which disqualify a case from beeing picked, not used for the algorithm but used to not choose a case as best.
    float dissExtra = comparisonFunctionQuick(curGamestate, caseGamestate, caseReplay, caseData, replayIndex, caseIndex, false, curCosts, -1, -1, -1, replayFiles);

    if (bestCandidate.similarity > dissimilarity + dissExtra) {
        bestCandidate.treePos.y = y;
        bestCandidate.treePos.x = x;
        bestCandidate.similarity = dissimilarity + dissExtra;
    }


    if (y > 0 && bestCandidate.similarity > (dissimilarity - cAdvance)) {

        for (int i = 0; i < tree[y][x].childNodes.size(); i++) {
            searchSimilarRemainingNode(comparisonNr, curGamestate, replayFiles, tree, tree[y][x].childNodes[i].y, tree[y][x].childNodes[i].x, bestCandidate, unexploredCandidates, treeRadius, costs, curCosts);
        }
    }
}

void CbrSearchTree::iterateRemainingNodes(int& comparisonNr, Metadata* curGamestate, std::vector<CbrReplayFile>& replayFiles, candidateNode& bestCandidate, std::vector<candidateNode>& unexploredCandidates, std::vector<float>& treeRadius, costWeights& costs, std::array<float, 200>& curCosts) {


    for (int i = 0; i < unexploredCandidates.size(); i++) {
        if (unexploredCandidates[i].similarity < bestCandidate.similarity) {
            for (int j = 0; j < tree[unexploredCandidates[i].treePos.y][unexploredCandidates[i].treePos.x].childNodes.size(); j++) {
                searchSimilarRemainingNode(comparisonNr, curGamestate, replayFiles, tree, tree[unexploredCandidates[i].treePos.y][unexploredCandidates[i].treePos.x].childNodes[j].y, tree[unexploredCandidates[i].treePos.y][unexploredCandidates[i].treePos.x].childNodes[j].x, bestCandidate, unexploredCandidates, treeRadius, costs, curCosts);
            }

        }
    }
}

void CbrSearchTree::insertTreeNode(int& cbrIndex, int& cbrReplayIndex, candidateNode& candidate) {
    treeNode newNode;
    newNode.cbrIndex = cbrIndex;
    newNode.cbrReplayIndex = cbrReplayIndex;
    tree[candidate.treePos.y - 1].push_back(newNode);
    int x = tree[candidate.treePos.y - 1].size() - 1;
    tree[candidate.treePos.y][candidate.treePos.x].childNodes.push_back({ candidate.treePos.y - 1 , x });
}





void CbrSearchTree::prepareSearchTree(costWeights& costs) {
    float lowestComp = costs.lowestCost;
    float levelValue = costs.combinedCost;
    while (levelValue > lowestComp) {
        tree.push_back(std::vector<treeNode>{});
        candidates.push_back(std::unordered_map<int, bool>{});
        treeCompValues.insert(treeCompValues.begin(), levelValue);
        parrentlessCandidates.push_back(std::unordered_map<int, bool>{});
        levelValue = (levelValue) / 2;
    }
}

void CbrSearchTree::insertFirstNode() {
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

void CbrSearchTree::insertNode(Metadata* curGamestate, int& cbrIndex, int& cbrReplayIndex, std::vector<CbrReplayFile> replayFiles, costWeights& costs, std::array<float, 200>& curCosts) {

    if (tree.size() == 0) {
        prepareSearchTree(costs);
        insertFirstNode();
        return;
    }

    auto height = treeCompValues.size() - 1;
    auto comparisonNr = 0;
    for (int i = 0; i <= height; i++) {
        candidates[i] = {};
        parrentlessCandidates[i] = {};
    }
    int lowestCandidateLevel = height;
    int lowestCreationLevel = height + 1;
    comparisonNr = 0;

    candidateNode candidate;
    candidate.treePos.y = 90000;
    searchInsertionSpot(curGamestate, comparisonNr, replayFiles, height, 0, candidate, treeCompValues, costs, curCosts);
    insertTreeNode(cbrIndex, cbrReplayIndex, candidate);
}


void CbrSearchTree::findCaseInTree(Metadata* curGamestate, int& cbrIndex, int& cbrReplayIndex, std::vector<CbrReplayFile> replayFiles, costWeights& costs, std::array<float, 200>& curCosts) {
    exploredCandidates.clear();
    caseList.clear();

    candidateNode bestCandidate;
    bestCandidate.similarity = 100;
    std::vector<candidateNode> unexploredCandidates{};
    float bestSim = 100;
    int bestCaseIndex = 0;
    int bestReplayIndex = 0;
    int y = 0;
    int x = 0;

    //Debug function to go through the entire tree and find the actually best candidate, allows for comparison with what the algorithm finds
    for (int i = 0; i < tree.size(); i++) {
        for (int j = 0; j < tree[i].size(); j++) {
            auto bufferSim = comparisonFunction(curGamestate, replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex)->getMetadata(), replayFiles[tree[i][j].cbrReplayIndex], replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex), tree[i][j].cbrReplayIndex, tree[i][j].cbrIndex, true, curCosts, -1, -1, -1);
            bufferSim += comparisonFunctionSlow(curGamestate, replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex)->getMetadata(), replayFiles[tree[i][j].cbrReplayIndex], replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex), tree[i][j].cbrReplayIndex, tree[i][j].cbrIndex, true, curCosts);
            bufferSim += comparisonFunctionQuick(curGamestate, replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex)->getMetadata(), replayFiles[tree[i][j].cbrReplayIndex], replayFiles[tree[i][j].cbrReplayIndex].getCase(tree[i][j].cbrIndex), tree[i][j].cbrReplayIndex, tree[i][j].cbrIndex, false, curCosts, -1, -1, -1);
            if (bestSim > bufferSim) {
                bestSim = bufferSim;
                bestCaseIndex = tree[i][j].cbrIndex;
                bestReplayIndex = tree[i][j].cbrReplayIndex;
                y = i;
                x = j;
            }
        }

    }
    caseList.push_back({ y,x, bestSim });

    //Still for debugging: creates the optimal search path that could have been used to find the best node
    while (y < tree.size() - 1) {
        for (int i = 0; i < tree.size(); i++) {
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


    int comparisonNr = 0;

    searchSimilarNode(comparisonNr, curGamestate, replayFiles, tree.size() - 1, 0, bestCandidate, unexploredCandidates, treeCompValues, costs, curCosts);
    iterateRemainingNodes(comparisonNr, curGamestate, replayFiles, bestCandidate, unexploredCandidates, treeCompValues, costs, curCosts);

    if (bestCandidate.similarity > bestSim) {
    std:printf("Asd");
    }

}
