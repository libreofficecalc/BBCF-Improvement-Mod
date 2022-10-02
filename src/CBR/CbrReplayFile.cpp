#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "CbrReplayFile.h"
#include "Metadata.h"
#include "AnnotatedReplay.h"



#define caseNeutralTime 12
#define caseMinTime 4


CbrReplayFile::CbrReplayFile()
{
}

CbrReplayFile::CbrReplayFile(std::string p1, std::string p2)
{
    characterName[0] = p1;
    characterName[1] = p2;
}
CbrReplayFile::CbrReplayFile(std::array< std::string, 2> arr): characterName(arr)
{
}
void CbrReplayFile::CopyInput(std::vector<int> in)
{
    input = in;
}
void CbrReplayFile::AddCase(CbrCase c)
{
    cbrCase.push_back(std::move(c));
}

CbrCase CbrReplayFile::getCase(int index) {
    if (index >= cbrCase.size()) {
        return cbrCase[0];
    }
    return cbrCase[index];
}

std::vector<CbrCase> CbrReplayFile::getCaseBase() {
    return cbrCase;
}

int CbrReplayFile::getCaseBaseLength() {
    return cbrCase.size();
}
int CbrReplayFile::getInput(int i) {
    return input[i];
}

void CbrReplayFile::MakeCaseBase(AnnotatedReplay ar, std::string charName, int startIndex, int endIndex, int insertPoint) {
    //auto i = inputs.size()-1;
    
    auto commands = FetchCommandActions(charName);
    input = ar.getInput();
    std::vector < std::vector<signed int>> inputResolve;

    std::string curState = ar.ViewMetadata(endIndex).getCurrentAction()[0];
    auto neutral = false;

    int iteratorIndex = endIndex;//last index of the replay
    int framesIdle = 0; //frames character can be in a neutral state 
    for (std::vector<int>::size_type i = endIndex;
        i != startIndex - 1; i--) {
        neutral = ar.ViewMetadata(i).getNeutral()[0];
        
        if (inputResolve.size() > 0) {
            auto deconInputs = DeconstructInput(input[i], ar.ViewMetadata(i).getFacing());
            for (std::vector<int>::size_type j = deconInputs.size() - 1;
                j != (std::vector<int>::size_type) - 1; j--) {
                inputResolve = CheckCommandExecution(deconInputs[j], inputResolve);//fix this inputResolve doesnt update from the function.
                if (inputResolve.size() == 0) {
                    curState = ar.ViewMetadata(i).getCurrentAction()[0];
                    cbrCase.insert(cbrCase.begin() + insertPoint, CbrCase(std::move(ar.CopyMetadataPtr(i)), (int)i, iteratorIndex));
                    iteratorIndex = i - 1;
                    framesIdle = -1;
                    break;
                }
            }
        }
        else {
            if (CheckCaseEnd(framesIdle, ar.ViewMetadata(i), curState, neutral)) {
                inputResolve = {};
                if (stateChangeTrigger == true) {
                    stateChangeTrigger = false;
                    inputResolve = MakeInputArray(curState, commands, ar.ViewMetadata(i).getCurrentAction()[0]);

                }
                curState = ar.ViewMetadata(i).getCurrentAction()[0];
                if (inputResolve.size() > 0) {
                    auto deconInputs = DeconstructInput(input[i], ar.ViewMetadata(i).getFacing());
                    for (std::vector<int>::size_type j = deconInputs.size() - 1;
                        j != (std::vector<int>::size_type) - 1; j--) {
                        inputResolve = CheckCommandExecution(deconInputs[j], inputResolve);
                        if (inputResolve.size() == 0) {
                            curState = ar.ViewMetadata(i).getCurrentAction()[0];
                            cbrCase.insert(cbrCase.begin()+ insertPoint, CbrCase(std::move(ar.CopyMetadataPtr(i)), (int)i, iteratorIndex));
                            iteratorIndex = i - 1;
                            framesIdle = -1;
                            break;
                        }
                    }
                }
                else {
                    //problem here inserts at beginning but cases already exist in front during midfightlearning
                    cbrCase.insert(cbrCase.begin()+ insertPoint, CbrCase(std::move(ar.CopyMetadataPtr(i)), (int)i, iteratorIndex));
                    iteratorIndex = i - 1;
                    framesIdle = -1;
                }
            }
        }
        framesIdle++;

        
        /* std::cout << someVector[i]; ... */
    }
    if (framesIdle > 0){
        if (framesIdle < caseMinTime && cbrCase.size() > 0) {
            auto end = cbrCase[insertPoint].getEndIndex();
            cbrCase.erase(cbrCase.begin() + insertPoint);
            cbrCase.insert(cbrCase.begin() + insertPoint, CbrCase(std::move(ar.CopyMetadataPtr(startIndex)), (int)startIndex, end));
        }
        else {
            cbrCase.insert(cbrCase.begin() + insertPoint, CbrCase(std::move(ar.CopyMetadataPtr(startIndex)), (int)startIndex, iteratorIndex));
        }
        
    }
    
}


bool CbrReplayFile::CheckCaseEnd(int framesIdle, Metadata ar, std::string prevState, bool neutral) {

    //if states changed and minimum case time is reached change state
    auto curState = ar.getCurrentAction()[0];
    if (curState != prevState && framesIdle > caseMinTime) {
        stateChangeTrigger = true;
        return true;
    }

    //if player is hit or hits the opponent also on guard.
    if ((ar.getHitThisFrame()[0] || ar.getBlockThisFrame()[0] || ar.getHitThisFrame()[1] || ar.getBlockThisFrame()[1]) &&(framesIdle >= caseMinTime)) {
        return true;
    }

    // if in neutral state for longer than caseNeutralTime end case
    if (neutral && (framesIdle >= caseNeutralTime)) {
        return true;
    }
    return false;
}

void CbrReplayFile::instantLearning(AnnotatedReplay ar, std::string charName) {
    int start = 0;
    int end = ar.getInput().size() - 1;
    if (end <= 1) { return; }
    if (cbrCase.size() > 0) {
        start = cbrCase[cbrCase.size() -1].getStartingIndex();
        cbrCase.pop_back();
        MakeCaseBase(ar, charName, start, end, cbrCase.size());
    }
    else {
        MakeCaseBase(ar, charName, start, end, 0);
    }
    
}

void CbrReplayFile::makeFullCaseBase(AnnotatedReplay ar, std::string charName) {
    int start = 0;
    int end = ar.getInput().size() - 1;
    MakeCaseBase(ar, charName, start, end, 0);
}



#define specialButton 512
#define tauntButton 256
#define DButton 128
#define CButton 64
#define BButton 32
#define AButton 16
#define D 128
#define C 64
#define B 32
#define A 16

std::vector<int> CbrReplayFile::DeconstructInput(int input, bool facingSwitch) {
    std::vector<int> inputs;

    auto buffer = input;
    auto test = buffer - specialButton;
    if (test > 0) { buffer = test; }
    test = buffer - tauntButton;
    if (test > 0) { buffer = test; }
    test = buffer - DButton;
    if (test > 0) {
        buffer = test;
        inputs.push_back(D);
    }
    test = buffer - CButton;
    if (test > 0) {
        buffer = test;
        inputs.push_back(C);
    }
    test = buffer - BButton;
    if (test > 0) {
        buffer = test;
        inputs.push_back(B);
    }
    test = buffer - AButton;
    if (test > 0) {
        buffer = test;
        inputs.push_back(A);
    }

    if (facingSwitch) {
        if (buffer == 6 || buffer == 3 || buffer == 9) {
            buffer = buffer - 2;
        }

        if (buffer == 4 || buffer == 7 || buffer == 1) {
            buffer = buffer + 2;
        }
    }
    inputs.push_back(buffer);
    return inputs;
}

std::vector<std::vector<int>> CbrReplayFile::CheckCommandExecution(int input, std::vector<std::vector<int>> inputBuffer)
{
    for (std::size_t i = 0; i < inputBuffer.size(); ++i) {
        for (std::size_t j = inputBuffer[i].size() - 1; j >= 0; --j) {
            if (inputBuffer[i][j] < 0) {
                if (input != inputBuffer[i][j] * -1) {
                    if (j == 0) {
                        return inputBuffer = {};
                    }
                    else {
                        inputBuffer[i][j] = 0;
                    }
                }
            }
            else {
                if (inputBuffer[i][j] != 0) {
                    if (input == inputBuffer[i][j]) {

                        if (j == 0) {
                            return inputBuffer = {};
                        }
                        else {
                            inputBuffer[i][j] = 0;
                        }
                    }
                    break;
                }
            }
        }
    }
    return inputBuffer;
}



std::vector<std::vector<int>> CbrReplayFile::MakeInputArray(std::string move, std::vector<CommandActions> commands, std::string prevState) {
    int lastInput = -1;
    bool threeSixty = false;

    std::vector<int> inputsSequence;
    std::vector<std::vector<int>> container = { };

    for (std::size_t i = 0; i < commands.size(); ++i) {
        if (commands[i].moveName == move) {
            std::vector<int> inputsSequence;
            if (commands[i].priorMove == prevState) {
                for (std::size_t j = 0; j < commands[i].altInputs.size(); ++j) {
                    if (lastInput == commands[i].altInputs[j]) {
                        inputsSequence.push_back(lastInput * -1);
                    }
                    inputsSequence.push_back(commands[i].altInputs[j]);

                    lastInput = commands[i].altInputs[j];
                }
            }
            else {
                for (std::size_t j = 0; j < commands[i].inputs.size(); ++j) {
                    if (lastInput == commands[i].inputs[j]) {
                        inputsSequence.push_back(lastInput * -1);
                    }
                    inputsSequence.push_back(commands[i].inputs[j]);

                    lastInput = commands[i].inputs[j];
                }
            }

            container.push_back(inputsSequence);
        }
    }
    return container;
}

std::vector<CommandActions> makotoCommands =
{
    { "CmnActFDash", {6,6}},
    { "CmnActBDash", {4,4}},
    { "CmnActAirFDash", {6,6}},
    { "CmnActAirBDash", {4,4}},
    { "AutoBlocking", {2,1,4,D}},
    { "BunshinStepA", {2,1,4,A}},
    { "BunshinStepB", {2,1,4,B}},
    { "BunshinStepC", {2,1,4,C}},
    { "PunchShot", {2,3,6,A}},
    { "Pile Bunker", {2,3,6,B}},
    { "Syouryu", {6,2,3,C}},
    { "Air_Syouryu", {6,2,3,C}},
    { "SiriusJolt", {4,1,2,3,6,C}},
    { "ShinSyouryu1", {2,3,6,2,3,6,D}},
    { "ShinSyouryu1_OD", {2,3,6,2,3,6,D}},
    { "BigPunch", {6,3,2,1,4,6,D}},
    { "BigPunch_OD", {6,3,2,1,4,6,D}},
    { "AstralHeat", {2,3,6,3,2,1,4,D}},
};


std::vector<CommandActions> lambdaCommands =
{
    { "CmnActFDash", {6,6}},
    { "CmnActBDash", {4,4}},
    { "CmnActAirFDash", {6,6}},
    { "CmnActAirBDash", {4,4}},
    { "AssaultA", {2,3,6,A}},
    { "AssaultB", {2,3,6,B}},
    { "AssaultC", {2,3,6,C}, "AssaultB", {6,C}},
    { "SlowFieldA", {2,1,4,A}},
    { "SlowFieldB", {2,1,4,B}},
    { "SlowFieldC", {2,1,4,C}},
    { "LargeShot", {2,1,4,D}},
    { "GedanShot", {2,3,6,D}},
    { "ChudanShot", {2,1,4,D}},
    { "BackShot", {2,2,D}},
    { "UltimateMultiSword", {2,3,6,2,3,6,D}},
    { "UltimateMultiSwordOD", {2,3,6,2,3,6,D}},
    { "UltimateLargeSwordLand", {6,3,2,1,4,6,D}},
    { "UltimateLargeSwordLandOD", {6,3,2,1,4,6,D}},
    { "UltimateLargeSwordAir", {6,3,2,1,4,6,D}},
    { "UltimateLargeSwordAirOD", {6,3,2,1,4,6,D}},
    { "AstralHeat", {2,1,4,2,1,4,D}},

};

std::vector<CommandActions> bulletCommands =
{
    { "CmnActFDash", {6,6}},
    { "CmnActBDash", {4,4}},
    { "CmnActAirFDash", {6,6}},
    { "CmnActAirBDash", {4,4}},
    { "InterrUpThrow", {6,2,3,B}},
    { "InterrUpAddAttack", {2,2,D}},
    { "DashThrow", {4,1,2,3,6,C}},
    { "DashAddAttack", {2,3,6,D}},
    { "AntiAirThrowLand", {6,2,3,C}},
    { "AntiAirThrowAir", {6,2,3,C}},
    { "AntiAirAddAttack", {6,2,3,D}},
    { "CommandThrow", {2,4,8,6,B}},
    { "CommandThrow", {2,6,8,4,B}},
    { "CommandThrow", {4,8,6,2,B}},
    { "CommandThrow", {4,2,6,8,B}},
    { "CommandThrow", {8,6,2,4,B}},
    { "CommandThrow", {8,4,2,6,B}},
    { "CommandThrow", {6,2,4,8,B}},
    { "CommandThrow", {6,8,4,2,B}},
    { "CommandThrowAddAttack", {6,3,2,1,4,D}},
    { "UltimateAssault", {2,3,6,3,2,1,4,C}},
    { "UltimateAssaultOD", {2,3,6,3,2,1,4,C}},
    { "UltimateThrow", {2,4,8,6,2,4,8,6,A}},
    { "UltimateThrow", {2,6,8,4,2,6,8,4,A}},
    { "UltimateThrow", {4,8,6,2,4,8,6,2,A}},
    { "UltimateThrow", {4,2,6,8,4,2,6,8,A}},
    { "UltimateThrow", {8,6,2,4,8,6,2,4,A}},
    { "UltimateThrow", {8,4,2,6,8,4,2,6,A}},
    { "UltimateThrow", {6,2,4,8,6,2,4,8,A}},
    { "UltimateThrow", {6,8,4,2,6,8,4,2,A}},
    { "UltimateThrowOD", {2,4,8,6,2,4,8,6,A}},
    { "UltimateThrowOD", {2,6,8,4,2,6,8,4,A}},
    { "UltimateThrowOD", {4,8,6,2,4,8,6,2,A}},
    { "UltimateThrowOD", {4,2,6,8,4,2,6,8,A}},
    { "UltimateThrowOD", {8,6,2,4,8,6,2,4,A}},
    { "UltimateThrowOD", {8,4,2,6,8,4,2,6,A}},
    { "UltimateThrowOD", {6,2,4,8,6,2,4,8,A}},
    { "UltimateThrowOD", {6,8,4,2,6,8,4,2,A}},
    { "AstralHeat", {6,3,2,1,4,6,D}},
};

std::vector<CommandActions> maiCommands =
{
    { "CmnActFDash", {6,6}},
    { "CmnActBDash", {4,4}},
    { "CmnActAirFDash", {6,6}},
    { "CmnActAirBDash", {4,4}},
    { "Backflip", {2,1,4}},
};

std::vector<CommandActions> CbrReplayFile::FetchCommandActions(std::string charName) {

    
    
    
    if (charName == "es") { return bulletCommands; };//es
    if (charName == "ny") { return bulletCommands; };//nu
    if (charName == "ma") { return maiCommands; };// mai
    if (charName == "tb") { return bulletCommands; };// tsubaki
    if (charName == "rg") { return bulletCommands; };//ragna
    if (charName == "hz") { return bulletCommands; };//hazama
    if (charName == "jn") { return bulletCommands; };//jin
    if (charName == "mu") { return bulletCommands; };//mu
    if (charName == "no") { return bulletCommands; };//noel
    if (charName == "mk") { return makotoCommands; };//makoto
    if (charName == "rc") { return bulletCommands; };//rachel
    if (charName == "vh") { return bulletCommands; };//valk
    if (charName == "tk") { return bulletCommands; };//taokaka
    if (charName == "pt") { return bulletCommands; };//platinum
    if (charName == "tg") { return bulletCommands; };//tager
    if (charName == "rl") { return bulletCommands; };//relius
    if (charName == "lc") { return bulletCommands; };//litchi
    if (charName == "iz") { return bulletCommands; };//izanami
    if (charName == "ar") { return bulletCommands; };//arakune
    if (charName == "am") { return bulletCommands; };//amane
    if (charName == "bn") { return bulletCommands; };//bang
    if (charName == "bl") { return bulletCommands; };//bullet
    if (charName == "ca") { return bulletCommands; };//carl
    if (charName == "az") { return bulletCommands; };//azrael
    if (charName == "ha") { return bulletCommands; };//hakumen
    if (charName == "kg") { return bulletCommands; };//kagura
    if (charName == "kk") { return bulletCommands; };//koko
    if (charName == "rm") { return lambdaCommands; };//lambda
    if (charName == "hb") { return bulletCommands; };//hibiki
    if (charName == "tm") { return bulletCommands; };//terumi
    if (charName == "ph") { return bulletCommands; };//nine
    if (charName == "ce") { return bulletCommands; };//Celica
    if (charName == "nt") { return bulletCommands; };//naoto
    if (charName == "mi") { return bulletCommands; };//izanami
    if (charName == "su") { return bulletCommands; };//susan
    if (charName == "??") { return bulletCommands; };//jubei
    return bulletCommands;
}