#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <array>
#include "Helper.h"
#include <boost/serialization/shared_ptr.hpp>

class Metadata {
private:
    friend class boost::serialization::access;
    std::array< int, 2>posX;
    std::array< int, 2>posY;
    std::array< std::string, 2>currentAction;
    std::array< std::string, 2>lastAction;
    std::array< size_t, 2>currentActionHash;
    std::array< size_t, 2>lastActionHash;
    int frame_count_minus_1;

    std::array< int, 2>blockstun;
    std::array< int, 2>hitstun;
    std::array< int, 2>attackType;
    std::array< int, 2>hitstop;
    std::array< int, 2>actionTimeNoHitstop;

    std::array< int, 2>comboProration;
    std::array< int, 2>starterRating;
    std::array< int, 2>comboTime;
    std::array< bool, 2>neutral;
    std::array< bool, 2>attack;
    std::array< bool, 2>wakeup;
    std::array< bool, 2>blocking;
    std::array< bool, 2>hit;
    std::array< bool, 2>hitThisFrame;
    std::array< bool, 2>BlockThisFrame;
    std::array< bool, 2>air;
    std::array< bool, 2>crouching;
    bool facing;
    bool inputBufferActive = false;

    //helpers
    std::array<std::vector<std::shared_ptr<Helper>>, 2> helpers = {{}};

    //bulletSpecificData
    int buHeat = -1;



    

public:
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& posX& posY& facing & currentAction & lastAction & currentActionHash & 
            lastActionHash & blockstun & hitstun & attackType & hitstop & 
            actionTimeNoHitstop & comboProration & starterRating & comboTime & 
            neutral & attack & wakeup & blocking & hit & hitThisFrame & BlockThisFrame & air & crouching & inputBufferActive & helpers;
    }
    Metadata();
    Metadata(int p1x, int p2x, int p1y, int p2y, bool b, std::string p1action, std::string p2action, int P1Blockstun, int P2Blockstun, int P1Hitstun, int P2Hitstun, int P1AttackType, int P2AttackType, int p1Hitstop, int p2Hitstop, int p1ActionTimeNHS, int p2ActionTimeNHS, char* p1LastAction, char* p2LastAction);

    void Metadata::addHelper(std::shared_ptr<Helper> h, int playerIndex);
    std::array<std::vector<std::shared_ptr<Helper>>, 2> Metadata::getHelpers();
    void setInputBufferActive(bool);
    bool getInputBufferActive();
    void SetFrameCount(int frameCount);
    int getFrameCount();
    std::array< int, 2> getPosX();
    std::array< int, 2>getPosY();
    bool getFacing();
    std::array< std::string, 2>getCurrentAction();
    std::array< std::string, 2>getLastAction();
    std::array< size_t, 2>getCurrentActionHash();
    std::array< size_t, 2>getLastActionHash();
    std::array< int, 2> getBlockstun();
    std::array< int, 2> getHitstun();
    std::array< int, 2> getAttackType();
    std::array< int, 2> getHitstop();
    std::array< int, 2> getActionTimeNHS();
    std::array< bool, 2> getNeutral();
    std::array< bool, 2> getAttack();
    std::array< bool, 2> getWakeup();
    std::array< bool, 2> getBlocking();
    std::array< bool, 2> getHit();
    std::array< bool, 2> getHitThisFrame();
    std::array< bool, 2> getBlockThisFrame();
    std::array< bool, 2> getAir();
    std::array< bool, 2> getCrouching();
    bool CheckNeutralState(std::string);
    void computeMetaData();
    bool CheckWakeupState(std::string);
    bool CheckCrouchingState(std::string);
    std::string PrintState();
    void SetComboVariables(int p1comboProration, int p2comboProration, int p1starterRating, int p2starterRating, int p1comboTime, int p2comboTime);
    std::array< int, 2> getComboProration();
    std::array< int, 2> getStarterRating();
    std::array< int, 2> getComboTime();

};
