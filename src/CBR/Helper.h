#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <array>

class Helper {
private:
    friend class boost::serialization::access;


public:
    std::string type;
    size_t typeHash;
    int posX;
    int posY;
    std::string currentAction;
    size_t currentActionHash;
    bool proximityScale = false;

    int hitstun;
    int attackType;
    int hitstop;
    int actionTimeNoHitstop;

    bool attack;
    bool hit;
    bool hitThisFrame;
    bool facing;
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& type& typeHash& posX& posY& currentAction& currentActionHash& hitstun& attackType& hitstop& actionTimeNoHitstop& attack& hit& hitThisFrame& facing;
    }
    Helper();
    Helper(int p1x, int p1y, bool b, std::string p1action, int P1Hitstun, int P1AttackType, int p1Hitstop, int p1ActionTimeNHS);

    int getPosX();
    int getPosY();
    bool getFacing();
    std::string getCurrentAction();
    size_t getCurrentActionHash();
    int getHitstun();
    int getAttackType();
    int getHitstop();
    int getActionTimeNHS();
    bool getAttack();
    bool getHit();
    bool getHitThisFrame();

    bool CheckNeutralState(std::string&);
    void computeMetaData(std::string&);
    std::string PrintState();;
};
