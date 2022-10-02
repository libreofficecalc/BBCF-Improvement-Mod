#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <array>

class Metadata {
private:
    friend class boost::serialization::access;
    std::array< int, 2>posX;
    std::array< int, 2>posY;
    std::array< std::string, 2>currentAction;
    std::array< int, 2>blockstun;
    std::array< int, 2>hitstun;
    std::array< int, 2>attackType;
    std::array< int, 2>hitstop;
    std::array< int, 2>actionTimeNoHitstop;

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



public:
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& posX& posY& facing & currentAction;
    }
    Metadata();
    //p1PosX, p2PosX, p1PosY, p2PosY, facing, P1Action, P2Action, P1Blockstun, P2Blockstun, P1Hitstun, P2Hitstun, P1AttackType, P2AttackType, p1Hitstop, p2Hitstop, p1ActionTimeNHS, p2ActionTimeNHS
    Metadata(int, int, int, int, bool, std::string, std::string, int, int, int, int, int ,int, int, int, int ,int );

    std::array< int, 2> getPosX();
    std::array< int, 2>getPosY();
    bool getFacing();
    std::array< std::string, 2>getCurrentAction();
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
};
