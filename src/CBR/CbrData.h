#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "Metadata.h"
#include "CbrReplayFile.h"

class CbrData {
private:
    friend class boost::serialization::access;
    std::string playerName;
    std::string characterName;
    std::vector<CbrReplayFile> replayFiles;
    int activeReplay;
    int activeCase;
    int activeFrame;

    bool enabled = false;

    


public:
    std::string debugText = "";
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& playerName& characterName& replayFiles;
    }
    CbrData();
    //player name,  characterName
    CbrData( std::string, std::string);
    void setEnabled(bool);
    bool getEnabled();
    void AddReplay(CbrReplayFile);
    CbrReplayFile* getLastReplay();
    int CBRcomputeNextAction(Metadata);
    int inverseInput(int);

    
    float comparisonFunction(Metadata, Metadata);
    float compRelativePosX(std::array< int, 2>, std::array< int, 2>);
    float compRelativePosY(std::array< int, 2>, std::array< int, 2>);
    float compState(std::string, std::string);
    float compNeutralState(bool, bool);
    float compAirborneState(bool, bool);
    float compWakeupState(bool, bool);
    float compBlockState(bool, bool);
    float compGetHitThisFrameState(bool, bool);
    float compBlockingThisFrameState(bool, bool);
    float compCrouching(bool, bool);
    float compDistanceToWall(std::array< int, 2>, std::array< int, 2>, bool, bool);
    void debugPrint(CbrCase, CbrCase, Metadata, float, float, int, int, int, int);
};