#pragma once

#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Game/CharData.h"
#include "Game/Jonb/JonbDBReader.h"
#include "Game/Scr/ScrStateReader.h"
#include "imgui.h"
#include <array>
#include <cstddef>
#include <deque>

// maximum number of frames to be kept track of in the history
const size_t HISTORY_DEPTH = 100;

/// An arbitrary, and abstract categorization of player state, several of these
/// are difficult to determine, they are here as a reminder to future
/// implementors
enum class FrameKind {
    Idle = 0x00,
    Startup = 0x01,
    Recovery = 0x02,
    Active = 0x04, // Indicates that there is an active hitbox
    Blockstun = 0x08,
    Hitstun = 0x10,
    // For moves which cannot be said to have a recovery, but which still don't
    // give enough cancel options to be called idle e.g. Izayoi teleports
    Special = 0x20,
    NotSpecial = ~Special,
    // States that rely on something other than character script to determine
    // length
    NonDeterministicActive =
    0x40 | Active, // used for the cases where the sprite length is 32767
    NonDeterministicRecovery =
    0x40 | Recovery, // used for the cases where the sprite length is 32767
    HardLanding = 0x80,
    Disarmed = Blockstun | Hitstun | Startup | Recovery | Active | HardLanding,
    // This is here to help with display
    Offense = Startup | Active | Recovery,
    Defense = Blockstun | Hitstun,
    // TODO: Come up with a better way to categorize these
    Misc = Special | HardLanding | 0x40,
};
inline FrameKind operator|(FrameKind a, FrameKind b) {
    return static_cast<FrameKind>(static_cast<int>(a) | static_cast<int>(b));
}
inline FrameKind operator&(FrameKind a, FrameKind b) {
    return static_cast<FrameKind>(static_cast<int>(a) & static_cast<int>(b));
}
inline FrameKind operator~(FrameKind a) {
    return static_cast<FrameKind>(~static_cast<int>(a));
}

// TODO: Consider changing this representation to be more in line with the game's

// General purpose attribute class for invul/guardpoint/attack attribute
enum class Attribute {

    // Move attributes
    H = 0x01,
    B = 0x02,
    F = 0x04,
    T = 0x08,
    P = 0x10,
    // Burst property, some weird things with this one
    O = 0x20,

    // The "no attribute", attribute. Necessary to represent moves with no invul,
    // for example.
    N = 0x00,
};

inline Attribute operator|(Attribute a, Attribute b) {
    return static_cast<Attribute>(static_cast<int>(a) | static_cast<int>(b));
}
inline Attribute operator&(Attribute a, Attribute b) {
    return static_cast<Attribute>(static_cast<int>(a) & static_cast<int>(b));
}
inline Attribute operator&(Attribute a, uint32 b) {
    return static_cast<Attribute>(static_cast<uint32>(a) & b);
}

struct BackedUpCharData {
    int32_t position_y;
};

// TODO: Represent invul, guardp, attack, and is_new
class PlayerFrameState {
public:
    FrameKind kind = FrameKind::Idle;
    Attribute invul = Attribute::N;
    Attribute guardp = Attribute::N;
    Attribute attack = Attribute::N;
    // Boolean to signify that this frame is the first from the current state
    bool is_new = false;
    bool loggable = true;

    PlayerFrameState(scrState* state, unsigned int frame, CharData* player, BackedUpCharData old_data);
    //PlayerFrameState(bool loggable);
    PlayerFrameState();
};

typedef std::array<PlayerFrameState, 2> StatePair;
typedef std::deque<StatePair> StatePairQueue;


struct FrameHistory {
public:
    FrameHistory();
    ~FrameHistory();

    // Update history with new data
    // Overwrites old history if both players have been previously idle
    void updateHistory(bool resetting);

    StatePairQueue& read();
    // void updateJonBMaps();

    // I would like to use these two to track state frame time and the char index
    // to guess at whether we've changed a character or not int32_t
    // frame_count_minus_1; //thanks to kding0 int32_t charIndex; //0x0034

    void clear();

private:
    StatePairQueue queue;
    bool is_old = false;

    // to check if characters changed
    int32_t p1_charIndex = -1;
    int32_t p2_charIndex = -1;
    // to track frames
    int32_t p1_frameCountMinus_1 = -2;
    int32_t p2_frameCountMinus_1 = -2;
    // to check for new states
    int32_t p1_stateChangedCount = -1;
    int32_t p2_stateChangedCount = -1;

    unsigned int p1_frames = 0;
    unsigned int p2_frames = 0;

    scrState* p1_State = NULL;
    scrState* p2_State = NULL;

    std::map<std::string, scrState*> p1_StateMap = {};
    std::map<std::string, scrState*> p2_StateMap = {};

    BackedUpCharData p1_old_data;
    BackedUpCharData p2_old_data;

    bool getPlayerFrameStates(CharData* player1, CharData* player2, StatePair*);
    void loadCharData();
    void backupChars();
};

std::array<float, 3> kindtoColor(FrameKind kind);
std::array<float, 3> attributetoColor(Attribute attr, std::array<Attribute, 3> rgb_attr);

const std::array<float, 3> GREEN = { 0, 1, 0 };
const std::array<float, 3> RED = { 1, 0, 0 };
const std::array<float, 3> BLUE = { 0, 0, 1 };
const std::array<float, 3> YELLOW = { 1, 1, 0 };
const std::array<float, 3> AQUAMARINE = { 0.5, 1, 0.83 };
const std::array<float, 3> PURPLE = { 1, 0, 1 };
const std::array<float, 3> BYZANTIUM = { 0.44, 0.14, 0.39 };
const std::array<float, 3> BURGUNDY = { 0.5, 0, 0.13 };
const std::array<float, 3> BLUSH = { 0.87, 0.36, 0.51 };
const std::array<float, 3> BLACK = { 0, 0, 0 };
