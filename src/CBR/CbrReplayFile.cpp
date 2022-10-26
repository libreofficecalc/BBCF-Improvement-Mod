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

CbrCase* CbrReplayFile::getCase(int index) {
    if (index >= cbrCase.size()) {
        return &cbrCase[0];
    }
    return &cbrCase[index];
}

std::vector<CbrCase>* CbrReplayFile::getCaseBase() {
    return &cbrCase;
}

int CbrReplayFile::getCaseBaseLength() {
    return cbrCase.size();
}
int CbrReplayFile::getInput(int i) {
    return input[i];
}

int CbrReplayFile::MakeCaseBase(AnnotatedReplay* ar, std::string charName, int startIndex, int endIndex, int insertPoint) {
    //auto i = inputs.size()-1;
    int errorCounter = 0;
    int debugInt = 0;

    bool facingCommandStart = 0;

    auto commands = FetchCommandActions(charName);
    input = ar->getInput();
    std::vector < std::vector<signed int>> inputResolve;
    std::vector < std::vector<signed int>> debugResolve;
    std::string curState = ar->ViewMetadata(endIndex).getCurrentAction()[0];
    auto neutral = false;

    int iteratorIndex = endIndex;//last index of the replay
    int framesIdle = 0; //frames character can be in a neutral state 

    int frameCount = ar->ViewMetadata(endIndex).getFrameCount();
    

    for (std::vector<int>::size_type i = endIndex;
        i != startIndex - 1; i--) {

        if (endIndex != i && frameCount <= ar->ViewMetadata(i).getFrameCount()) {
            continue;
        }
        frameCount = ar->ViewMetadata(i).getFrameCount();


        neutral = ar->ViewMetadata(i).getNeutral()[0];
        
        if (inputResolve.size() > 0) {
            debugInt++;
            if (debugInt == 100) {
                debugInt = 0;
                errorCounter++;
            }
            auto deconInputs = DeconstructInput(input[i], facingCommandStart);
            for (std::vector<int>::size_type j = 0; j < deconInputs.size(); ++j) {
                inputResolve = CheckCommandExecution(deconInputs[j], inputResolve);//fix this inputResolve doesnt update from the function.
                if (inputResolve.size() == 0) {
                    curState = ar->ViewMetadata(i).getCurrentAction()[0];
                    ar->CopyMetadataPtr(i)->setInputBufferActive(true);
                    cbrCase.insert(cbrCase.begin() + insertPoint, CbrCase(std::move(ar->CopyMetadataPtr(i)), (int)i, iteratorIndex));
                    if ((int)i > iteratorIndex) {
                        errorCounter++;
                    }
                    iteratorIndex = i - 1;
                    framesIdle = -1;
                    break;
                }
            }
        }
        else {
            if (CheckCaseEnd(framesIdle, ar->ViewMetadata(i), curState, neutral)) {
                inputResolve = {};
                if (stateChangeTrigger == true) {
                    debugInt = 0;
                    stateChangeTrigger = false;
                    facingCommandStart = ar->ViewMetadata(i + 1).getFacing();
                    inputResolve = MakeInputArray(curState, commands, ar->ViewMetadata(i).getCurrentAction()[0]);

                }
                curState = ar->ViewMetadata(i).getCurrentAction()[0];
                debugResolve = inputResolve;
                if (inputResolve.size() > 0) {
                    auto deconInputs = DeconstructInput(input[i], facingCommandStart);
                    for (std::vector<int>::size_type j = 0; j < deconInputs.size(); ++j) {
                        inputResolve = CheckCommandExecution(deconInputs[j], inputResolve);
                        if (inputResolve.size() == 0) {
                            curState = ar->ViewMetadata(i).getCurrentAction()[0];
                            ar->CopyMetadataPtr(i)->setInputBufferActive(true);
                            cbrCase.insert(cbrCase.begin()+ insertPoint, CbrCase(std::move(ar->CopyMetadataPtr(i)), (int)i, iteratorIndex));
                            if ((int)i > iteratorIndex) {
                                errorCounter++;
                            }
                            iteratorIndex = i - 1;
                            framesIdle = -1;
                            break;
                        }
                    }
                }
                else {
                    cbrCase.insert(cbrCase.begin()+ insertPoint, CbrCase(std::move(ar->CopyMetadataPtr(i)), (int)i, iteratorIndex));
                    if ((int)i > iteratorIndex) {
                        errorCounter++;
                    }
                    iteratorIndex = i - 1;
                    framesIdle = -1;
                }
            }
        }
        framesIdle++;

        
        /* std::cout << someVector[i]; ... */
    }

    if (framesIdle > 0){
        auto debugCase = cbrCase;
        if (framesIdle < caseMinTime && cbrCase.size() > 0 && insertPoint > 0) {
            int updatedInsertPoint = insertPoint - 1;
            cbrCase[updatedInsertPoint].SetEndIndex(cbrCase[updatedInsertPoint].getEndIndex() + framesIdle);
            //auto end = cbrCase[updatedInsertPoint].getEndIndex();
            //cbrCase.erase(cbrCase.begin() + updatedInsertPoint);
            //cbrCase.insert(cbrCase.begin() + updatedInsertPoint, CbrCase(std::move(ar->CopyMetadataPtr(startIndex)), (int)startIndex, end));
            if (cbrCase[updatedInsertPoint].getStartingIndex() > cbrCase[updatedInsertPoint].getEndIndex()) {
                errorCounter++;
            }
            auto caseBaseSize = cbrCase.size();
            auto testEnd = 0;
            for (std::size_t j = 0; j < caseBaseSize; ++j) {
                if (j == 0) {
                    testEnd = cbrCase[j].getEndIndex();
                }
                else {
                    if (testEnd + 1 != cbrCase[j].getStartingIndex()) {
                        errorCounter++;
                    }
                    testEnd = cbrCase[j].getEndIndex();
                }
            }
        }
        else {
            cbrCase.insert(cbrCase.begin() + insertPoint, CbrCase(std::move(ar->CopyMetadataPtr(startIndex)), (int)startIndex, iteratorIndex));
            if ((int)startIndex > iteratorIndex) {
                errorCounter++;
            }
            auto caseBaseSize = cbrCase.size();
            auto testEnd = 0;
            for (std::size_t j = 0; j < caseBaseSize; ++j) {
                if (j == 0) {
                    testEnd = cbrCase[j].getEndIndex();
                }
                else {
                    if (testEnd + 1 != cbrCase[j].getStartingIndex()) {
                        errorCounter++;
                    }
                    testEnd = cbrCase[j].getEndIndex();
                }
            }
            
        }
        
    }
    auto caseBaseSize = cbrCase.size();
    auto testEnd = 0;
    for (std::size_t j = 0; j < caseBaseSize; ++j) {
        if (j == 0) {
            testEnd = cbrCase[j].getEndIndex();
        }
        else {
            if (testEnd + 1 != cbrCase[j].getStartingIndex()) {
                errorCounter++;
            }
            testEnd = cbrCase[j].getEndIndex();
        }
    }
    return errorCounter;
}


bool CbrReplayFile::CheckCaseEnd(int framesIdle, Metadata ar, std::string prevState, bool neutral) {

    //if states changed and minimum case time is reached change state
    auto curState = ar.getCurrentAction()[0];
    if (curState != prevState ) { //&& framesIdle > caseMinTime
        stateChangeTrigger = true;
        return true;
    }

    //if player is hit or hits the opponent also on guard.
    if ((ar.getHitThisFrame()[0] || ar.getBlockThisFrame()[0] || ar.getHitThisFrame()[1] || ar.getBlockThisFrame()[1])) {
        return true;
    }

    // if in neutral state for longer than caseNeutralTime end case
    if (neutral && (framesIdle >= caseNeutralTime)) {
        return true;
    }
    return false;
}

int CbrReplayFile::instantLearning(AnnotatedReplay* ar, std::string charName) {
    int ret = 0;
    int start = 0;
    int end = ar->getInput().size() - 1;
    if (end <= 1) { return ret; }
    auto copy = cbrCase;
    if (cbrCase.size() > 0) {
        start = cbrCase[cbrCase.size() -1].getStartingIndex();
        cbrCase.pop_back();
        ret = MakeCaseBase(ar, charName, start, end, cbrCase.size());
    }
    else {
        ret = MakeCaseBase(ar, charName, start, end, 0);
    }
    return ret;
}

int CbrReplayFile::makeFullCaseBase(AnnotatedReplay* ar, std::string charName) {
    int ret = 0;
    int start = 0;
    int end = ar->getInput().size() - 1;
    ret += MakeCaseBase(ar, charName, start, end, 0);
    return ret;
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
#define forward 1100
#define back 1101
#define up 1102
#define down 1103
#define chargeBack back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back
#define chargeForward forward, forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward,forward
#define chargeUp up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up
#define chargeDown down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down
#define chargeDown45f down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down,down
#define chargeUp20F up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up,up
#define chargeBack45f back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back,back

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
        else {
            if (buffer == 4 || buffer == 7 || buffer == 1) {
                buffer = buffer + 2;
            }
        }


    }
    inputs.push_back(buffer);
    return inputs;
}

std::vector<std::vector<int>> CbrReplayFile::CheckCommandExecution(int input, std::vector<std::vector<int>> inputBuffer)
{
    for (int i = 0; i < inputBuffer.size(); ++i) {
        auto test = inputBuffer[i].size() - 1;
        for (int j = inputBuffer[i].size() - 1; j >= 0; --j) {
            if (inputBuffer[i][j] < 0) {
                if (isDirectionInputs(-1 * inputBuffer[i][j])) {
                    if (!checkDirectionInputs(inputBuffer[i][j] * -1, input)) {
                        if (j == 0) {
                            return inputBuffer = {};
                        }
                        else {
                            inputBuffer[i][j] = 0;
                        }
                    }
                }
                else {
                    if (input != inputBuffer[i][j] * -1) {
                        if (j == 0) {
                            return inputBuffer = {};
                        }
                        else {
                            inputBuffer[i][j] = 0;
                        }
                    }
                }
                break;
            }
            if (inputBuffer[i][j] > 0) {
                if (isDirectionInputs( inputBuffer[i][j])) {
                    if (checkDirectionInputs(inputBuffer[i][j], input)) {
                        if (j == 0) {
                            return inputBuffer = {};
                        }
                        else {
                            inputBuffer[i][j] = 0;
                        }
                    }
                }
                else {
                    if (input == inputBuffer[i][j] ) {
                        if (j == 0) {
                            return inputBuffer = {};
                        }
                        else {
                            inputBuffer[i][j] = 0;
                        }
                    }
                }
                break;
            }
        }
    }
    return inputBuffer;
}

bool CbrReplayFile::isDirectionInputs(int direction) {
    if (direction >= 1100)return true;
    return false;
}

bool CbrReplayFile::checkDirectionInputs(int direction, int input) {
    if (direction == up) {
        if (input == 7)return true;
        if (input == 8)return true;
        if (input == 9)return true;
    }
    if (direction == forward) {
        if (input == 9)return true;
        if (input == 6)return true;
        if (input == 3)return true;
    }
    if (direction == down) {
        if (input == 3)return true;
        if (input == 2)return true;
        if (input == 1)return true;
    }
    if (direction == back) {
        if (input == 1)return true;
        if (input == 4)return true;
        if (input == 7)return true;
    }
    return false;
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
                    inputsSequence.push_back(commands[i].altInputs[j]);
                }
            }
            else {
                for (std::size_t j = 0; j < commands[i].inputs.size(); ++j) {
                    inputsSequence.push_back(commands[i].inputs[j]);
                }
            }

            container.push_back(inputsSequence);
        }
    }
    return container;
}
std::vector<CommandActions> esCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "ShotA", {2,3,6,A}},
    { "ShotB", {2,3,6,B}},
    { "AirShotA", {2,3,6,A}},
    { "AirShotB", {2,3,6,B}},
    { "AntiAir", {6,2,3,C}},
    { "SpecialThrow", {2,1,4,A}},
    { "MidAssault", {2,1,4,B}},
    { "Assault", {2,1,4,C}},
    { "Chage", {2,1,4,D}},
    { "UltimateShot_LandA", {6,3,1,4,6,A}},
    { "UltimateShot_LandA", {6,2,4,6,A}},
    { "UltimateShot_LandB",  {6,3,1,4,6,B}},
    { "UltimateShot_LandB",  {6,2,4,6,B}},
    { "UltimateShot_AirA", {6,3,1,4,6,A}},
    { "UltimateShot_AirA", {6,2,4,6,A}},
    { "UltimateShot_AirB",  {6,3,1,4,6,B}},
    { "UltimateShot_AirB",  {6,2,4,6,B}},
    { "UltimateShot_LandAOD", {6,3,1,4,6,A}},
    { "UltimateShot_LandAOD", {6,2,4,6,A}},
    { "UltimateShot_LandBOD",  {6,3,1,4,6,B}},
    { "UltimateShot_LandBOD",  {6,2,4,6,B}},
    { "UltimateShot_AirAOD", {6,3,1,4,6,A}},
    { "UltimateShot_AirAOD", {6,2,4,6,A}},
    { "UltimateShot_AirBOD",  {6,3,1,4,6,B}},
    { "UltimateShot_AirBOD",  {6,2,4,6,B}},
    { "UltimateAssault",  {6,3,1,4,6,C}},
    { "UltimateAssaultOD",  {6,2,4,6,C}},
    { "UltimateAssault",  {6,3,1,4,6,C}},
    { "UltimateAssaultOD",  {6,2,4,6,C}},
    { "UltimateChage",  {6,3,1,4,6,D}},
    { "UltimateChage",  {6,2,4,6,D}},
    { "AstralHeat", {2,6,2,6,C}},
};
std::vector<CommandActions> susanCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Assault", {2,3,6,A}},
    { "AntiAir", {6,2,3,C}},
    { "Shot", {2,1,4,A}},
    { "MidAssault", {2,1,4,B}},
    { "AirMidAssault", {2,1,4,B}},
    { "LowAssault", {2,3,6,B}},
    { "RushAssault", {C,C,C,C}},
    { "AssaultThrow", {6,2,4,C}},
    { "AssaultThrow", {6,3,1,4,C}},
    { "LongAssault", {2,3,6,D}},
    { "UltimateLongAssault", {2,6,2,6,D}},
    { "UltimateLongAssaultOD", {2,6,2,6,D}},
    { "UltimateRushAssault", {6,2,4,6,D}},
    { "UltimateRushAssaultOD", {6,2,4,6,D}},
    { "UltimateRushAssault", {6,3,1,4,6,D}},
    { "UltimateRushAssaultOD", {6,3,1,4,6,D}},
    { "UltimateCommand", {chargeDown,8,D}},
    { "AstralHeat", {2,4,2,6,C}},
    { "AstralHeat", {2,1,4,1,3,6,C}},
};
std::vector<CommandActions> azraelCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Assault", {2,3,6,A}},
    { "AZcombo1", {2,3,6,C}},
    { "Dunk", {6,2,3,B}},
    { "ShotAtemi", {2,1,4,B}},
    { "Baigaeshi", {2,3,6,B}},
    { "HomingJump", {2,1,4,C}},
    { "HomingJump", {-down,2,-down, 2, C}},
    { "VanishingAttack", {2,3,6,D}},
    { "DustAttack", {2,1,4,D}},
    { "SuperPunch", {2,6,2,6,D}},
    { "Hikou", {2,4,2,4,D}},
    { "SuperPunch_OD", {2,6,2,6,D}},
    { "Hikou_OD", {2,4,2,4,D}},
    { "AstralHeat", {6,3,1,4,6,C}},
    { "AstralHeat", {6,2,4,6,C}},
};
std::vector<CommandActions> litchiCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Kantsu_A", {4,2,6,A}},
    { "Kantsu_B", {4,2,6,B}},
    { "Kantsu_C", {4,2,6,C}},
    { "Kantsu_A", {4,1,3,6,A}},
    { "Kantsu_B", {4,1,3,6,B}},
    { "Kantsu_C", {4,1,3,6,C}},
    { "RenchanB", {2,1,4,B}},
    { "RenchanC", {2,1,4,C}},
    { "AntiAir", {6,2,3,D}},
    { "Assault", {2,3,6,A}},
    { "Assault3", {2,3,6,B}},
    { "AirAssault3", {2,3,6,B}},
    { "Assault2", {2,3,6,C}},
    { "AirAssault2", {2,3,6,C}},
    { "DashAssault", {4,2,1,C}},
    { "JumpToRod_Ride", {6,2,4,A}},
    { "JumpToRod_Kick", {6,2,4,B}},
    { "JumpToRod_Jump", {6,2,4,C}},
    { "JumpToRod_Ride", {6,3,1,4,A}},
    { "JumpToRod_Kick", {6,3,1,4,B}},
    { "JumpToRod_Jump", {6,3,1,4,C}},
    { "TonNanShaPeiAntiAir", {4,2,6,D}},
    { "TonNanShaPeiAntiLand", {4,2,6,D}},
    { "TonNanShaPeiAntiAir", {4,1,3,6,D}},
    { "TonNanShaPeiAntiLand", {4,1,3,6,D}},
    { "RocketLandF", {6,2,3,D}},
    { "RocketLandB", {4,2,1,D}},
    { "UltimateBakuhatsu", {6,4,2,8,C}},
    { "UltimateBakuhatsuOD", {6,4,2,8,C}},
    { "UltimateRush", {6,2,4,6,C}},
    { "UltimateRushOD", {6,2,4,6,C}}, 
    { "UltimateNingyouU", {6,2,4,6,D}},
    { "UltimateNingyouD", {6,3,1,4,6,D}},
    { "UltimateRush", {6,3,1,4,6,C}},
    { "UltimateRushOD", {6,3,1,4,6,C}},
    { "UltimateNingyouU", {6,3,1,4,6,D}},
    { "UltimateNingyouD", {6,3,1,4,6,D}},
    { "AstralHeat", {4,6,D}},
    { "AstralHeat", {3,4,2,6,4,6,D}},
    { "AstralHeat", {3,4,1,3,6,4,6,D}},
};
std::vector<CommandActions> taoCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Rush_A", {2,3,6,A}},
    { "Rush_B", {2,3,6,B}},
    { "Rush_C", {2,3,6,C}},
    { "LandChainSaw", {-down,2,-down,2,C}},
    { "Shot_A", {6,3,1,4,A}},
    { "Shot_B", {6,3,1,4,B}},
    { "Shot_C", {6,3,1,4,C}},
    { "Shot_A", {6,2,4,A}},
    { "Shot_B", {6,2,4,B}},
    { "Shot_C", {6,2,4,C}},
    { "Mid_Jump", {2,1,4,D}},
    { "TriangleJumpRight", {2,1,4,D}},
    { "TriangleJumpLeft", {2,1,4,D}},
    { "WallAssault_DBack", {chargeDown,8,D}},//charge
    { "WallAssault_DBack", {chargeDown,7,D}},//charge
    { "WallAssault_DFront", {chargeDown,9,D}},//charge
    { "UltimateAssault", {2,6,2,6,D}},
    { "UltimateAssaultOD", {2,6,2,6,D}},
    { "UltimateCatch", {2,4,2,4,C}},
    { "UltimateCatchOD", {2,4,2,4,C}},
    { "UltimateAirRush", {2,6,2,6,D}},
    { "UltimateAirRushOD", {2,6,2,6,D}},
    { "AstralHeat", {chargeBack,1,2,8,D}},
};
std::vector<CommandActions> hakuCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Assault", {2,1,4,A}},
    { "Assault4", {6,2,3,A}},
    { "Assault2", {2,3,6,B}},
    { "Assault3", {4,2,6,C}},
    { "Assault3", {4,1,3,6,C}},
    { "Assault5", {2,1,4,D}},
    { "AirAssault3", {2,1,4,A}},
    { "AirAssault2", {2,1,4,B}},
    { "AirAssault", {2,1,4,C}},
    { "UltimateAssault", {6,3,2,1,4,6,C}},
    { "UltimateAssaultOD", {6,3,2,1,4,6,C}},
    { "UltimateAssault", {6,2,4,6,C}},
    { "UltimateAssaultOD", {6,3,1,4,6,C}},
    { "UltimateGrip", {2,6,2,6,D}},
    { "UltimateGripOD", {2,6,2,6,D}},
    { "UltimateMode", {2,4,2,4,B}},
    { "AstralHeat", {chargeDown,8,D}},//charge
};
std::vector<CommandActions> jinCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "ShortDash", {-6,6,-forward,6}},
    { "Shot", {2,3,6,A}},
    { "AirShot", {2,3,6,A}},
    { "AirShot_D", {2,3,6,D}},
    { "Shot_D", {2,3,6,D}},
    { "Assault", {2,1,4,B}},
    { "Assault_D", {2,1,4,D}},
    { "AntiAir_Fast", {6,2,3,B}},
    { "AntiAir_Slow", {6,2,3,C}},
    { "AntiAir_D", {6,2,3,D}},
    { "RushAssault", {-down,2,-down,2,C}},
    { "AirAssault", {2,1,4,C}},
    { "AirAssault_D", {2,1,4,D}},
    { "UltimateShot", {6,3,1,4,6,C}},
    { "UltimateShotOD", {6,3,1,4,6,C}},
    { "UltimateAntiAirShot", {6,3,1,4,6,D}},
    { "UltimateAntiAirShotOD", {6,3,1,4,6,D}},
    { "UltimateAirShot", {6,3,1,4,6,D}},
    { "UltimateAirShotOD", {6,3,1,4,6,D}},
    { "UltimateShot", {6,2,4,6,C}},
    { "UltimateShotOD", {6,2,4,6,C}},
    { "UltimateAntiAirShot", {6,2,4,6,D}},
    { "UltimateAntiAirShotOD", {6,2,4,6,D}},
    { "UltimateAirShot", {6,2,4,6,D}},
    { "UltimateAirShotOD", {6,2,4,6,D}},
    { "UltimateAtami", {2,6,2,6,D}},
    { "UltimateAtamiOD", {2,6,2,6,D}},
    { "AstralHeat", {chargeDown,8,D}},//charge
};
std::vector<CommandActions> ragnaCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "AntiAir_C", {6,2,3,C}},
    { "AntiAir_D", {6,2,3,D}},
    { "AntiAir2nd", {2,3,6,C}},
    { "AntiAir2nd", {2,3,6,C}},
    { "AntiAir3rdYoko", {2,3,6,C}},
    { "AntiAir3rdTate", {2,1,4,D}},
    { "ShortDash", {-6,6,-forward,6}},
    { "MidAssault", {2,1,4,B}},
    { "MidAssault2nd", {2,1,4,D}},
    { "JumpAssault", {2,1,4,D}},
    { "AirJumpAssault", {2,1,4,D}},
    { "Shot", {2,3,6,D}},
    { "Assault", {2,1,4,A}},
    { "Assault2nd", {2,1,4,D}},
    { "AirAssault", {2,1,4,C}},
    { "AirAssault2nd", {2,1,4,D}},
    { "Oiuchi", {-down,2,-down,2,C}},
    { "UltimateAssault", {6,3,1,4,6,D}},
    { "UltimateAssault_OD", {6,3,1,4,6,D}},
    { "UltimateAssault", {6,3,1,4,6,D}},
    { "UltimateAssault_OD", {6,2,4,6,D}},
    { "BloodWeaponFinish", {2,4,2,4,D}},
    { "BloodWeaponFinish_OD", {2,4,2,4,D}},
    { "AstralHeat", {2,4,2,6,C}},//charge
    { "AstralHeat", {2,1,4,1,3,6,C}},//charge
};
std::vector<CommandActions> maiCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Assault", {2,3,6}},
    { "Backflip", {2,1,4}},
    { "UltimateJump", {2,3,6,D}},
    { "UltimateJumpOD", {2,3,6,D}},
    { "UltimateAssault", {2,1,4,D}},
    { "UltimateAssaultOD", {2,1,4,D}},
    { "AstralHeat", {-down,2,-down,2,-down, 2,D}},
};
std::vector<CommandActions> noelCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "ShortDash", {-6,6,-forward,6}},
    { "con236D", {2,3,6,D}},
    { "con214D", {2,1,4,D}},
    { "con28D", {6,2,3,D}},
    { "Special236A", {2,3,6,A}},
    { "Special236B", {2,3,6,B}},
    { "ShortAssault", {2,3,6,C}},
    { "AirSpecial", {2,1,4,C}},
    { "SpecialThrow", {2,1,4,A}},
    { "OIUCHI_A1", {-down,2,-down,2,B}},
    { "OIUCHI_B1", {-down,2,-down,2,C}},
    { "UltimateShot", {6,3,1,4,6,D}},
    { "UltimateShotOD", {6,3,1,4,6,D}},
    { "UltimateShot", {6,2,4,6,D}},
    { "UltimateShotOD", {6,2,4,6,D}},
    { "UltimateAirShot", {2,6,2,6,D}},
    { "UltimateAirShotOD", {2,6,2,6,D}},
    { "AstralHeat", {2,6,2,6,C}},
    { "AstralHeatOld", {2,4,2,4,C}},
};
std::vector<CommandActions> makotoCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "AutoBlocking", {2,1,4,D}},
    { "BunshinStepA", {2,1,4,A}},
    { "BunshinStepB", {2,1,4,B}},
    { "BunshinStepC", {2,1,4,C}},
    { "CreateEnergyBall", {2,3,6,A}},
    { "Pile Bunker", {2,3,6,B}},
    { "Syouryu", {6,2,3,C}},
    { "Air_Syouryu", {6,2,3,C}},
    { "SiriusJolt", {4,1,3,6,C}},
    { "SiriusJolt", {4,2,6,C}},
    { "ShinSyouryu1", {2,6,2,6,D}},
    { "ShinSyouryu1_OD", {2,6,2,6,D}},
    { "BigPunch", {6,3,1,4,6,D}},
    { "BigPunch_OD", {6,3,1,4,6,D}},
    { "AstralHeat", {2,6,2,4,D}},
    { "BigPunch", {6,2,4,6,D}},
    { "BigPunch_OD", {6,2,4,6,D}},
    { "AstralHeat", {2,3,6,3,1,4,D}},
};
std::vector<CommandActions> lambdaCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "AssaultA", {2,3,6,A}},
    { "AssaultB", {2,3,6,B}},
    { "AssaultC", {2,3,6,C}, "AssaultB", {6,C}},
    { "SlowFieldA", {2,1,4,A}},
    { "SlowFieldB", {2,1,4,B}},
    { "SlowFieldC", {2,1,4,C}},
    { "LargeShot", {2,1,4,D}},
    { "GedanShot", {2,3,6,D}},
    { "ChudanShot", {2,1,4,D}},
    { "BackShot", {-down,2,-down,2,D}},
    { "UltimateMultiSword", {2,6,2,6,D}},
    { "UltimateMultiSwordOD", {2,6,2,6,D}},
    { "UltimateLargeSwordLand", {6,2,4,6,D}},
    { "UltimateLargeSwordLandOD", {6,2,4,6,D}},
    { "UltimateLargeSwordAir", {6,2,4,6,D}},
    { "UltimateLargeSwordAirOD", {6,2,4,6,D}},
    { "UltimateLargeSwordLand", {6,3,1,4,6,D}},
    { "UltimateLargeSwordLandOD", {6,3,1,4,6,D}},
    { "UltimateLargeSwordAir", {6,3,1,4,6,D}},
    { "UltimateLargeSwordAirOD", {6,3,1,4,6,D}},
    { "AstralHeat", {2,4,2,4,D}},
};
std::vector<CommandActions> nuCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "SpecialDashF", {-6,6,-forward,6}},
    { "SpecialDashB", {-4,4,-back,4}},
    { "SpecialDashAirF", {-6,6,-forward,6}},
    { "SpecialDashAirB", {-4,4,-back,4}},
    { "GedanShot", {2,3,6,D}},
    { "TimelagShot", {2,3,6,D}},
    { "ChudanShot", {2,1,4,D}},
    { "SlowFieldA", {2,1,4,A}},
    { "SlowFieldB", {2,1,4,B}},
    { "SlowFieldC", {2,1,4,C}},
    { "BackAssault", {6,2,3,A}},
    { "UltimateMultiSword", {2,6,2,6,D}},
    { "UltimateMultiSwordOD", {2,6,2,6,D}},
    { "UltimateLargeSwordLand", {6,2,4,6,D}},
    { "UltimateLargeSwordLandOD", {6,2,4,6,D}},
    { "UltimateLargeSwordAir", {6,2,4,6,D}},
    { "UltimateLargeSwordAirOD", {6,2,4,6,D}},
    { "UltimateLargeSwordLand", {6,3,1,4,6,D}},
    { "UltimateLargeSwordLandOD", {6,3,1,4,6,D}},
    { "UltimateLargeSwordAir", {6,3,1,4,6,D}},
    { "UltimateLargeSwordAirOD", {6,3,1,4,6,D}},
    { "AstralHeat", {2,4,2,4,D}},
};
std::vector<CommandActions> bulletCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "InterrUpThrow", {6,2,3,B}},
    { "InterrUpAddAttack", {2,2,D}},
    { "Shot", {2,3,6,A}},
    { "CompulsionHeatUp", {2,1,4,D}},
    { "DashThrow", {4,1,3,6,C}},
    { "DashThrow", {4,2,6,C}},
    { "DashAddAttack", {2,3,6,D}},
    { "AntiAirThrowLand", {6,2,3,C}},
    { "AntiAirThrowAir", {6,2,3,C}},
    { "AntiAirAddAttackExe", {6,2,3,D}},
    { "AntiAirAddAttackExe", {4,2,1,D}},
    { "AntiAirAddAttackExe", {6,2,1,D}},
    { "AntiAirAddAttackExe", {4,2,3,D}},
    { "CommandThrow", {2,4,8,6,B}},
    { "CommandThrow", {2,6,8,4,B}},
    { "CommandThrow", {4,8,6,2,B}},
    { "CommandThrow", {4,2,6,8,B}},
    { "CommandThrow", {8,6,2,4,B}},
    { "CommandThrow", {8,4,2,6,B}},
    { "CommandThrow", {6,2,4,8,B}},
    { "CommandThrow", {6,8,4,2,B}},
    { "CommandThrowAddAttack", {6,3,1,4,D}},
    { "CommandThrowAddAttack", {6,2,4,D}},
    { "UltimateAssault", {2,3,6,3,1,4,C}},
    { "UltimateAssaultOD", {2,3,6,3,1,4,C}},
    { "UltimateAssault", {2,6,2,4,C}},
    { "UltimateAssaultOD", {2,6,2,4,C}},
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
    { "AstralHeat", {6,2,4,6,D}},
    { "AstralHeat", {6,3,1,4,6,D}},
};
std::vector<CommandActions> tagerCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CommandThrow", {2,4,8,6,A}},
    { "CommandThrow", {2,6,8,4,A}},
    { "CommandThrow", {4,8,6,2,A}},
    { "CommandThrow", {4,2,6,8,A}},
    { "CommandThrow", {8,6,2,4,A}},
    { "CommandThrow", {8,4,2,6,A}},
    { "CommandThrow", {6,2,4,8,A}},
    { "CommandThrow", {6,8,4,2,A}},
    { "CommandThrowB2", {2,4,8,6,B}},
    { "CommandThrowB2", {2,6,8,4,B}},
    { "CommandThrowB2", {4,8,6,2,B}},
    { "CommandThrowB2", {4,2,6,8,B}},
    { "CommandThrowB2", {8,6,2,4,B}},
    { "CommandThrowB2", {8,4,2,6,B}},
    { "CommandThrowB2", {6,2,4,8,B}},
    { "CommandThrowB2", {6,8,4,2,B}},
    { "CommandThrowB2_Tuika", {2,4,8,6,B}},
    { "CommandThrowB2_Tuika", {2,6,8,4,B}},
    { "CommandThrowB2_Tuika", {4,8,6,2,B}},
    { "CommandThrowB2_Tuika", {4,2,6,8,B}},
    { "CommandThrowB2_Tuika", {8,6,2,4,B}},
    { "CommandThrowB2_Tuika", {8,4,2,6,B}},
    { "CommandThrowB2_Tuika", {6,2,4,8,B}},
    { "CommandThrowB2_Tuika", {6,8,4,2,B}},
    { "CommandThrowC", {2,4,8,6,C}},
    { "CommandThrowC", {2,6,8,4,C}},
    { "CommandThrowC", {4,8,6,2,C}},
    { "CommandThrowC", {4,2,6,8,C}},
    { "CommandThrowC", {8,6,2,4,C}},
    { "CommandThrowC", {8,4,2,6,C}},
    { "CommandThrowC", {6,2,4,8,C}},
    { "CommandThrowC", {6,8,4,2,C}},
    { "AntiAir", {6,2,3,C}},
    { "Stock", {2,1,4,D}},
    { "StockAttack", {2,3,6,A}},
    { "RollingAttack", {2,3,6,A}},
    { "RollingAttackB", {2,3,6,B}},
    { "RollingAttackA2nd", {2,3,6,A}},
    { "RollingAttackB2nd", {2,3,6,A}},
    { "Oiuchi", {-down,2,-down,2,D}},
    { "AntiAirShot", {6,2,3,D}},
    { "Shot", {4,1,3,6,D}},
    { "Shot", {4,2,6,D}},
    { "AirAssault", {6,3,1,4,A}},
    { "AirAssault", {6,2,4,A}},
    { "MTH", {2,6,2,6,B}},
    { "MTH_OD", {2,3,6,3,1,4,B}},
    { "MTH2nd", {2,6,2,6,B}},
    { "MTH2nd_OD", {2,3,6,3,1,4,B}},
    { "GETB", {2,4,8,6,2,4,8,6,A}},
    { "GETB", {2,6,8,4,2,6,8,4,A}},
    { "GETB", {4,8,6,2,4,8,6,2,A}},
    { "GETB", {4,2,6,8,4,2,6,8,A}},
    { "GETB", {8,6,2,4,8,6,2,4,A}},
    { "GETB", {8,4,2,6,8,4,2,6,A}},
    { "GETB", {6,2,4,8,6,2,4,8,A}},
    { "GETB", {6,8,4,2,6,8,4,2,A}},
    { "GETB_OD", {2,4,8,6,2,4,8,6,A}},
    { "GETB_OD", {2,6,8,4,2,6,8,4,A}},
    { "GETB_OD", {4,8,6,2,4,8,6,2,A}},
    { "GETB_OD", {4,2,6,8,4,2,6,8,A}},
    { "GETB_OD", {8,6,2,4,8,6,2,4,A}},
    { "GETB_OD", {8,4,2,6,8,4,2,6,A}},
    { "GETB_OD", {6,2,4,8,6,2,4,8,A}},
    { "GETB_OD", {6,8,4,2,6,8,4,2,A}},
    { "AstralHeat", {2,4,8,6,2,4,8,6,2,4,8,6,A}},
    { "AstralHeat", {2,6,8,4,2,6,8,4,2,6,8,4,A}},
    { "AstralHeat", {4,8,6,2,4,8,6,2,4,8,6,2,A}},
    { "AstralHeat", {4,2,6,8,4,2,6,8,4,2,6,8,A}},
    { "AstralHeat", {8,6,2,4,8,6,2,4,8,6,2,4,A}},
    { "AstralHeat", {8,4,2,6,8,4,2,6,8,4,2,6,A}},
    { "AstralHeat", {6,2,4,8,6,2,4,8,6,2,4,8,A}},
    { "AstralHeat", {6,8,4,2,6,8,4,2,6,8,4,2,A}},
};
std::vector<CommandActions> terumiCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Assault", {2,3,6,D}},
    { "CommandThrow", {2,1,4,D}},
    { "Oiuchi", {-down,2,-down,2,C}},
    { "MidAssault", {2,1,4,C}},
    { "UltimateAssault", {4,1,3,6,C}},
    { "UltimateAssault", {4,2,6,C}},
    { "UltimateAssault_OD", {4,1,3,6,C}},
    { "UltimateAssault_OD", {4,2,6,C}},
    { "UltimateAirAssault", {4,1,3,6,C}},
    { "UltimateAirAssault", {4,2,6,C}},
    { "UltimateAirAssault_OD", {4,1,3,6,C}},
    { "UltimateAirAssault_OD", {4,2,6,C}},
    { "UltimateAntiAir", {6,2,3,B}},
    { "UltimateAntiAir_OD", {6,2,3,B}},
    { "UltimateAntiAir_Air", {6,2,3,B}},
    { "UltimateAntiAir_Air_OD", {6,2,3,B}},
    { "UltimateRushMid", {6,2,4,B}},
    { "UltimateRushMid", {6,3,1,4,B}},
    { "UltimateRushMid_OD", {6,2,4,B}},
    { "UltimateRushMid_OD", {6,3,1,4,B}},
    { "UltimateRushLow", {6,2,4,A}},
    { "UltimateRushLow", {6,3,1,4,A}},
    { "UltimateRushLow_OD", {6,2,4,A}},
    { "UltimateRushLow_OD", {6,3,1,4,A}},
    { "UltimateAtemi", {2,6,2,6,A}},
    { "UltimateAtemi_OD", {2,6,2,6,A}},
    { "UltimateLock", {6,2,4,6,D}},
    { "UltimateLock", {6,3,1,4,6,D}},
    { "UltimateLock_OD", {6,2,4,6,D}},
    { "UltimateLock_OD", {6,3,1,4,6,D}},
    { "UltimateShot_AntiLand", {2,6,2,6,D}},
    { "UltimateShot_AntiLand_OD", {2,6,2,6,D}},
    { "UltimateShot_AntiAir", {2,4,2,4,D}},
    { "UltimateShot_AntiAir_OD", {2,4,2,4,D}},
    { "AstralHeat", {-down,2,-down,2,-down,2,D}},
};
std::vector<CommandActions> hazamaCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "SpecialShot", {2,3,6,A}},
    { "SpecialAssault", {2,1,4,B}},
    { "Kamae", {2,1,4,D}},
    { "KamaeDashF", {-6,6,-forward,6}},
    { "KamaeDashB", {-4,4,-back,4}},
    { "AntiAir", {6,2,3,D}},
    { "AirAssault2", {2,1,4,B}},
    { "SPThrow", {2,3,6,C}},
    { "UltimateAssault", {2,6,2,6,B}},
    { "UltimateAssault_OD", {2,6,2,6,B}},
    { "UltimateShot", {6,2,4,6,C}},
    { "UltimateShot", {6,3,1,4,6,C}},
    { "UltimateShot_OD", {6,2,4,6,C}},
    { "UltimateShot_OD", {6,3,1,4,6,C}},
    { "UltimateThrow", {6,2,4,6,D}},
    { "UltimateThrow", {6,3,1,4,6,D}},
    { "UltimateThrow_OD", {6,2,4,6,D}},
    { "UltimateThrow_OD", {6,3,1,4,6,D}},
    { "AstralHeat", {1,6,2,4,3,D}},
    { "AstralHeat", {1,6,3,1,4,3,D}},
};
std::vector<CommandActions> kaguraCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "StandByFDash", {-6,6,-forward,6}},
    { "StandByBDash", {-4,4,-back,4}},
    { "StandByBDash", {-4,4,-back,4}},
    { "ShotA", {chargeBack45f,6,A}},
    { "ShotB", {chargeBack45f,6,B}},
    { "AntiAirB", {chargeDown45f,up,B}},
    { "AntiAirC", {chargeDown45f,up,C}},
    { "AirAssault", {chargeUp20F,down,C}},
    { "UltimateShot", {chargeBack,1,3,6,C}},
    { "UltimateShot", {chargeBack,2,6,C}},
    { "UltimateShotOD", {chargeBack,1,3,6,C}},
    { "UltimateShotOD", {chargeBack,2,6,C}},
    { "UltimateStandByLand", {up,down,D}},
    { "UltimateStandByAir", {up,down,D}},
    { "UltimateStandByLandOD", {up,down,D}},
    { "UltimateStandByAirOD", {up,down,D}},
    { "AstralHeat", {2,6,2,6,C}},
};
std::vector<CommandActions> celicaCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "AirAssault", {2,1,4,A}},
    { "Shot", {2,3,6,B}},
    { "MidAssault", {2,1,4,B}},
    { "AntiAir", {2,3,6,C}},
    { "Assault", {2,1,4,C}},
    { "UltimateHeal", {6,2,4,6,A}},
    { "UltimateHeal", {6,3,1,4,6,A}},
    { "UltimateHealOD", {6,2,4,6,A}},
    { "UltimateHealOD", {6,3,1,4,6,A}},
    { "UltimateShot", {6,3,1,4,6,B}},
    { "UltimateShot", {6,2,4,6,B}},
    { "UltimateShotOD", {6,3,1,4,6,B}},
    { "UltimateShotOD", {6,2,4,6,B}},
    { "UltimateAssault", {6,3,1,4,6,C}},
    { "UltimateAssault", {6,2,4,6,C}},
    { "UltimateAssaultOD", {6,3,1,4,6,C}},
    { "UltimateAssaultOD", {6,2,4,6,C}},
    { "AstralHeat", {-down, 2,-down,2,-down,2,A}},
};
std::vector<CommandActions> platinumCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Hopping", {2,3,6,A}},
    { "SurfingU", {2,3,6,B}},
    { "SurfingV", {2,3,6,C}},
    { "Shabon_A", {2,1,4,A}},
    { "Shabon_B", {2,1,4,B}},
    { "Shabon_C", {2,1,4,C}},
    { "AirSlide", {2,3,6,C}},
    { "CommandThrow", {4,2,6,D}},
    { "CommandThrow", {4,1,3,6,D}},
    { "WeaponThrow", {2,1,4,D}},
    { "Atemi", {chargeDown,2,D}},
    { "Oiuchi", {-down,2,-down,2,C}},
    { "UltimateAssault", {6,3,1,4,6,C}},
    { "UltimateAssault", {6,2,4,6,C}},
    { "UltimateAssault_OD", {6,3,1,4,6,C}},
    { "UltimateAssault_OD", {6,2,4,6,C}},
    { "HiWeapon", {2,6,2,6,D}},
    { "HiWeapon_OD", {2,6,2,6,D}},
    { "AstralHeat", {2,6,2,6,C}},
};
std::vector<CommandActions> naotoCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "ShortDash", {-6,6,-forward,6}},
    { "Assault", {2,3,6,B}},
    { "Assault_2nd", {2,3,6,B}},
    { "Assault_3rdYoko", {2,3,6,B}},
    { "Assault_3rdTate", {2,3,6,C}},
    { "AntiAirC", {6,2,3,C}},
    { "AntiAirD", {6,2,3,D}},
    { "AntiAirC_Air", {6,2,3,C}},
    { "AntiAirD_Air", {6,2,3,D}},
    { "EdgeAssault", {2,1,4,D}},
    { "Dodge", {2,1,4,A}},
    { "UltimateAssault", {6,2,4,6,B}},
    { "UltimateAssault", {6,3,1,4,6,B}},
    { "UltimateAirAssault", {6,2,4,6,B}},
    { "UltimateAirAssault", {6,3,1,4,6,B}},
    { "UltimateAssaultOD", {6,2,4,6,B}},
    { "UltimateAssaultOD", {6,3,1,4,6,B}},
    { "UltimateAirAssaultOD", {6,2,4,6,B}},
    { "UltimateAirAssaultOD", {6,3,1,4,6,B}},
    { "UltimateEdgeAssault", {6,3,1,4,6,D}},
    { "UltimateEdgeAssault", {6,2,4,6,D}},
    { "UltimateEdgeAssaultOD", {6,3,1,4,6,D}},
    { "UltimateEdgeAssaultOD", {6,2,4,6,D}},
    { "AstralHeat", {2,1,4,1,3,6,C}},
    { "AstralHeat", {2,4,2,6,C}},
};
std::vector<CommandActions> reliusCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Kaihi", {2,3,6,A}},
    { "Assault", {4,2,6,B}},
    { "Assault", {4,1,3,6,B}},
    { "Strike", {2,3,6,C}},
    { "Strike_Air", {2,3,6,C}},
    { "Act_IG_SpAttack01", {2,1,4,A}},
    { "Act_IG_AddAttack01", {2,1,4,A}},
    { "Act_IG_SpAirAttack01", {2,1,4,B}},
    { "Act_IG_SpAttack02", {2,1,4,B}},
    { "Act_IG_AddAttack02", {2,1,4,B}},
    { "Act_IG_SpAttack03", {2,1,4,C}},
    { "Act_IG_AddAttack03", {2,1,4,C}},
    { "Shot_A", {-down, 2,-down,2,A}},
    { "Shot_B", {-down, 2,-down,2,B}},
    { "Shot_C", {-down, 2,-down,2,C}},
    { "Act_IG_SpThrow", {2,3,6,D}},
    { "Act_IG_SpAttack04", {2,1,4,D}},
    { "UltimateField", {2,6,2,4,C}},
    { "UltimateField", {2,3,6,3,1,4,C}},
    { "UltimateFieldOD", {2,6,2,4,C}},
    { "UltimateFieldOD", {2,3,6,3,1,4,C}},
    { "Act_IG_UltimateAtk", {6,3,1,4,6,D}},
    { "Act_IG_UltimateAtk", {6,2,4,6,D}},
    { "Act_IG_UltimateAtkOD", {6,3,1,4,6,D}},
    { "Act_IG_UltimateAtkOD", {6,2,4,6,D}},
    { "Act_IG_UltimateRush", {2,6,2,6,D}},
    { "Act_IG_UltimateRushOD", {2,6,2,6,D}},
    { "AstralHeat", {2,4,2,4,D}},
};
std::vector<CommandActions> valkCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Assault", {2,3,6,A}},
    { "LowAssault", {2,3,6,B}},
    { "LowAssault2nd", {2,3,6,B}},
    { "MidAssault", {2,3,6,C}},
    { "AirAssault", {2,1,4,B}},
    { "BeastCannon_H", {2,3,6,A}},
    { "BeastCannon_V", {2,3,6,B}},
    { "BeastCannon_U", {2,3,6,C}},
    { "BeastCannonAIR_H", {2,3,6,A}},
    { "BeastCannonAIR_V", {2,3,6,B}},
    { "BeastCannonAIR_U", {2,3,6,C}},
    { "BeastCannonAIR_V2", {2,1,4,A}},
    { "BeastCannonAIR_V3", {2,1,4,B}},
    { "BeastCannonAIR_V4", {2,1,4,C}},
    { "UltimateAssault", {6,3,1,4,6,D}},
    { "UltimateAssault", {6,2,4,6,D}},
    { "UltimateAssault_OD", {6,3,1,4,6,D}},
    { "UltimateAssault_OD", {6,2,4,6,D}},
    { "UltimateAirAssault", {2,6,2,6,C}},
    { "UltimateAirAssault_OD", {2,6,2,6,C}},
    { "AstralHeat", {2,4,2,4,C}},
};
std::vector<CommandActions> izanamiCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "SpecialShot", {4,1,3,6,C}},
    { "SpecialShot", {4,2,6,C}},
    { "Assault", {2,1,4,A}},
    { "AssaultLow", {2,1,4,B}},
    { "ShockWave", {2,3,6,A}},
    { "AirAssault", {6,3,1,4,B}},
    { "DrainThrow", {6,3,1,4,C}},
    { "DrainThrow", {6,2,4,C}},
    { "Barrier_On", {6,2,3,B}},
    { "Barrier_Off", {6,2,3,B}},
    { "ShotLaser", {4,2,6,D}},
    { "ShotLaser", {4,1,3,6,D}},
    { "ShotSaws", {6,2,4,D}},
    { "ShotSaws", {6,3,1,4,D}},
    { "UltimateAssault", {2,6,2,6,B}},
    { "UltimateAssault_OD", {2,6,2,6,B}},
    { "UltimateTimeStopThrow", {2,4,2,4,C}},
    { "UltimateTimeStopThrow_OD", {2,4,2,4,C}},
    { "UltimateTimeStop", {2,4,8,6,2,4,8,6,A}},
    { "UltimateTimeStop", {2,6,8,4,2,6,8,4,A}},
    { "UltimateTimeStop", {4,8,6,2,4,8,6,2,A}},
    { "UltimateTimeStop", {4,2,6,8,4,2,6,8,A}},
    { "UltimateTimeStop", {8,6,2,4,8,6,2,4,A}},
    { "UltimateTimeStop", {8,4,2,6,8,4,2,6,A}},
    { "UltimateTimeStop", {6,2,4,8,6,2,4,8,A}},
    { "UltimateTimeStop", {6,8,4,2,6,8,4,2,A}},
    { "AstralHeat", {1,6,3,1,4,3,D}},
    { "AstralHeat", {1,6,2,4,3,D}},
};
std::vector<CommandActions> nineCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "AssaultLand", {2,3,6,A}},
    { "AssaultAir", {2,3,6,A}},
    { "AntiAir", {2,1,4,B}},
    { "MidAssault", {2,1,4,C}},
    { "MagicConversion", {2,3,6,D}},
    { "MagicReduction", {2,1,4,D}},
    { "UltimateShot", {2,6,2,6,A}},
    { "UltimateShotOD", {2,6,2,6,A}},
    { "UltimateCharge", {2,6,2,6,B}},
    { "UltimateChargeOD", {2,6,2,6,B}},
    { "UltimateLock", {2,6,2,6,C}},
    { "UltimateLockOD", {2,6,2,6,C}},
    { "AstralHeat", {2,6,2,6,D}},
};
std::vector<CommandActions> muCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "SGShot", {2,3,6,D}},
    { "SGShotAir", {2,3,6,D}},
    { "SGAssault", {2,1,4,D}},
    { "SGAssaultAir", {2,1,4,D}},
    { "DirectShot", {2,3,6,A}},
    { "DirectShotAir", {2,3,6,A}},
    { "FunnelBarrier", {6,2,3,C}},
    { "FunnelBarrier_Air", {6,2,3,C}},
    { "FlyingAssault", {6,3,1,4,B}},
    { "FlyingAssault", {6,2,4,B}},
    { "SwrodSwing2", {6,3,1,4,C}},
    { "SwrodSwing2", {6,2,4,C}},
    { "UltimateAssault", {6,2,4,6,C}},
    { "UltimateAssault", {6,3,1,4,6,C}},
    { "UltimateAssaultOD", {6,2,4,6,C}},
    { "UltimateAssaultOD", {6,3,1,4,6,C}},
    { "UltimateSGShot", {6,3,1,4,6,D}},
    { "UltimateSGShot", {6,2,4,6,D}},
    { "UltimateSGShotOD", {6,3,1,4,6,D}},
    { "UltimateSGShotOD", {6,2,4,6,D}},
    { "UltimateSGShotAir", {6,3,1,4,6,D}},
    { "UltimateSGShotAir", {6,2,4,6,D}},
    { "UltimateSGShotAirOD", {6,3,1,4,6,D}},
    { "UltimateSGShotAirOD", {6,2,4,6,D}},
    { "AstralHeat", {-down, 2,-down,2,-down,2,D}},
};
std::vector<CommandActions> hibikiCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "FrontJump", {2,3,6,A}},
    { "FrontJump_Air", {2,3,6,A}},
    { "BunshinAssault_A", {2,1,4,A}},
    { "BunshinAssault_B", {2,1,4,B}},
    { "Assault_C", {2,1,4,C}},
    { "Assault_D", {2,1,4,D}},
    { "AntiAir", {6,2,3,C}},
    { "AntiAir_Air", {6,2,3,C}},
    { "AirAssault", {2,1,4,A},"FrontJump_Air", {A}},
    { "UltimateAssault", {6,3,1,4,6,D}},
    { "UltimateAssault", {6,2,4,6,D}},
    { "UltimateAssault_OD", {6,3,1,4,6,D}},
    { "UltimateAssault_OD", {6,2,4,6,D}},
    { "UltimateAssault2", {6,3,1,4,6,C}},
    { "UltimateAssault2", {6,2,4,6,C}},
    { "UltimateAssault2_OD", {6,3,1,4,6,C}},
    { "UltimateAssault2_OD", {6,2,4,6,C}},
    { "AstralHeat", {-down, 2,-down,2,-down,2,D}},
};
std::vector<CommandActions> rachelCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "ShotLandA", {2,3,6,A}},
    { "ShotLandB", {2,3,6,B}},
    { "ShotLandC", {2,3,6,C}},
    { "ShotAirA", {2,3,6,A}},
    { "ShotAirB", {2,3,6,B}},
    { "ShotAirC", {2,3,6,C}},
    { "ShotFlogLand", {2,1,4,A}},
    { "ShotFlogAir", {2,1,4,A}},
    { "IvyBlossom", {2,1,4,B}},
    { "IvyBlossomAir", {2,1,4,B}},
    { "LightningMini", {2,1,4,C}},
    { "LightningMiniAir", {2,1,4,C}},
    { "CallWindSuctionGhostLand", {-down,2,-down,2,A}},
    { "CallWindSuctionGhostAir", {-down,2,-down,2,A}},
    { "LightningLandB", {6,3,1,4,6,C}},
    { "LightningLandB", {6,2,4,6,C}},
    { "LightningLandB_OD", {6,3,1,4,6,C}},
    { "LightningLandB_OD", {6,2,4,6,C}},
    { "LightningAirB", {6,3,1,4,6,C}},
    { "LightningAirB", {6,2,4,6,C}},
    { "LightningAirB_OD", {6,3,1,4,6,C}},
    { "LightningAirB_OD", {6,2,4,6,C}},
    { "UltimateStorm", {6,3,1,4,6,B}},
    { "UltimateStorm", {6,2,4,6,B}},
    { "UltimateStorm_OD", {6,3,1,4,6,B}},
    { "UltimateStorm_OD", {6,2,4,6,B}},
    { "AstralHeat", {2,6,2,4,C}},
    { "AstralHeat", {2,3,6,3,1,4,C}},
};
std::vector<CommandActions> carlCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Rolling_A", {2,3,6,A}},
    { "Rolling_B", {2,3,6,B}},
    { "RemoteThrow", {6,2,3,C}},
    { "Air_MultiHit", {2,1,4,C}},
    { "UltimateHaguruma", {6,2,4,6,C}},
    { "UltimateHaguruma", {6,3,1,4,6,C}},
    { "UltimateHagurumaOD", {6,2,4,6,C}},
    { "UltimateHagurumaOD", {6,3,1,4,6,C}},
    { "OrderNirvanaRush", {2,6,2,6,D,-D}},
    { "OrderNirvanaRushOD", {2,6,2,6,D,-D}},
    { "OrderNirvanaRushAir", {2,6,2,6,D,-D}},
    { "OrderNirvanaRushAirOD", {2,6,2,6,D,-D}},
    { "OrderNirvanaStrike", {2,4,2,4,D,-D}},
    { "OrderNirvanaStrikeOD", {2,4,2,4,D,-D}},
    { "OrderNirvanaStrikeAir", {2,4,2,4,D,-D}},
    { "OrderNirvanaStrikeODAir", {2,4,2,4,D,-D}},
    { "UltimateLock", {6,2,4,6,D,-D}},
    { "UltimateLock", {6,3,1,4,6,D,-D}},
    { "AstralHeat", {6,4,6,4,1,3,6,D}},
    { "AstralHeat", {6,4,6,4,2,6,D}},
};
std::vector<CommandActions> tsubakiCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "LandTackleA", {2,3,6,A}},
    { "LandTackleB", {2,3,6,B}},
    { "LandTackleC", {2,3,6,C}},
    { "LandTackleEX", {2,3,6,D}},
    { "LandTackleEX", {2,3,6,D}},
    { "SwordFuriage", {2,1,4,B}},
    { "SwordFuriageEX", {2,1,4,D}},
    { "MidAssault", {-down,2,-down,2,B}},
    { "MidAssaultEX", {-down,2,-down,2,D}},
    { "MidAssault_Hasei", {-down,2,-down,2,B}},
    { "MidAssaultEX_Hasei", {-down,2,-down,2,D}},
    { "AntiAir", {6,2,3,C}},
    { "AntiAirEX", {6,2,3,D}},
    { "WingAttack", {2,3,6,A}},
    { "WingAttackEx", {2,3,6,A}},
    { "WingAttack_Hasei", {2,3,6,A}},
    { "WingAttackEx_Hasei", {2,3,6,A}},
    { "AirTackleA", {2,1,4,A}},
    { "AirTackleB", {2,1,4,B}},
    { "AirTackleC", {2,1,4,C}},
    { "AirTackleEX", {2,1,4,D}},
    { "AirTackle_Hasei", {2,1,4,C}},
    { "AirTackleEX_Hasei", {2,1,4,D}},
    { "Shot", {4,2,1,A}},
    { "ShotEX", {4,2,1,D}},
    { "IGUpThrow", {6,2,4,C}},
    { "IGUpThrow", {6,3,1,4,C}},
    { "UltimateLock_C", {2,6,2,6,C}},
    { "UltimateLock_C_OD", {2,6,2,6,C}},
    { "UltimateLock_D", {2,6,2,6,D}},
    { "UltimateLock_D_OD", {2,6,2,6,D}},
    { "ExModeBegin", {2,4,2,4,D}},
    { "UltimateShot", {6,2,4,6,B}},
    { "UltimateShot", {6,3,1,4,6,B}},
    { "UltimateShot_OD", {6,2,4,6,B}},
    { "UltimateShot_OD", {6,3,1,4,6,B}},
    { "AstralHeat", {6,2,4,6,C}},
    { "AstralHeat", {6,3,1,4,6,C}},
};
std::vector<CommandActions> kokonoeCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Assault_A", {2,3,6,A}},
    { "SpinAssault", {2,3,6,B}},
    { "Freeze_Shot", {2,3,6,C}},
    { "Shot_A", {2,1,4,A}},
    { "Shot_B", {2,1,4,B}},
    { "Shot_C", {2,1,4,C}},
    { "AirShot_A", {2,1,4,A}},
    { "AirShot_B", {2,1,4,B}},
    { "AirShot_C", {2,1,4,C}},
    { "Lightning_Shot_A", {-down,2,-down,2,A}},
    { "Lightning_Shot_B", {-down,2,-down,2,B}},
    { "Warp", {-down,2,-down,2,C}},
    { "UltimateShot_A", {2,4,2,4,A}},
    { "UltimateShot_B", {2,4,2,4,B}},
    { "UltimateShot_C", {2,4,2,4,C}},
    { "UltimateAirShot_A", {2,4,2,4,A}},
    { "UltimateAirShot_B", {2,4,2,4,B}},
    { "UltimateAirShot_C", {2,4,2,4,C}},
    { "UltimateShotOD_A", {2,4,2,4,A}},
    { "UltimateShotOD_B", {2,4,2,4,B}},
    { "UltimateShotOD_C", {2,4,2,4,C}},
    { "UltimateAirShotOD_A", {2,4,2,4,A}},
    { "UltimateAirShotOD_B", {2,4,2,4,B}},
    { "UltimateAirShotOD_C", {2,4,2,4,C}},
    { "UltimateBlackhole", {6,3,1,4,6,D}},
    { "UltimateBlackhole", {6,2,4,6,D}},
    { "UltimateBlackhole_OD", {6,3,1,4,6,D}},
    { "UltimateBlackhole_OD", {6,2,4,6,D}},
    { "UltimateLaser", {6,4,6,4,1,3,6,C}},
    { "UltimateLaser", {6,4,6,4,2,6,C}},
    { "UltimateAirLaser", {6,4,6,4,1,3,6,C}},
    { "UltimateAirLaser", {6,4,6,4,2,6,C}},
    { "UltimateLaser_OD", {6,4,6,4,1,3,6,C}},
    { "UltimateLaser_OD", {6,4,6,4,2,6,C}},
    { "UltimateAirLaser_OD", {6,4,6,4,1,3,6,C}},
    { "UltimateAirLaser_OD", {6,4,6,4,2,6,C}},
    { "AstralHeat", {2,4,8,6,2,4,8,6,D}},
    { "AstralHeat", {2,6,8,4,2,6,8,4,D}},
    { "AstralHeat", {4,8,6,2,4,8,6,2,D}},
    { "AstralHeat", {4,2,6,8,4,2,6,8,D}},
    { "AstralHeat", {8,6,2,4,8,6,2,4,D}},
    { "AstralHeat", {8,4,2,6,8,4,2,6,D}},
    { "AstralHeat", {6,2,4,8,6,2,4,8,D}},
    { "AstralHeat", {6,8,4,2,6,8,4,2,D}},
};
std::vector<CommandActions> izayoiCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Shot_A", {2,3,6,A}},
    { "Shot_D", {2,3,6,D}},
    { "AirShot_A", {2,3,6,A}},
    { "AirShot_D", {2,3,6,D}},
    { "Iai_Slash_Gedan", {2,3,6,B}},
    { "Iai_AntiAirA", {6,2,3,B}},
    { "Iai_AntiAirB", {6,2,3,C}},
    { "Iai_Slash", {2,3,6,C}},
    { "Iai_SlashAirA", {6,2,3,B}},
    { "Iai_SlashAirB", {2,3,6,C}},
    { "Iai_Slash_Gedan", {2,3,6,B}},
    { "Warp_A", {2,1,4,A}},
    { "Warp_B", {2,1,4,B}},
    { "Warp_C", {2,1,4,C}},
    { "Warp_D_Land", {2,1,4,D}},
    { "Warp_D_Air", {2,1,4,D}},
    { "UltimateAssault", {2,6,2,6,C}},
    { "UltimateAssault_OD", {2,6,2,6,C}},
    { "SummonFunnel", {6,3,1,4,6,D}},
    { "SummonFunnel", {6,2,4,6,D}},
    { "AstralHeat", {6,3,1,4,6,C}},
    { "AstralHeat", {6,2,4,6,C}},
};
std::vector<CommandActions> arakuneCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Meisai", {2,3,6,B}},
    { "SpecialShotLandA", {-down,2,-down,2,A}},
    { "SpecialShotLandB", {-down,2,-down,2,B}},
    { "SpecialShotLandC", {-down,2,-down,2,C}},
    { "SpecialShotAirA", {-down,2,-down,2,A}},
    { "SpecialShotAirB", {-down,2,-down,2,B}},
    { "SpecialShotAirC", {-down,2,-down,2,C}},
    { "Air_Assault", {2,3,6,C}},
    { "CurseShotAir", {2,3,6,D}},
    { "CurseShotLand", {2,3,6,D}},
    { "FaintLandA", {2,1,4,A}},
    { "FaintLandB", {2,1,4,B}},
    { "FaintLandC", {2,1,4,C}},
    { "FaintAirA", {2,1,4,A}},
    { "FaintAirB", {2,1,4,B}},
    { "FaintAirC", {2,1,4,C}},
    { "TriangleJumpLeft", {-4,4,-back,4}},
    { "TriangleJumpRight", {-4,4,-back,4}},
    { "TriangleJumpLeft", {-6,6,-forward,6}},
    { "TriangleJumpRight", {-6,6,-forward,6}},
    { "UltimateAntiAirShot", {2,6,2,6,C}},
    { "UltimateAntiAirShotOD", {2,6,2,6,C}},
    { "UltimateAirShot", {2,4,2,4,D}},
    { "UltimateAirShotOD", {2,4,2,4,D}},
    { "WarmSummon_Ultimate", {2,6,2,6,D}},
    { "WarmSummon_UltimateOD", {2,6,2,6,D}},
    { "WarmSummonAir_Ultimate", {2,6,2,6,D}},
    { "WarmSummonAir_UltimateOD", {2,6,2,6,D}},
    { "AstralHeat", {6,4,6,4,1,3,6,D}},
    { "AstralHeat", {6,4,6,4,1,3,6,D}},
};
std::vector<CommandActions> bangCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Strike", {6,2,3,B}},
    { "Strike_Air", {6,2,3,B}},
    { "SPThrow", {6,2,3,C}},
    { "SPAirThrow", {6,2,3,C}},
    { "Shot_A", {2,3,6,A}},
    { "Shot_B", {2,3,6,B}},
    { "Shot_C", {2,3,6,C}},
    { "Shot_D", {2,3,6,D}},
    { "Shot_AD", {2,3,6,A,D}},
    { "Shot_BD", {2,3,6,B,D}},
    { "Shot_CD", {2,3,6,C,D}},
    { "UltimateShot", {6,3,1,4,6,B}},
    { "UltimateShot", {6,2,4,6,B}},
    { "Musasabi", {-down,2,-down,2,A}},
    { "Stuck_A", {2,1,4,A}},
    { "Stuck_B", {2,1,4,B}},
    { "Stuck_C", {2,1,4,C}},
    { "Stuck_D", {2,1,4,D}},
    { "AirStuck_A", {2,1,4,A}},
    { "AirStuck_B", {2,1,4,B}},
    { "AirStuck_C", {2,1,4,C}},
    { "AirStuck_D", {2,1,4,D}},
    { "UltimateThrow", {2,6,2,4,C}},
    { "UltimateThrow", {2,3,6,3,1,4,C}},
    { "UltimateThrow_OD", {2,6,2,4,C}},
    { "UltimateThrow_OD", {2,3,6,3,1,4,C}},
    { "UltimateAntiAir", {2,6,2,6,A}},
    { "UltimateAntiAir_OD", {2,6,2,6,A}},
    { "UltimateSPThrow", {2,6,2,6,D}},
    { "UltimateSPThrow_OD", {2,6,2,6,D}},
    { "AstralHeat", {6,2,4,6,2,4,D}},
    { "AstralHeat", {6,3,1,4,6,3,1,4,D}},
};
std::vector<CommandActions> amaneCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Kamae", {2,3,6,D}},
    { "MultiStrike", {2,3,6,C}},
    { "AirMultiStrike", {2,3,6,C}},
    { "Anti_Air", {6,2,3,C}},
    { "FrontJumpA", {2,3,6,A}},
    { "FrontJumpB", {2,3,6,B}},
    { "AirFrontJumpA", {2,3,6,A}},
    { "AirFrontJumpB", {2,3,6,B}},
    { "BackJumpA", {2,1,4,A}},
    { "BackJumpB", {2,1,4,B}},
    { "AirBackJumpA", {2,1,4,A}},
    { "AirBackJumpB", {2,1,4,B}},
    { "Ginga", {2,1,4,C}},
    { "UltimateAssault", {2,6,2,6,D}},
    { "UltimateAssault_OD", {2,6,2,6,D}},
    { "ImperialDrill", {6,3,1,4,6,D}},
    { "ImperialDrill_OD", {6,3,1,4,6,D}},
    { "ImperialDrill", {6,2,4,6,D}},
    { "ImperialDrill_OD", {6,2,4,6,D}},
    { "AstralHeat", {-down,2-down,2-down,2,D}},
};
std::vector<CommandActions> jubeiCommands =
{
    { "CmnActFDash", {-6,6,-forward,6}},
    { "CmnActBDash", {-4,4,-back,4}},
    { "CmnActAirFDash", {-6,6,-forward,6}},
    { "CmnActAirFDash", {-9,9,-forward,6}},
    { "CmnActAirBDash", {-4,4,-back,4}},
    { "CmnActAirBDash", {-7,7,-back,4}},
    { "Assault_A", {2,3,6,A}},
    { "Assault_B", {2,6,2,6,A}},
    { "HexaEdge_1", {2,3,6,A}},
    { "Assault_Low", {2,3,6,B}},
    { "AirAssault_Mid", {2,1,4,B}},
    { "Assault_Mid", {2,1,4,B}},
    { "Assault_Multi", {2,1,4,C}},
    { "AirAssault_Multi", {2,1,4,C}},
    { "HexaEdge_Chage", {6,4,6,C}},
    { "Assault_ChageAttack", {6,2,4,6,C}},
    { "Assault_ChageAttack", {6,3,1,4,6,C}},
    { "Shot", {2,3,6,C}},
    { "AirShot", {2,3,6,C}},
    { "AirMoveFront", {2,3,6,D}},
    { "AirMoveBack", {2,1,4,D}},
    { "UltimateAssault", {2,6,2,6,D}},
    { "UltimateAssault_OD", {2,6,2,6,D}},
    { "UltimateAirAssault", {2,6,2,6,D}},
    { "UltimateAirAssault_OD", {2,6,2,6,D}},
    { "UltimateChage", {2,4,8,6,A}},
    { "UltimateChage", {2,6,8,4,A}},
    { "UltimateChage", {4,8,6,2,A}},
    { "UltimateChage", {4,2,6,8,A}},
    { "UltimateChage", {8,6,2,4,A}},
    { "UltimateChage", {8,4,2,6,A}},
    { "UltimateChage", {6,2,4,8,A}},
    { "UltimateChage", {6,8,4,2,A}},
    { "AstralHeat", {6,2,4,6,2,4,C}},
    { "AstralHeat", {6,3,1,4,6,3,1,4,C}},
    { "AstralHeat", {6,3,1,4,6,2,4,C}},
    { "AstralHeat", {6,2,4,6,3,1,4,C}},
};
std::vector<CommandActions> CbrReplayFile::FetchCommandActions(std::string charName) {
    if (charName == "es") { return esCommands; };//es
    if (charName == "ny") { return nuCommands; };//nu
    if (charName == "ma") { return maiCommands; };// mai
    if (charName == "tb") { return tsubakiCommands; };// tsubaki
    if (charName == "rg") { return ragnaCommands; };//ragna
    if (charName == "hz") { return hazamaCommands; };//hazama
    if (charName == "jn") { return jinCommands; };//jin
    if (charName == "mu") { return muCommands; };//mu
    if (charName == "no") { return noelCommands; };//noel
    if (charName == "mk") { return makotoCommands; };//makoto
    if (charName == "rc") { return rachelCommands; };//rachel
    if (charName == "vh") { return valkCommands; };//valk
    if (charName == "tk") { return taoCommands; };//taokaka
    if (charName == "pt") { return platinumCommands; };//platinum
    if (charName == "tg") { return tagerCommands; };//tager
    if (charName == "rl") { return reliusCommands; };//relius
    if (charName == "lc") { return litchiCommands; };//litchi
    if (charName == "iz") { return izayoiCommands; };//izayoi
    if (charName == "ar") { return arakuneCommands; };//arakune
    if (charName == "am") { return amaneCommands; };//amane
    if (charName == "bn") { return bangCommands; };//bang
    if (charName == "bl") { return bulletCommands; };//bullet
    if (charName == "ca") { return carlCommands; };//carl
    if (charName == "az") { return azraelCommands; };//azrael
    if (charName == "ha") { return hakuCommands; };//hakumen
    if (charName == "kg") { return kaguraCommands; };//kagura
    if (charName == "kk") { return kokonoeCommands; };//koko
    if (charName == "rm") { return lambdaCommands; };//lambda
    if (charName == "hb") { return hibikiCommands; };//hibiki
    if (charName == "tm") { return terumiCommands; };//terumi
    if (charName == "ph") { return nineCommands; };//nine
    if (charName == "ce") { return celicaCommands; };//Celica
    if (charName == "nt") { return naotoCommands; };//naoto
    if (charName == "mi") { return izanamiCommands; };//izanami
    if (charName == "su") { return susanCommands; };//susan
    if (charName == "jb") { return jubeiCommands; };//jubei
    return bulletCommands;
}