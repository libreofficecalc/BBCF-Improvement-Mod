#include "frameHistory.h"
#include "framedata.h"
#include <cstddef>

// thanks to PCVolt
const std::list<std::string> idleWords =
{ "_NEUTRAL", "CmnActStand", "CmnActStandTurn", "CmnActStand2Crouch",
"CmnActCrouch", "CmnActCrouchTurn", "CmnActCrouch2Stand",
"CmnActFWalk", "CmnActBWalk",
"CmnActFDash", "CmnActFDashStop",
"CmnActJumpLanding", "CmnActLandingStiffEnd",
"CmnActUkemiLandNLanding", // to fix, 12F too long!
// Proxi block is triggered when an attack is closing in without being actually blocked
// If the player.blockstun is = 0, then those animations are still considered idle
"CmnActCrouchGuardPre", "CmnActCrouchGuardLoop", "CmnActCrouchGuardEnd",                 // Crouch
"CmnActCrouchHeavyGuardPre", "CmnActCrouchHeavyGuardLoop", "CmnActCrouchHeavyGuardEnd",  // Crouch Heavy
"CmnActMidGuardPre", "CmnActMidGuardLoop", "CmnActMidGuardEnd",                          // Mid
"CmnActMidHeavyGuardPre", "CmnActMidHeavyGuardLoop", "CmnActMidHeavyGuardEnd",           // Mid Heavy
"CmnActHighGuardPre", "CmnActHighGuardLoop", "CmnActHighGuardEnd",                       // High
"CmnActHighHeavyGuardPre", "CmnActHighHeavyGuardLoop", "CmnActHighHeavyGuardEnd",        // High Heavy
"CmnActAirGuardPre", "CmnActAirGuardLoop", "CmnActAirGuardEnd",                          // Air
// Character specifics
"com3_kamae" // Mai 5xB stance
};


char kindtoLetter(FrameKind kind) {
  // Disregard determinism
  int res = static_cast<int>(kind) & (~0x40);
  switch (static_cast<FrameKind>(res)) {
  case FrameKind::Idle:
    return 'I';
  case FrameKind::Startup:
    return 'S';
  case FrameKind::Active:
    return 'A';
  case FrameKind::Recovery:
    return 'R';
  case FrameKind::Blockstun:
    return 'B';
  case FrameKind::Hitstun:
    return 'H';
  case FrameKind::Special:
    return 'P';
  default:
    break;
  }
}

PlayerFrameState::PlayerFrameState(scrState *state, unsigned int frame,
                                   CharData *player) {
  // TODO: Make this more robust.

  if (state == nullptr) {
    return;
  }
  int active = -1;
  for (size_t i = 0; i < state->frame_activity_status.size(); i++)
  {
      if (static_cast<int>(state->frame_activity_status[i]) & static_cast<int>(FrameActivity::Active)) {
          active = i;
          break;
      }
  }

  if (player->blockstun > 0) {
    kind = FrameKind::Blockstun | kind;
  }
  if (player->hitstun > 0) {
      kind = FrameKind::Hitstun | kind;
  }
  if (isDoingActionInList(state->name.c_str(), idleWords)) {
      kind = FrameKind::Idle;
  } 
  if (len != 0) {
    // HACK: kind of hacky :/, might change later
    for (size_t i = 0; i < len; i += 2) {
      if (state->active_ranges[i] <= frame &&
          state->active_ranges[i + 1] >= frame) {
        kind |= ACTIVE;
        break;
      }
    }

    if ((kind & ACTIVE) == 0) {
      if (frame < state->active_ranges[0]) {
        kind |= STARTUP;
      } else {
        kind |= RECOVERY;
      }
    }
  } else {
    kind |= SPECIAL;
  }
  return;
}

StatePair FrameHistory::getPlayerFrameStates(CharData *player1,
                                             CharData *player2) {
  // get the state
  // we need the amount of frames spent on the current state
  // Suppose we know the frame count for state n, if we changed states, start
  // another count otherwise, add the difference between our framecount, and the
  // global frame count. I could use the global frame count for this, or the
  // global players, I am just going to write something.

  if (player1->stateChangedCount != p1_stateChangedCount) {
    // if it is not, fetch the new states from the map
    auto p1_new_state = p1_StateMap.find(player1->currentAction);
    if (p1_new_state != p1_StateMap.end()) {
      p1_State = p1_StateMap.at(player1->currentAction);
    } else {
      p1_State = nullptr;
    }
    p1_frames = 0;
  } else {
    // could instead use the difference, to increment this
    p1_frames += 1;
  }

  if (player2->stateChangedCount != p2_stateChangedCount) {
    auto p2_new_state = p2_StateMap.find(player2->currentAction);
    if (p2_new_state != p2_StateMap.end()) {
      p2_State = p2_StateMap.at(player2->currentAction);
    } else {
      p2_State = nullptr;
    }
    p2_frames = 0;
  } else {
    p2_frames += 1;
  }

  return {PlayerFrameState(p1_State, p1_frames, player1),
          PlayerFrameState(p2_State, p2_frames, player2)};
}

/// update the history queue with the new player states. Only call after the
/// game time has moved and by NO MORE than 1 frame
void FrameHistory::updateHistory() {
  CharData *p1 = g_interfaces.player1.GetData();
  CharData *p2 = g_interfaces.player2.GetData();

  if (p1->charIndex != p1_charIndex || p2->charIndex != p2_charIndex) {
    loadCharData();
  }

  // update the player states, return their parsed versions
  StatePair states = getPlayerFrameStates(p1, p2);

  // sync up everything
  p1_frameCountMinus_1 = p1->frame_count_minus_1;
  p2_frameCountMinus_1 = p2->frame_count_minus_1;
  p1_stateChangedCount = p1->stateChangedCount;
  p2_stateChangedCount = p2->stateChangedCount;

  if (states[0].kind == FrameKind::Idle && states[1].kind == FrameKind::Idle) {
    // for the first case, keep the queue as it is. There is nothing of note
    // to write Signal to later calls that we have had both players be idle
    // before.
    is_old = true;
  } else {
    // if something is happening, push the info. But not before clearing out
    if (is_old) {
      // clear the queue
      /* size_t length = queue.size();
      for (size_t i = 0; i < length; i++) {
        queue.pop_front();
      } */
      queue.clear();
    }

    if (queue.size() == HISTORY_DEPTH) {
      queue.pop_front();
    }
    queue.push_back(states);
    is_old = false;
  }
}

StatePairQueue &FrameHistory::read() { return queue; }

// TODO: Add safety checks
void FrameHistory::loadCharData() {
  CharData *p1 = g_interfaces.player1.GetData();
  CharData *p2 = g_interfaces.player2.GetData();

  p1_charIndex = p1->charIndex;
  p2_charIndex = p2->charIndex;
  p1_frameCountMinus_1 = p1->frame_count_minus_1;
  p2_frameCountMinus_1 = p2->frame_count_minus_1;
  p1_stateChangedCount = p1->stateChangedCount - 1;
  p2_stateChangedCount = p2->stateChangedCount - 1;

  char *bbcf_base_adress = GetBbcfBaseAdress();

  std::vector<scrState *> states_p1 = parse_scr(bbcf_base_adress, 1);
  std::vector<scrState *> states_p2 = parse_scr(bbcf_base_adress, 2);

  for (size_t i1 = 0; i1 < states_p1.size(); i1++) {
    p1_StateMap[states_p1[i1]->name] = states_p1[i1];
  }

  for (size_t i2 = 0; i2 < states_p2.size(); i2++) {
    p2_StateMap[states_p2[i2]->name] = states_p2[i2];
  }
}

void FrameHistory::clear() { queue.clear(); }

FrameHistory::FrameHistory() { queue = std::deque<StatePair>(); }

FrameHistory::~FrameHistory() {}
