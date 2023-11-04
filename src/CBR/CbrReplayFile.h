#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "Metadata.h"
#include "CbrCase.h"
#include "AnnotatedReplay.h"

struct CbrGenerationError
{
    std::string structure;
    std::string errorDetail;
    int errorCount;
};

struct CommandActions
{
    std::string moveName;
    std::vector<int> inputs;
    std::string priorMove;
    std::vector<int> altInputs;
};

struct inputMemory 
{
    std::vector<signed int> inputs;
    std::string name;
};

class CbrReplayFile {
private:
    //friend class boost::serialization::access;
    std::vector<CbrCase> cbrCase;
    std::vector<int> input;
    std::array<std::string, 2> characterName;
    std::array<int, 2> characterId;
    static std::array< std::string, 13> neutralActions;
    bool stateChangeTrigger = false;
    bool commandInputNeedsResolving = false;
 
    std::string inputResolveName = "";

public:
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& cbrCase& characterName & input &  stateChangeTrigger & commandInputNeedsResolving & characterId;
    }
    CbrReplayFile();
    //p1Charname p2Charname
    CbrReplayFile::CbrReplayFile(std::string p1, std::string p2, int p1ID, int p2ID);
    CbrReplayFile::CbrReplayFile(std::array< std::string, 2> arr, std::array< int, 2> arrId);
    void CopyInput(std::vector<int>);
    void AddCase(CbrCase);
    CbrCase* getCase(int);
    std::vector<CbrCase>* getCaseBase();
    int getCaseBaseLength();
    int getInput(int);

    CbrGenerationError MakeCaseBase(AnnotatedReplay*, std::string, int, int, int);
    bool CbrReplayFile::CheckCaseEnd(int framesIdle, Metadata ar, std::string prevState, bool neutral, std::vector<CommandActions>& commands);
    bool CheckNeutralState(std::string);
    std::vector<int> DeconstructInput(int, bool);
    std::vector < inputMemory> CbrReplayFile::CheckCommandExecution(int input, std::vector < inputMemory>& inputBuffer);
    std::vector < inputMemory> CbrReplayFile::MakeInputArray(std::string& move, std::vector<CommandActions>& commands, std::string prevState);
    std::vector<CommandActions> FetchCommandActions(std::string&);
    std::vector<CommandActions> CbrReplayFile::FetchNirvanaCommandActions();
    CbrGenerationError instantLearning(AnnotatedReplay*, std::string);
    CbrGenerationError CbrReplayFile::makeFullCaseBase(AnnotatedReplay*, std::string);
    bool checkDirectionInputs(int direction, int input);
    bool isDirectionInputs(int direction);
    std::array<int, 2>& CbrReplayFile::getCharIds();
    std::array<std::string, 2>& CbrReplayFile::getCharNames();
    std::vector < inputMemory> CbrReplayFile::MakeInputArraySuperJump(std::string move, std::vector<int>& inputs, int startingIndex, std::vector < inputMemory>& inVec);
    std::vector < inputMemory> CbrReplayFile::DeleteCompletedInputs(std::string name, std::vector < inputMemory>& inputBuffer);
    bool CbrReplayFile::ContainsCommandState(std::string& move, std::vector<CommandActions>& commands);
};


