#include "frameHistory.h"
#include "framedata.h"
#include <cstddef>

// thanks to PCVolt
const std::list<std::string> idleWords =
{ "_NEUTRAL", "CmnActStand", "CmnActStandTurn", "CmnActStand2Crouch",
"CmnActCrouch", "CmnActCrouchTurn", "CmnActCrouch2Stand",
"CmnActFWalk", "CmnActBWalk",
"CmnActFDash", "CmnActFDashStop", "CmnActJumpUpper","CmnActJumpDown","CmnActJumpUpperEnd",
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

std::array<float, 3> kindtoColor(FrameKind kind) {
    std::array<float, 3> res;
    auto distribution = [](float x, float scale) { return pow(log(x + 1) / log(scale + 1), 2.); };
    res[0] = distribution(static_cast<int>(FrameKind::Offense & kind), static_cast<int>(FrameKind::Offense));
    res[1] = distribution(static_cast<int>(FrameKind::Defense & kind), static_cast<int>(FrameKind::Defense));
    res[2] = distribution(static_cast<int>(FrameKind::Misc & kind), static_cast<int>(FrameKind::Misc));
    return res;
}

char kindtoLetter(FrameKind kind) {
  // Disregard determinism and landing (they only affect colors
  int res = static_cast<int>(kind) & (~(0x40 | static_cast<int>(FrameKind::HardLanding)));
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
    return 'U';
  }
}
int first_det_active(std::vector<FrameActivity> &activity_status) {
    for (size_t i = 0; i < activity_status.size(); i++)
    {
        if (activity_status[i] == FrameActivity::Active) {
            return i;
        }
    }
    return -1;
}
PlayerFrameState::PlayerFrameState(scrState* state, unsigned int frame,
    CharData* player, bool active) {
    // TODO: Make this more robust.

    int active_1;
    if (state == nullptr) {
        active_1 = -1;
    }
    else {
        active_1 = first_det_active(state->frame_activity_status);
    }
    is_new = frame == 0;
    
    if (isDoingActionInList(player->currentAction, idleWords)) {
        kind = FrameKind::Idle;
    }
    else {
        kind = FrameKind::Special;
    }
    if (active)
    {
        kind = FrameKind::Active | kind;
    }
    if (player->blockstun > 0) {
        kind = FrameKind::Blockstun | kind;
    }
    if (player->hitstun > 0) {
        kind = FrameKind::Hitstun | kind;
    }

    if (player->hardLandingRecovery > 0) {
        kind = FrameKind::HardLanding | kind;
    }
    // NOTE: Startup is only defined in a context with deterministic active frames
    if (active_1 > -1 && !active) {
        if (frame < active_1) {
            kind = FrameKind::Startup | kind;
        }
        else {
            kind = FrameKind::Recovery | kind;
        }
    }
    
    // Clear the "Special" specification, if the state is already well defined
    if  ((kind & FrameKind::Disarmed) != FrameKind::Idle) {
      kind = FrameKind::NotSpecial & kind;
    }

    // invul = static_cast<Attribute>(state->frame_invuln_status[frame]);
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

  return {PlayerFrameState(p1_State, p1_frames, player1, player1->hitboxCount > 0),
          PlayerFrameState(p2_State, p2_frames, player2, player2->hitboxCount > 0)};
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
