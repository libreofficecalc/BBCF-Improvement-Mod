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

    

public:
    std::array< int, 2>timeAfterRecovery;
    int matchState = -1;
    std::array< int, 2> heatMeter;
    std::array< int, 2> overdriveMeter;
    std::array< int, 2> overdriveTimeleft;

    //CharacterSpecific
    std::array< int, 2> CharSpecific1 = { 0, 0 };
    std::array< int, 2> CharSpecific2 = { 0, 0 };
    std::array< int, 2> CharSpecific3 = { 0, 0 };
    std::array< int, 2> CharSpecific4 = { 0, 0 };
    std::array< int, 2> CharSpecific5 = { 0, 0 };
    std::array< int, 2> CharSpecific6 = { 0, 0 };
    std::array< int, 2> CharSpecific7 = { 0, 0 };
    std::array< int, 2> CharSpecific8 = { 0, 0 };
    std::array< int, 2> CharSpecific9 = { 0, 0 };

    //bulletSpecificData
    //int buHeat = -1;

    //litchiSpecificData
    //int lcStaff = -1;

    //Tager
    //std::array<bool, 2> tagerSparkRdy = { false, false };
    //std::array<bool, 2> tagerMagnetized = { false, false };

    //rachelWind
    //int RachelWindMeter = -1;

    //Azrael
    //std::array< int, 2> azFireball = { 0, 0 };
    //bool azTopWeakspot = -1;
    //bool azBotWeakspot = -1;

    //Es
    //bool EsBuff = -1;

    //Nu
    //bool NuGravity = -1;

    //Hazama
    //std::array< int, 2> hazamaChains = { 0, 0 };

    //Celica
    //int CelicaRegen = -1;

    //Platinum
    //std::array< int, 2> PlatCurItem = { 0, 0 };
    //std::array< int, 2> PlatNextItem = { 0, 0 };
    //std::array< int, 2> PlatItemNr = { 0, 0 };

    //Relius
    //std::array< int, 2> ReliusDollMeter = { 0, 0 };
    //std::array< int, 2> ReliusDollState = { 0, 0 };
    //std::array< bool, 2> ReliusDollInCooldown = { 0, 0 };

    //Valkenhein
    //std::array< int, 2> ValkWolfState = { 0, 0 };
    //std::array< int, 2> ValkWolfMeter = { 0, 0 };

    //Susano
    //std::array< int, 2> SusanUnlocks1 = { 0, 0 };
    //std::array< int, 2> SusanUnlocks2 = { 0, 0 };
    //std::array< int, 2> SusanUnlocks3 = { 0, 0 };
    //std::array< int, 2> SusanUnlocks4 = { 0, 0 };
    //std::array< int, 2> SusanUnlocks5 = { 0, 0 };
    //std::array< int, 2> SusanUnlocks6 = { 0, 0 };
    //std::array< int, 2> SusanUnlocks7 = { 0, 0 };
    //std::array< int, 2> SusanUnlocks8 = { 0, 0 };
    //int SusanDrivePosition = 0;

    //Jubei
    //std::array< bool, 2> JubeiBuff = { 0, 0 };
    //std::array< bool, 2> JubeiMark = { 0, 0 };

    //Izanami
    //std::array< bool, 2> IzanamiShotCooldown = { 0, 0 };
    //std::array< bool, 2> IzanamiRibcage = { 0, 0 };
    //std::array< bool, 2> IzanamiFloating = { 0, 0 };
    //std::array< bool, 2> IzanamiStance = { 0, 0 };

    //Nine
    //std::array< int, 2> NineSpell = { 0, 0 };
    //std::array< int, 2> NineSpellBackup = { 0, 0 };
    //std::array< int, 2> NineSpellSlots = { 0, 0 };
    //std::array< int, 2> NineSpellSlotsBackup = { 0, 0 };

    //Carl
    //std::array< int, 2> CarlDollMeter = { 0, 0 };
    //std::array< bool, 2> CarlDollInactive = { 0, 0 };

    //Tsubaki
    //std::array< int, 2> TsubakiMeter = { 0, 0 };

    //Kokonoe
    //std::array< int, 2> KokoGravitron = { 0, 0 };

    //Izayoi
    //std::array< bool, 2> IzayoiState = { 0, 0 };
    //std::array< int, 2> IzayoiStock = { 0, 0 };
    //std::array< int, 2> IzayoiSupermode = { 0, 0 };

    //Arakune
    //std::array< bool, 2> ArakuneCurseOn = { 0, 0 };
    //std::array< int, 2> ArakuneCurseMeter = { 0, 0 };

    //Arakune
    //std::array< int, 2> BangNails = { 0, 0 };
    //std::array< bool, 2> BangFrkz1 = { 0, 0 };
    //std::array< bool, 2> BangFrkz2 = { 0, 0 };
    //std::array< bool, 2> BangFrkz3 = { 0, 0 };
    //std::array< bool, 2> BangFrkz4 = { 0, 0 };

    //Amane
    //std::array< int, 2> AmaneDrillMeter = { 0, 0 };
    //std::array< bool, 2> AmaneDrillOverheat = { 0, 0 };

    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& posX& posY& facing& currentAction& lastAction& currentActionHash&
            lastActionHash& blockstun& hitstun& attackType& hitstop& timeAfterRecovery&
            actionTimeNoHitstop& comboProration& starterRating& comboTime&
            neutral& attack& wakeup& blocking& hit& hitThisFrame& BlockThisFrame& air& crouching& inputBufferActive& helpers& matchState& 
            heatMeter& overdriveMeter& overdriveTimeleft&
            CharSpecific1& CharSpecific2& CharSpecific3& CharSpecific4& CharSpecific5& CharSpecific6& CharSpecific7& CharSpecific8& CharSpecific9;
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
