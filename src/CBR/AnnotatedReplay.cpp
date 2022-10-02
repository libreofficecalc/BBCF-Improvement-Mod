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

AnnotatedReplay::AnnotatedReplay( std::string n, std::string p1, std::string p2) : playerName(n)
{
    characterName[0] = p1;
    characterName[1] = p2;
    replayIndex = 0;
    //metaData = {};
    //input = {};
}

void AnnotatedReplay::AddFrame(std::shared_ptr<Metadata> m, int i)
{
    metaData.push_back(std::move(m));
    input.push_back(i);
    debugFrameIndex++;
}

std::shared_ptr<Metadata> AnnotatedReplay::getMetadata(int i)
{
    auto ret = std::move(metaData[i]);
    return ret;
}

Metadata AnnotatedReplay::ViewMetadata(int i) {
    return *metaData[i];
}

std::shared_ptr<Metadata> AnnotatedReplay::CopyMetadataPtr(int i) {
    return metaData[i];
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
std::array< std::string, 2> AnnotatedReplay::getCharacterName() {
    return characterName;
}
int AnnotatedReplay::getNextInput() {
    auto ret = input[replayIndex];
    replayIndex++;
    if (replayIndex >= input.size()) {
        replayIndex = 0;
    }
    return ret;
}

