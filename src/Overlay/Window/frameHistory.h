#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include "Core/utils.h"
#include "Game/CharData.h"
#include "Core/interfaces.h"
#include "Game/Jonb/JonbDBReader.h"
#include "Game/Scr/ScrStateReader.h"

// maximum number of frames to be kept track of in the history
const size_t HISTORY_DEPTH = 100;

// Move properties
const unsigned char F_PROP = 0x01;
const unsigned char B_PROP = 0x02;
const unsigned char H_PROP = 0x04;
const unsigned char P_PROP = 0x08;
const unsigned char T_PROP = 0x10;
// Doll property 
const unsigned char D_PROP = 0x20;
// Burst property, some weird things with this one
const unsigned char O_PROP = 0x40;


// This state is not well defined.
// It is left as 0, because it cannot coincide with any other of these
const unsigned char IDLE = 0x00;
// These are only well defined in the case of moves with well defined active frames
const unsigned char STARTUP = 0x01;
const unsigned char RECOVERY = 0x02;
const unsigned char ACTIVE = 0x04;
const unsigned char BLOCKSTUN = 0x08;
const unsigned char HITSTUN = 0x10;
// anything which cannot have active frames, and is not any of the above, but is still not quite idle
const unsigned char SPECIAL = 0x20;

char kindtoLetter(unsigned char kind);

struct PlayerFrameState {
public:
  // these are bitfields. Check values above
  // TODO: Justify the use of a bitfield for 'kind'
  unsigned char kind   = 0;
  unsigned char invul  = 0;
  unsigned char guardp = 0;
  unsigned char attack = 0;

  // PlayerState(bool player_1, unsigned int frame, std::map<std::string, JonbDBEntry>* jonbin_map);
  PlayerFrameState(scrState* state, unsigned int frames, CharData *player);
};


// TODO: Change this name
typedef std::array<PlayerFrameState, 2> StatePair;
typedef std::deque<StatePair> StatePairQueue;


// const StatePair BOTH_IDLE = {PlayerState::idle(), PlayerState(SAR::Idle, 0, 0, 0)};

struct FrameHistory {
public:
  FrameHistory();
  ~FrameHistory();

  // Update history with new data
  // Overwrites old history if both players have been previously idle
  void updateHistory();
  StatePairQueue &read();
  // void updateJonBMaps();
  
  
  // I would like to use these two to track state frame time and the char index to guess at whether we've changed a character or not
	// int32_t frame_count_minus_1; //thanks to kding0
	// int32_t charIndex; //0x0034
	
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

  scrState* p1_State = NULL;
  scrState* p2_State = NULL;
  
  std::map<std::string, scrState*> p1_StateMap = {};
  std::map<std::string, scrState*> p2_StateMap = {};
  

  StatePair getPlayerFrameStates(CharData* player1, CharData* player2);
  void loadCharData();
};
