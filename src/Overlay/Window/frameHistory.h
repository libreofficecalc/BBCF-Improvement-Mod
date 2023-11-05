#pragma once

#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Game/CharData.h"
#include "Game/Jonb/JonbDBReader.h"
#include "Game/Scr/ScrStateReader.h"
#include <array>
#include <cstddef>
#include <deque>

// maximum number of frames to be kept track of in the history
const size_t HISTORY_DEPTH = 100;

char kindtoLetter(FrameKind kind);

// An arbitrary, and abstract categorization of player state, several of these
// are difficult to determine, they are here as a reminder to future
// implementors
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
  // States that rely on something other than character script to determine
  // length
  NonDeterministicAcive =
      0x40 | Active, // used for the cases where the sprite length is 32767
  NonDeterministicRecovery =
      0x40 | Recovery // used for the cases where the sprite length is 32767
};
inline FrameKind operator|(FrameKind a, FrameKind b) {
  return static_cast<FrameKind>(static_cast<int>(a) | static_cast<int>(b));
}

// General purpose attribute class for invul/guardpoint/attack attribute
enum class Attribute {

  // Move attributes
  F = 0x01,
  B = 0x02,
  H = 0x04,
  P = 0x08,
  T = 0x10,
  // Doll property
  D = 0x20,
  // Burst property, some weird things with this one
  O = 0x40,

  // The "no attribute", attribute. Necessary to represent moves with no invul,
  // for example.
  N = 0x00,
};

inline Attribute operator|(Attribute a, Attribute b) {
  return static_cast<Attribute>(static_cast<int>(a) | static_cast<int>(b));
}

struct PlayerFrameState {
public:
  // these are bitfields. Check values above
  // TODO: Justify the use of a bitfield for 'kind'
  FrameKind kind = FrameKind::Idle;
  Attribute invul = Attribute::N;
  Attribute guardp = Attribute::N;
  Attribute attack = Attribute::N;
  // Boolean to signify that this move
  bool is_new = false;

  // PlayerState(bool player_1, unsigned int frame, std::map<std::string,
  // JonbDBEntry>* jonbin_map);
  PlayerFrameState(scrState *state, unsigned int frames, CharData *player);
};

// TODO: Change this name
typedef std::array<PlayerFrameState, 2> StatePair;
typedef std::deque<StatePair> StatePairQueue;

// const StatePair BOTH_IDLE = {PlayerState::idle(), PlayerState(SAR::Idle, 0,
// 0, 0)};

struct FrameHistory {
public:
  FrameHistory();
  ~FrameHistory();

  // Update history with new data
  // Overwrites old history if both players have been previously idle
  void updateHistory();
  StatePairQueue &read();
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

  /* int32_t actionTime; //0x0160
  int32_t actionTime2; //0x0164
  int32_t actionTimeNoHitstop; //0x0170 */

  unsigned int p1_frames = 0;
  unsigned int p2_frames = 0;

  scrState *p1_State = NULL;
  scrState *p2_State = NULL;

  std::map<std::string, scrState *> p1_StateMap = {};
  std::map<std::string, scrState *> p2_StateMap = {};

  StatePair getPlayerFrameStates(CharData *player1, CharData *player2);
  void loadCharData();
};
