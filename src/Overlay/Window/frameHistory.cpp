#include "frameHistory.h"
#include "FrameAdvantage/PlayerExtendedData.h"
#include <cstddef>
#include <list>

// thanks to PCVolt
const std::list<std::string> idleWords = {
    "_NEUTRAL", "CmnActStand", "CmnActStandTurn", "CmnActStand2Crouch",
    "CmnActCrouch", "CmnActCrouchTurn", "CmnActCrouch2Stand", "CmnActFWalk",
    "CmnActBWalk", "CmnActFDash", "CmnActFDashStop", "CmnActJumpUpper",
    "CmnActJumpDown", "CmnActJumpUpperEnd", "CmnActJumpLanding",
    // "CmnActLandingStiffEnd", // Testing whether this is actually hard landing
    "CmnActUkemiLandNLanding", // to fix, 12F too long!
    // Proxi block is triggered when an attack is closing in without being
    // actually blocked If the player.blockstun is = 0, then those animations
    // are still considered idle
    "CmnActCrouchGuardPre", "CmnActCrouchGuardLoop", "CmnActCrouchGuardEnd", // Crouch
    "CmnActCrouchHeavyGuardPre", "CmnActCrouchHeavyGuardLoop", "CmnActCrouchHeavyGuardEnd", // Crouch Heavy
    "CmnActMidGuardPre", "CmnActMidGuardLoop", "CmnActMidGuardEnd", // Mid
    "CmnActMidHeavyGuardPre", "CmnActMidHeavyGuardLoop", "CmnActMidHeavyGuardEnd", // Mid Heavy
    "CmnActHighGuardPre", "CmnActHighGuardLoop", "CmnActHighGuardEnd", // High
    "CmnActHighHeavyGuardPre", "CmnActHighHeavyGuardLoop", "CmnActHighHeavyGuardEnd", // High Heavy
    "CmnActAirGuardPre", "CmnActAirGuardLoop", "CmnActAirGuardEnd", // Air
    // Character specifics
    "com3_kamae" // Mai 5xB stance
};

std::array<float, 3> attributetoColor(Attribute attr) {
    std::array<float, 3> res;
    auto distribution = [](float x, float scale) {
        return pow(log(x + 1) / log(scale + 1), 2.);
        };
    res[0] = distribution(static_cast<int>((Attribute::H)&attr),
        static_cast<int>(Attribute::H));
    res[1] = distribution(static_cast<int>((Attribute::B)&attr),
        static_cast<int>(Attribute::B));
    res[2] = distribution(static_cast<int>((Attribute::F)&attr),
        static_cast<int>(Attribute::F));
    return res;
}

std::array<float, 3> kindtoColor(FrameKind kind) {
    std::array<float, 3> res;
    // it's not so clear cut. Moves that don't let you be actionable in air maybe still be classified as idle,
    // because we remove hardlanding from immediate classification. Needs more testing
    FrameKind cleankind = kind & ~(static_cast<FrameKind>(0x40) | FrameKind::HardLanding);
    switch (cleankind)
    {
    case FrameKind::Idle:
        res = BLACK;
        break;
    case FrameKind::Startup:
        res = GREEN;
        break;
    case FrameKind::Recovery:
        if (static_cast<int>(kind & FrameKind::HardLanding) != 0) {
            res = BLUSH;
        }
        else {
            res = PURPLE;
        }
        break;
    case FrameKind::Active:
        res = RED;
        break;
    case FrameKind::Blockstun:
        res = YELLOW;
        break;
    case FrameKind::Hitstun:
        res = BLUE;
        break;
    case FrameKind::Special:
        // TODO: new condition: if hardlanding last frame, and we are in the state right after that on special
        if (static_cast<int>(kind & FrameKind::HardLanding) != 0) {
            res = BLUSH;
        }
        else {
            res = AQUAMARINE;
        }
        break;
    default:
        if (static_cast<int>(cleankind & FrameKind::Active) != 0) {
            res = RED;
        }

        else if (static_cast<int>(cleankind & FrameKind::Startup) != 0) {
            res = GREEN;

        }
        else if (static_cast<int>(cleankind & FrameKind::Recovery) != 0) {
            res = PURPLE;
        }
        else if (static_cast<int>(cleankind & FrameKind::Disarmed) != 0) {
            res = BYZANTIUM;

        }
        else {
            res = BURGUNDY;
        }
        break;
    }
    return res;
}

int first_det_active(std::vector<FrameActivity>& activity_status) {
    for (size_t i = 0; i < activity_status.size(); i++) {
        if (activity_status[i] == FrameActivity::Active) {
            return i;
        }
    }
    return -1;
}

PlayerFrameState::PlayerFrameState(scrState* state, unsigned int frame,
    CharData* player, BackedUpCharData old_data) {
    // TODO: Make this more robust.

    int active_1;
    if (state == nullptr) {
        active_1 = -1;
    }
    else {
        active_1 = first_det_active(state->frame_activity_status);
        if (state->frame_invuln_status.size() > frame)
            invul = static_cast<Attribute>(state->frame_invuln_status.at(frame));
    }
    is_new = frame == 0;
    // BUG: Check if dereferencing player causes a crash here (mirror matches)
    if (is_new && player->currentAction == "CmnActUkemiLandNLanding") {
        kind = FrameKind::Recovery;
    }
    else if (std::find(std::begin(idleWords), std::end(idleWords), player->currentAction) != std::end(idleWords)) {
        kind = FrameKind::Idle;
    }
    else {
        kind = FrameKind::Special;
    }
    if (player->hitboxCount > 0) {
        kind = FrameKind::Active | kind;
    }
    if (player->blockstun > 0) {
        kind = FrameKind::Blockstun | kind;
    }
    if (player->hitstun > 0) {
        kind = FrameKind::Hitstun | kind;
    }

    // hardlanding is set even if the player is still airborn. We only want to flag the *landing* portion
    if (player->hardLandingRecovery > 0 && player->position_y + old_data.position_y == 0) {
        kind = FrameKind::HardLanding | kind;
    }
    // NOTE: Startup is only defined in a context with deterministic active frames
    if (active_1 > -1 && player->hitboxCount <= 0) {
        if (frame < active_1) {
            kind = FrameKind::Startup | kind;
        }
        else {
            kind = FrameKind::Recovery | kind;
        }
    }

    // Clear the "Special" specification, if the state is already well defined
    if ((kind & FrameKind::Disarmed) != FrameKind::Idle) {
        kind = FrameKind::NotSpecial & kind;
    }

    return;
}
PlayerFrameState::PlayerFrameState()
{
}

bool FrameHistory::getPlayerFrameStates(CharData* player1,
    CharData* player2, StatePair* res) {
    // get the state
    // we need the amount of frames spent on the current state
    // Suppose we know the frame count for state n, if we changed states, start
    // another count otherwise, add the difference between our framecount, and the
    // global frame count.

    // If the actionTime hasn't yet changed, don't register this frame.
    bool condition1 = p1_frames == player1->actionTime - 1;
    if (player1->stateChangedCount != p1_stateChangedCount) {
        // if it is not, fetch the new states from the map
        auto p1_new_state = p1_StateMap.find(player1->currentAction);
        if (p1_new_state != p1_StateMap.end()) {
            p1_State = p1_StateMap.at(player1->currentAction);
        }
        else {
            p1_State = nullptr;
        }
        p1_frames = 0;
    }
    else {
        // could instead use the difference between local and global framecounts, to
        // increment this If there is hitstop, do not add a frame.
        p1_frames = player1->actionTime - 1;
    }
    bool condition2 = p2_frames == player2->actionTime - 1;
    if (player2->stateChangedCount != p2_stateChangedCount) {
        auto p2_new_state = p2_StateMap.find(player2->currentAction);
        if (p2_new_state != p2_StateMap.end()) {
            p2_State = p2_StateMap.at(player2->currentAction);
        }
        else {
            p2_State = nullptr;
        }
        p2_frames = 0;
    }
    else {
        p2_frames = player2->actionTime - 1;
    }

    // Note that frame calculation is first performed, because we don't want to
    // miss a state switch because of hitstop

    // Only skip writing to the grid, if both players are experiencing hitstop
    // (might be redundant)
    if (condition1 && condition2) {
        return false;
    }
    else {
        *res = { PlayerFrameState(p1_State, p1_frames, player1, p1_old_data),
            PlayerFrameState(p2_State, p2_frames, player2, p2_old_data) };
        return true;
    }
}

/// update the history queue with the new player states. Only call after the
/// game time has moved and by NO MORE than 1 frame
void FrameHistory::updateHistory() {
    CharData* p1 = g_interfaces.player1.GetData();
    CharData* p2 = g_interfaces.player2.GetData();

    if (p1->charIndex != p1_charIndex || p2->charIndex != p2_charIndex) {
        loadCharData();
    }

    // update the player states, return their parsed versions
    StatePair states = {   };

    if (!getPlayerFrameStates(p1, p2, &states)) {
        return;
    }

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
    }
    else {
        // if something is happening, push the info. But not before clearing out
        if (is_old) {
            queue.clear();
        }

        if (queue.size() == HISTORY_DEPTH) {
            queue.pop_front();
        }
        queue.push_back(states);
        is_old = false;
    }
    // after doing updates, store this information for later.
    backupPlayers();
}

void FrameHistory::backupPlayers()
{
    CharData* p1 = g_interfaces.player1.GetData();
    CharData* p2 = g_interfaces.player2.GetData();
    p1_old_data = BackedUpCharData();
    p2_old_data = BackedUpCharData();
    p1_old_data.position_y = p1->position_y;
    p2_old_data.position_y = p2->position_y;
}

StatePairQueue& FrameHistory::read() { return queue; }

// TODO: Add safety checks
void FrameHistory::loadCharData() {
    CharData* p1 = g_interfaces.player1.GetData();
    CharData* p2 = g_interfaces.player2.GetData();

    p1_charIndex = p1->charIndex;
    p2_charIndex = p2->charIndex;
    p1_frameCountMinus_1 = p1->frame_count_minus_1;
    p2_frameCountMinus_1 = p2->frame_count_minus_1;
    p1_stateChangedCount = p1->stateChangedCount - 1;
    p2_stateChangedCount = p2->stateChangedCount - 1;

    char* bbcf_base_adress = GetBbcfBaseAdress();

    std::vector<scrState*> states_p1 = parse_scr(bbcf_base_adress, 1);
    std::vector<scrState*> states_p2 = parse_scr(bbcf_base_adress, 2);

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
