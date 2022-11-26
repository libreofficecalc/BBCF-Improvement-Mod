#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include "Metadata.h"
class AnnotatedReplay {
private:
    friend class boost::serialization::access;
    int replayIndex;
    std::vector<std::shared_ptr<Metadata>> metaData;
    std::vector<int> input;
    std::string playerName;
    std::array< std::string, 2> characterName;
    std::array<int, 2> characterId;
    bool playing = false;


public:
    int debugFrameIndex = 0;
    int debugReplayIndex = 0;
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& playerName& characterName& metaData& input & characterId;
    }
    AnnotatedReplay();
    AnnotatedReplay::AnnotatedReplay(std::string n, std::string p1, std::string p2, int p1Id, int p2Id);
    void AddFrame(std::shared_ptr<Metadata>, int);
    std::vector<int> getInput();
    std::vector<int> copyInput();
    int AnnotatedReplay::getInputSize();
    std::vector<std::shared_ptr<Metadata>>* getAllMetadata();
    std::shared_ptr<Metadata> getMetadata(int);
    std::shared_ptr<Metadata>CopyMetadataPtr(int i);
    Metadata ViewMetadata(int);

    std::string getPlayerName();
    std::string getFocusCharName();
    std::array< std::string, 2> getCharacterName();
    int getNextInput(bool);
    void resetReplayIndex();

    bool getPlaying();
    void setPlaying(bool);
    int inverseInput(int input);
    std::array<int, 2> getCharIds();
    
};
