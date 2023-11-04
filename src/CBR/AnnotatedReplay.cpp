#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include "AnnotatedReplay.h"
#include "Metadata.h"
#include <memory>

AnnotatedReplay::AnnotatedReplay()
{
    replayIndex = 0;
}

AnnotatedReplay::AnnotatedReplay( std::string n, std::string p1, std::string p2, int p1Id, int p2Id) : playerName(n)
{
    characterName[0] = p1;
    characterName[1] = p2;
    replayIndex = 0;
    characterId[0] = p1Id;
    characterId[1] = p2Id;
    //metaData = {};
    //input = {};
}

void AnnotatedReplay::AddFrame(std::shared_ptr<Metadata> m, int i)
{
    metaData.push_back(std::move(m));
    input.push_back(i);
    debugFrameIndex++;
}

std::array<int, 2> AnnotatedReplay::getCharIds() {
    return characterId;
}
std::shared_ptr<Metadata> AnnotatedReplay::getMetadata(int i)
{
    auto ret = std::move(metaData[i]); 
    return ret;
}
std::vector<std::shared_ptr<Metadata>>* AnnotatedReplay::getAllMetadata() {
    return &metaData;
}
Metadata AnnotatedReplay::ViewMetadata(int i) {
//#ifdef _DEBUG
//    if (i >= metaData.size()) {
//        return *metaData[0];
//    }
//#endif
    return *metaData[i];
}
int AnnotatedReplay::MetadataSize() {
    //#ifdef _DEBUG
    //    if (i >= metaData.size()) {
    //        return *metaData[0];
    //    }
    //#endif
    return metaData.size();
}
std::shared_ptr<Metadata> AnnotatedReplay::CopyMetadataPtr(int i) {
    return metaData[i];
}

int AnnotatedReplay::getInputSize()
{
    return input.size();
}

std::vector<int> AnnotatedReplay::getInput() 
{
    return input;
}

std::string AnnotatedReplay::getPlayerName() {
    return playerName;
}
std::string AnnotatedReplay::getFocusCharName() {
    return characterName[0];
}
std::array< std::string, 2>& AnnotatedReplay::getCharacterName() {
    return characterName;
}
int AnnotatedReplay::getNextInput(bool facing) {
    auto ret = input[replayIndex];
    if (facing != metaData[replayIndex]->getFacing()) {
        ret = inverseInput(ret);
    }
    replayIndex++;
    if (replayIndex >= input.size()) {
        replayIndex = 0;
        playing = false;
    }
    return ret;
}

void AnnotatedReplay::resetReplayIndex() {
    replayIndex = 0;
}

bool AnnotatedReplay::getPlaying() {
    return playing;
}
void AnnotatedReplay::setPlaying(bool b) {
    playing = b;
}


#define specialButton 512
#define tauntButton 256
#define DButton 128
#define CButton 64
#define BButton 32
#define AButton 16
int AnnotatedReplay::inverseInput(int input) {
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

    if (buffer == 4 || buffer == 7 || buffer == 1) {
        return input + 2;
    }
    return input;
}