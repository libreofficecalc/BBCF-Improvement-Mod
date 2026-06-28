#include "FrameHistory.h"
#include "Overlay/Window/FrameAdvantage/PlayerExtendedData.h"
#include <cstddef>
#include "Core/logger.h"
#include "Overlay/Logger/ImGuiLogger.h"
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))


// thanks to PCVolt
const std::vector<std::string> idleWords = {
    // now classified under "Special"
    // "CmnActFDash",

    
    "_NEUTRAL", "CmnActStand", "CmnActStandTurn", "CmnActStand2Crouch",
    "CmnActCrouch", "CmnActCrouchTurn", "CmnActCrouch2Stand", "CmnActFWalk",
    "CmnActBWalk", "CmnActFDashStop", "CmnActJumpUpper",
    "CmnActJumpDown", "CmnActJumpUpperEnd", "CmnActJumpLanding",
    "CmnActLandingStiffEnd",
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

// TODO: Make this use arbitrary bases vectors
std::array<float, 3> attributetoColor(Attribute attr, std::array<Attribute, 3> rgb_attr) {
    std::array<float, 3> res = {};
    
    res[0] = static_cast<int>(rgb_attr[0] & attr) > 0;
    res[1] = static_cast<int>(rgb_attr[1] & attr) > 0;
    res[2] = static_cast<int>(rgb_attr[2] & attr) > 0;
    
    return res;
}

std::array<float, 3> kindtoColor(FrameKind kind) {
    std::array<float, 3> res;
    // it's not so clear cut. Moves that don't let you be actionable in air may still be classified as idle,
    // TODO: because we remove hardlanding from immediate classification. Needs more testing
    FrameKind cleankind = kind & ~(static_cast<FrameKind>(0x40) | FrameKind::HardLanding);
    switch (cleankind)
    {
    case FrameKind::Idle:
        if (static_cast<int>(kind & FrameKind::HardLanding) != 0) {
            res = BLUSH;
        }
        else {
            res = BLACK;
        }
        break;
    case FrameKind::Startup:
        res = GREEN;
        break;
    case FrameKind::Recovery:
        if (static_cast<int>(kind & FrameKind::HardLanding) != 0) {
            res = BLUSH;
        }
        else {
            res = BLUE;
        }
        break;
    case FrameKind::Active:
        res = RED;
        break;
    case FrameKind::Blockstun:
        res = YELLOW;
        break;
    case FrameKind::Hitstun:
        res = PURPLE;
        break;
    case FrameKind::Special:
        res = AQUAMARINE;
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


Attribute parse_dyn_invul(uint32 invul_field, uint32 gp_bitfield) {
    if ((invul_field & 0x02) == 0) {
        return Attribute::N;
    }

    Attribute invul = Attribute::N;

    if (gp_bitfield & 0x01)     invul = invul | Attribute::GP;
    if (invul_field & 0x08)     invul = invul | Attribute::H;
    if (invul_field & 0x10)     invul = invul | Attribute::B;
    if (invul_field & 0x20)     invul = invul | Attribute::F;
    if (invul_field & 0x80000)  invul = invul | Attribute::P;
    if (invul_field & 0x40)     invul = invul | Attribute::T;



    return invul;
}

PlayerFrameState::PlayerFrameState(scrState* state, unsigned int frame,
    CharData* player, BackedUpCharData old_data) {

    // set state variables
    invul = parse_dyn_invul(player->invuln_bitfield, player->guard_point_bitfield);
    is_new = frame == 0;
    
    
    // Set kind
    std::string currentAction = std::string(player->currentAction);
    int fst_det_active;
    Attribute det_invul = Attribute::N;
    bool is_idle_state = std::find(std::begin(idleWords), std::end(idleWords), currentAction) != std::end(idleWords);
    
    if (state == nullptr) {
        fst_det_active = -1;
    }
    else {
        fst_det_active = first_det_active(state->frame_activity_status);
    }
    

    if (is_new && currentAction == "CmnActUkemiLandNLanding") {
        kind = FrameKind::Recovery;
    }
    else if (is_idle_state) {
        kind = FrameKind::Idle;
    }
    else {
        kind = FrameKind::Special;
    }
    if (player->hitboxCount > 0
        && (player->bitflags_for_curr_state_properties_or_smth & (0x400 | 0x200)) == 0) {
        kind = FrameKind::Active | kind;
    }
    if (player->blockstun > 0) {
        kind = FrameKind::Blockstun | kind;
    }
    if (player->hitstun > 0) {
        kind = FrameKind::Hitstun | kind;
    }

    // hardlanding is set even if the player is still airborn. We only want to flag the *landing* portion
    if (/*player->hardLandingRecovery > 0 && player->position_y + old_data.position_y == 0 && */currentAction == "CmnActLandingStiffLoop") {
        kind = FrameKind::HardLanding | kind;
    }
    // NOTE: Startup is only defined in a context with deterministic active frames
    if (fst_det_active > -1
        && (player->hitboxCount <= 0 || (player->bitflags_for_curr_state_properties_or_smth & (0x400 | 0x200)) == 0)
        && !is_idle_state) {
        
        if (frame < fst_det_active) {
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
    // we need the amount of frames spent on the current state, in order to index into the state frames.
    // TODO: Might want to count frames since last update, and add those to the p1_frames. However, what if a state was changed in between updates, then we can't know.
    // This is all the more reason to query states purely dynamically.
    std::string currentAction = std::string(player1->currentAction);
    // If the actionTime hasn't yet changed, don't register this frame.
    bool condition1 = p1_frames == player1->actionTime - 1;
    if (player1->stateChangedCount != p1_stateChangedCount) {
        // if it is not, fetch the new states from the map
        auto p1_new_state = p1_StateMap.find(currentAction);
        if (p1_new_state != p1_StateMap.end()) {
            p1_State = p1_StateMap.at(currentAction);
        }
        else {
            p1_State = nullptr;
        }
        p1_frames = 0;
    }
    else {
        // could instead use the difference between local and global framecounts, to
        // increment this. If there is hitstop, do not add a frame.
        p1_frames = player1->actionTime - 1;
    }
    currentAction = std::string(player2->currentAction);
    bool condition2 = p2_frames == player2->actionTime - 1;
    if (player2->stateChangedCount != p2_stateChangedCount) {
        auto p2_new_state = p2_StateMap.find(currentAction);
        if (p2_new_state != p2_StateMap.end()) {
            p2_State = p2_StateMap.at(currentAction);
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
void FrameHistory::updateHistory(bool resetting) {
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


    // TODO: If you add attack and guardp, add them to this condition
    // Reset on completely idle frames
    if (
         (states[0].kind == FrameKind::Idle
         && states[0].invul == Attribute::N)
         &&
         (states[1].kind == FrameKind::Idle
         && states[1].invul == Attribute::N)
       )
    {
        is_old = true;
    }
    else {
        // if something is happening, push the info. But not before clearing out
        if (is_old && resetting) {
            queue.clear();
        }

        while (queue.size() >= HISTORY_DEPTH) {
            queue.pop_front();
        }
        
        queue.push_back(states);
        is_old = false;
    }
    // after doing updates, store this information for later.
    backupChars();
}

void FrameHistory::backupChars()
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
    std::vector<scrState*> states_p2;
    if (p1_charIndex == p2_charIndex) {
        states_p2 = parse_scr(bbcf_base_adress, 1);
    } else {
        states_p2 = parse_scr(bbcf_base_adress, 2);
    }

    for (size_t i1 = 0; i1 < states_p1.size(); i1++) {
        p1_StateMap[states_p1[i1]->name] = states_p1[i1];
    }

    for (size_t i2 = 0; i2 < states_p2.size(); i2++) {
        p2_StateMap[states_p2[i2]->name] = states_p2[i2];
    }
}

void FrameHistory::clear() { queue.clear(); }

FrameHistory::FrameHistory() {
    queue = std::deque<StatePair>();
    p1_old_data = BackedUpCharData();
    p2_old_data = BackedUpCharData();
}

FrameHistory::~FrameHistory() {}
