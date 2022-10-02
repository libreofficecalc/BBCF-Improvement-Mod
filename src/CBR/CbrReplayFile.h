#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "Metadata.h"
#include "CbrCase.h"
#include "AnnotatedReplay.h"


struct CommandActions
{
    std::string moveName;
    std::vector<int> inputs;
    std::string priorMove;
    std::vector<int> altInputs;
};


class CbrReplayFile {
private:
    //friend class boost::serialization::access;
    std::vector<CbrCase> cbrCase;
    std::vector<int> input;
    std::array< std::string, 2> characterName;
    static std::array< std::string, 13> neutralActions;
    bool stateChangeTrigger = false;
    bool commandInputNeedsResolving = false;
 

public:
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& cbrCase& characterName & input;
    }
    CbrReplayFile();
    //p1Charname p2Charname
    CbrReplayFile(std::string, std::string);
    CbrReplayFile(std::array< std::string, 2>);
    void CopyInput(std::vector<int>);
    void AddCase(CbrCase);
    CbrCase getCase(int);
    std::vector<CbrCase> getCaseBase();
    int getCaseBaseLength();
    int getInput(int);

    void MakeCaseBase(AnnotatedReplay, std::string, int, int, int);
    bool CheckCaseEnd(int, Metadata, std::string, bool);
    bool CheckNeutralState(std::string);
    std::vector<int> DeconstructInput(int, bool);
    std::vector < std::vector<signed int>> CheckCommandExecution(int, std::vector<std::vector<int>>);
    std::vector<std::vector<int>> MakeInputArray(std::string, std::vector<CommandActions>, std::string);
    std::vector<CommandActions> FetchCommandActions(std::string);
    void instantLearning(AnnotatedReplay, std::string);
    void CbrReplayFile::makeFullCaseBase(AnnotatedReplay, std::string);
};


