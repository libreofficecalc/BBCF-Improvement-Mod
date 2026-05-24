typedef unsigned char undefined;
typedef unsigned char undefined1;
typedef unsigned short undefined2;
typedef unsigned int undefined4;
typedef undefined* pointer;

typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;
typedef long long longlong;
typedef unsigned int dword;

#pragma pack ( push, 1 )


struct AA_CNetworker {
    undefined** vftable;
    undefined4 __0004[3];
    undefined4 __0010; //chat struct start
    undefined4 __0014[5];
    char username[0x40];
    char message[0x40];
    undefined __00a8[0x1d][0x94];
    undefined __116c[0x2c];
};
static_assert(sizeof(AA_CNetworker) == 4504);

struct GGPOSessionCallbacks {
    undefined* begin_game;
    undefined* save_game_state; //Type 'F_save_game_state *' was deleted
    undefined* load_game_state;
    undefined* maybe_log_game_state;
    undefined* free_buffer;
    undefined* advance_frame;
    undefined* on_event;
};
static_assert(sizeof(GGPOSessionCallbacks) == 28);

struct Sync__SavedFrame {
    byte* buf;
    int cbuf;
    int frame;
    int checksum;
};
static_assert(sizeof(Sync__SavedFrame) == 16);

struct Sync__SavedState {
    struct Sync__SavedFrame frames[0xa];
    int head;
};
static_assert(sizeof(Sync__SavedState) == 164);

struct Sync__Config {
    struct GGPOSessionCallbacks callbacks;
    int maybe_num_prediction_frames;
    int maybe_num_players;
    int maybe_input_size;
};
static_assert(sizeof(Sync__Config) == 40);

/*prob need to look at Sync::SavedStated again to fix the offset hardcoded*/
struct Sync_ {
    void** vftable;
    struct GGPOSessionCallbacks _callbacks;
    struct Sync__SavedState _savedstate;
    struct Sync__Config _config;
    int _rollingback;
    int _last_confirmed_frame;
    int _framecount;
    int _max_prediction_frames;
    struct InputQueue_* _input_queues;
    undefined __0100[0x40c];
    struct UdpMsg__connect_status* _local_connect_status;
};
static_assert(sizeof(Sync_) == 1296);

/*PlaceHolder Class Structure*/
struct AA_LooseList {
    undefined** vftable;
    struct AA_ListNode* head;
    struct AA_ListNode* tail;
};
static_assert(sizeof(AA_LooseList) == 12);

struct AA_TaskList {
    undefined* vftable;
    struct AA_LooseList list;
};
static_assert(sizeof(AA_TaskList) == 16);

struct AAWin_CApplication {
    undefined** vftable;
    struct AA_TaskList tasks;
    int frame_count;
    pointer base_adress; //it's base adress in memory
    struct HWND___placeholder* hWnd; //windows handle to bbcf window
    wchar_t title_[0x100]; //"BLAZBLUE CENTRALFICTION"
    undefined __0220[0x144c];
};
static_assert(sizeof(AAWin_CApplication) == 5740);

struct ReplayRoundInfo {
    int frame_count;
    int winner;
    int valid_;
    undefined __000c[0xc];
    int p_overdrive[2];
    undefined __0020[0x80];
};
static_assert(sizeof(ReplayRoundInfo) == 160);

struct AA_TaskNode {
    undefined** vftable;
    struct AA_ListNode* next;
    struct AA_ListNode* prev;
    int count_;
    int priority;
    int state; //0: normal, 2: remove
};
static_assert(sizeof(AA_TaskNode) == 24);

/*PlaceHolder Class Structure*/
struct AASTEAM_SystemManager {
    undefined** vftable;
    struct AAWin_CApplication* app;
    int __0008;
    struct AA_TaskNode* system_input_controller_0; //maybe GAME_KeyControler*
    struct AA_TaskNode* input_pads_0[2];
    struct AA_TaskNode* system_input_controller;
    struct AA_TaskNode* input_pads[2];
    struct AA_TaskNode* input_controllers[2];
    struct AAWin_CFileReader_Thread* file_reader_thread;
    int __0030;
    int system_CInput_index_;
    undefined4 __0038;
    undefined __003c[0x9c8];
};
static_assert(sizeof(AASTEAM_SystemManager) == 2564);

/*PlaceHolder Class Structure*/
struct GAME_SynthKeyControler {
    undefined** vftable;
    int index;
};
static_assert(sizeof(GAME_SynthKeyControler) == 8);

struct SnapshotManager__struct {
    int _framecount;
    undefined* _ptr_buf_saved_frame;
    int __0008;
    int __000c;
    undefined* __0010;
    undefined* _ptr_buf_save_frame_1;
    undefined* _ptr_buf_save_frame_1_plus_some_offset;
    undefined* _ptr_BB_CEventInstance_0;
    undefined __0020[0xc];
    undefined* _ptr_BB_CEventInstance_1;
    undefined* _ptr_BB_CEventInstance_1_plus_some_offset;
    undefined __0034[0xc];
    undefined* __0040;
    int __0044;
};
static_assert(sizeof(SnapshotManager__struct) == 72);

/*IMPORTANT!: There's always a 8 byte offset on the names since it was copied from ReplayList, and ReplayList has an initial 8 bytes padding!!!!*/
struct ReplayFileHeader {
    undefined __0000[4];
    undefined __0004[4];
    uint valid; //- always 1. empty items in replay list have this set to 0
    undefined __000c[4];
    ulonglong date1_time64;
    uint date1_int[6]; // year, month, day, hour, minute, second
    char date1[0x20];
    longlong date2_time64;
    uint date2_int[6]; // year, month, day, hour, minute, second
    char date2[0x20];
    uint winner_maybe;
    ulonglong p1_steamID64;
    wchar_t p1_name[0x12];
    char pad_0xc8[0x9e];
    ulonglong p2_steamID64;
    wchar_t p2_name[0x12];
    char pad_0x192[0x9e];
    uint p1_toon;
    uint p2_toon;
    ulonglong recorder_steamID64;
    dword recorder_name[0x12];
    undefined __0280[0x7c];
    undefined4 __02fc;
    undefined4 replay_version_;
    undefined4 catalog_version_;
    uint favourite;
    uint p1_lvl;
    uint p2_lvl;
    undefined4 __0314;
    undefined __0318[0x74];
    undefined4 __038c;
};
static_assert(sizeof(ReplayFileHeader) == 912);

struct ReplayFileSettings {
    undefined4 rounds_to_win;
    undefined4 round_time;
    undefined4 stage;
    undefined4 music;
    undefined4 p1_flags_;
    undefined4 __0014;
    undefined4 p1_char_index;
    undefined4 __001c;
    undefined4 __0020;
    undefined4 p1_palette;
    undefined4 __0028;
    undefined4 __002c;
    undefined4 __0030;
    undefined4 __0034;
    undefined4 __0038;
    undefined4 __003c;
    undefined4 __0040;
    undefined4 __0044;
    undefined4 __0048;
    undefined4 __004c;
    undefined4 __0050;
    undefined4 __0054;
    undefined4 __0058;
    undefined4 __005c;
    undefined4 __0060;
    undefined4 __0064;
    undefined4 __0068;
    undefined4 __006c;
    undefined4 __0070;
    undefined4 __0074;
    undefined4 __0078;
    undefined4 p2_flags_;
    undefined4 __0080;
    undefined4 p2_char_index;
    undefined4 __0088;
    undefined4 __008c;
    undefined4 p2_palette;
    undefined4 __0094;
    undefined4 __0098;
    undefined4 __009c;
    undefined4 __00a0;
    undefined4 __00a4;
    undefined4 __00a8;
    undefined4 __00ac;
    undefined4 __00b0;
    undefined4 __00b4;
    undefined4 __00b8;
    undefined4 __00bc;
    undefined4 __00c0;
    undefined4 __00c4;
    undefined4 __00c8;
    undefined4 __00cc;
    undefined4 __00d0;
    undefined4 __00d4;
    undefined4 __00d8;
    undefined4 __00dc;
    undefined4 __00e0;
    undefined4 __00e4;
    undefined4 __00e8;
    undefined4 __00ec;
};
static_assert(sizeof(ReplayFileSettings) == 240);

struct ReplayFile {
    undefined __0000[4];
    undefined __0004[4];
    struct ReplayFileHeader head;
    struct ReplayFileSettings settings;
    struct ReplayRoundInfo round_info[6];
    undefined __0848[0x84];
    undefined inputs[0xf734];
};
static_assert(sizeof(ReplayFile) == 65536);

struct CBattleReplayDataManager {
    undefined** vftable;
    undefined** vftable_1;
    undefined replay[8]; //start of an unpacked replay. size 0x54ed0
    struct ReplayFileHeader replay_header;
    struct ReplayFileSettings replay_settings;
    struct ReplayRoundInfo replay_round_info[6];
    undefined __0850[0x84];
    ushort replay_inputs[0xc][0x3840];
    undefined __54ed4[4];
    struct ReplayFile replay_buffer;
    int playback_frame;
    int playback_round;
    int playback_speed; //0: normal, 1: stopped, 2: fast forward, 3: step
    int playback_hide_controls;
    int __64ee8;
    int __64eec;
    int __64ef0;
    int __64ef4;
    struct ReplayFileHeader finished_replay_header;
};
static_assert(sizeof(CBattleReplayDataManager) == 414344);

enum FPACIndex {
    static_obj_pac = 1,
    Font_pac = 2,
    shared_setting_pac = 3,
    se00_pac = 4,
    rannyu_pac = 6,
    p1_scr_pac = 0x45,
    p2_scr_pac = 0x51,
    e0_scr_pac_ = 0x60,
};

struct GAME_LoadTarget {
    undefined** vftable;
    struct AA_ListNode* __0004;
    struct AA_ListNode* __0008;
    char filename[0x100];
    undefined4 __010c;
    enum FPACIndex fpac_index;
    int file_size;
    undefined4 __0118;
    undefined4 __011c;
};
static_assert(sizeof(GAME_LoadTarget) == 288);

struct probable_struct {
    uint __0000;
    int __0004;
    int __0008;
    undefined __000c[4];
    undefined4 __0010;
    undefined __0014[0xb0];
    int __00c4;
    int __00c8;
    int _framecount_or_last_confirmed_frame;
};
static_assert(sizeof(probable_struct) == 208);

struct BackendStaticStruct {
    pointer _steamp2pbackend_ptr;
    undefined4* _ggpo_performance_monitor_ptr;
    struct BackendStaticStruct__0x8* _backend_static_struct__0x8_ptr;
    undefined4 __000c;
    undefined __0010[0x18];
    struct probable_struct _probable_struct;
    undefined __00f8[0x1c];
    undefined2 __0114;
    undefined __0116;
    undefined __0117;
    undefined4 __0118;
    undefined __011c[4];
    undefined4 __0120;
    undefined __0124;
    undefined __0125;
    undefined __0126;
    undefined __0127;
};
static_assert(sizeof(BackendStaticStruct) == 296);

/* /winsock.h/sockaddr_in */
struct sockaddr_in_placeholder { undefined __0000[0x10]; };

struct SteamUdpProtocol__QueueEntry {
    int queue_time;
    struct sockaddr_in_placeholder dest_addr;
    struct UdpMsg* msg;
};
static_assert(sizeof(SteamUdpProtocol__QueueEntry) == 24);

struct SCENE_CBase {
    struct AA_TaskNode base;
    struct AA_TaskList tasks;
    undefined __0028[4];
    uint GameSceneState;
    undefined __0030[0x14];
    int __0044;
    undefined __0048[0x18];
};
static_assert(sizeof(SCENE_CBase) == 96);

struct InputBufferButton {
    char down;
    char hit;
    char release;
    char hit_5f;
    char up;
    char pad_05[4];
};
static_assert(sizeof(InputBufferButton) == 9);

struct InputBufferDirection {
    char down;
    char similar_down;
    char pad_02[6];
    char up;
    char similar_up;
    char pad_0A[3];
};
static_assert(sizeof(InputBufferDirection) == 13);

/*CharData*/
struct OBJ_CCharBase {
    void** vftable;
    int frame_count_minus_1;
    int hitstop;
    char pad_000c[4];
    int unknownStatus1;
    char pad_0014[4];
    int stateChangedCount;
    undefined4 pad_001C;
    undefined4 __0020;
    undefined4 __0024;
    undefined4 __0028;
    undefined4 __002c;
    undefined4 player_index_;
    int charIndex;
    char pad_0038[0x14];
    char* pJonbEntryBegin;
    char pad_0050[0x44];
    uint hurtboxCount;
    uint hitboxCount;
    char pad_009C[0x3c];
    char* current_sprite_img;
    char pad_00DC[0x78];
    int unknown_status2;
    char pad_0158[8];
    int actionTime;
    int actionTime2;
    int actionTimeNoHitstop;
    char pad_0174[8];
    int SLOT_31;
    int SLOT_51;
    int SLOT_52;
    int SLOT_53;
    int SLOT_54;
    int SLOT_55;
    int SLOT_56;
    int SLOT_57;
    int SLOT_58;
    int SLOT_59;
    int SLOT_60;
    int SLOT_61;
    int SLOT_62;
    char pad_01B0[0x10];
    int overdriveTimeleft;
    int overdriveTimerStartedAt;
    char pad_01C8[0x14];
    int moveSuperflashTime;
    char pad_01D8[4];
    int superflashTime;
    int isDoingDistortion;
    char pad_01E4[4];
    struct OBJ_CBase* ownerEntity;
    char pad_01EC[4];
    char* enemyChar;
    char pad_01F4[0x36];
    undefined __022a;
    undefined __022b;
    char* last_child_entity_spawned;
    char* extra_child_entities[7];
    char* main_child_entity;
    char pad_0250[0xc];
    int bitflags_for_curr_state_properties_or_smth;
    char pad_0260[4];
    int facingLeft;
    int position_x;
    int position_y;
    char pad_0270[4];
    int offsetX_1;
    char pad_0278[4];
    int rotationDegrees;
    char pad_0280[4];
    int scaleX;
    int scaleY;
    char pad_028C[0x50];
    int position_x_dupe;
    int position_y_dupe;
    char pad_02E4[0x10];
    int offsetX_2;
    char pad_02F8[4];
    int offsetY_2;
    char pad_0300[0x1c];
    int BoundingX;
    int BoundingY;
    int BoundingFixX;
    int BoundingFixY;
    int BoundingAddY;
    int BoundingAddX;
    int stageEdgeTouchTimer;
    char pad_0338[0x150];
    int typeOfAttack;
    int attackLevel;
    int moveDamage;
    char pad_0494[0x2c];
    short moveSpecialBlockstun;
    char pad_04C2[6];
    short moveGuardCrushTime;
    char pad_04CA[0xa];
    int vectorcheckX_1;
    int vectorcheckY_1;
    int vectorcheckX_2;
    int vectorcheckY_2;
    int ThrowRange;
    char pad_04E8[0x1c];
    char performedThrowName[0x20];
    char pad_0524[0x140];
    int moveAirPushbackX;
    int moveAirPushbackY;
    char pad_066C[0x14];
    int moveHitstunOverwrite;
    int moveUntechOverwrite;
    char pad_0688[0xc];
    int movePushbackX;
    char pad_0698[0xc];
    int moveP1Overwrite;
    int moveP2Overwrite;
    char pad_06AC[0x28];
    int moveCounterHitAirPushbackY;
    char pad_06D8[0x2c0];
    int invuln_bitfield;
    int guard_point_bitfield;
    char pad_09A0[0x30];
    int previousHP;
    int currentHP;
    int maxHP;
    char pad_09DC[0x93c];
    undefined* scr_hash_table;
    undefined* __131c;
    char* nextScriptLineLocationInMemory;
    char* currentScriptStateEntryInMemory;
    int frameCounterCurrentSprite;
    int frameLengthCurrentSprite;
    int frameLengthCurrentSprite2;
    int linesReadInCurrentState;
    longlong currentSprite;
    char pad_1340[0xcb0];
    undefined* active_cmd;
    char lastAction[0x14];
    char pad_2008[0xc];
    char current_action2[0x14];
    char pad_2028[0x18];
    char set_action_override[0x14];
    char pad_2054[0x1c];
    char currentAction[0x14];
    char pad_2084[0x1c4];
    char char_abbr[4];
    char pad_224C[0x14];
    int input_direction_;
    undefined __2264[0x10];
    int blockstun;
    char pad_2278[0x2ef8];
    int hitstun;
    char pad_5174[0x8c];
    uint hardLandingRecovery;
    char pad_5204[0x10];
    int defaultProration[6];
    char pad_5228[0x544];
    int hitCount;
    int hitCount2;
    int timeAfterTechIsPerformed;
    int timeAfterLatestHit;
    int comboDamage;
    int comboDamage2;
    int lastcomboDamage;
    int comboProration;
    int starterRating;
    int comboTime;
    int singleHitDamage;
    char pad_579C[4];
    int realTimeComboTime;
    int heatGeneratedForCombo;
    char pad_57A4[0x38];
    char hitByWhichAction[0x20];
    char pad_5800[0x1c];
    char sameMoveProrationStack[0x20];
    char pad_583C[0x298];
    int heatMeter;
    char pad_5AD8[4];
    int heatGainCooldown;
    char pad_5AE0[4];
    int overdriveMeter;
    char pad_5AE8[0x10];
    int overdriveMaxtime;
    char pad_5AFC[8];
    int barrier;
    char pad_5B08[0x287c];
    int SLOT_unknown1;
    char pad_8388[0x163b0];
    int _input_counters_[0x19];
    char L_input_buffer_flag_base;
    struct InputBufferButton L_input_buttons[6];
    struct InputBufferDirection L_input_directions[9];
    char buffer_L_236;
    char buffer_L_623;
    char buffer_L_214;
    char buffer_L_41236;
    char buffer_L_421;
    char buffer_L_63214;
    char buffer_L_236236;
    char buffer_L_214214;
    char padding_1E850;
    char buffer_L_6321463214;
    char buffer_L_632146_2;
    char padding_1E853;
    char buffer_L_2141236;
    char buffer_L_2363214;
    char buffer_L_22;
    char buffer_L_46;
    char padding_1e858;
    char buffer_L_2H6;
    char buffer_L_6428;
    char buffer_L_4H128;
    char buffer_L_64641236;
    char buffer_L_412364123641236;
    char buffer_L_KS28_;
    char buffer_L_646;
    char padding_1E860[2];
    char buffer_L_CMASH;
    char padding_1E863[3];
    char buffer_L_360;
    char buffer_L_720;
    char padding_1E869;
    char buffer_L_222;
    char padding_1E86B;
    char buffer_L_236236LambdaSuper;
    char padding_1E86D[9];
    char buffer_L_8izanamifloat;
    char buffer_L_66;
    char buffer_L_44;
    char padding_1E878[3];
    char buffer_L_22jinsekkajin;
    char padding_1E87B;
    char buffer_L_44nineseconddash;
    char buffer_L_66nineseconddash;
    char padding_1E87F;
    char buffer_L_88nineseconddash;
    char padding_1E877[0x11];
    char buffer_L_1080;
    char buffer_L_1632143;
    char padding_1E895;
    char buffer_L_632146;
    char padding_1E897[3];
    char buffer_L_takemikazuchi_shit;
    char buffer_L_4H6;
    char buffer_L_2H8;
    char padding_1E89D;
    char buffer_L_4H1236;
    char buffer_L_82;
    char buffer_L_8H2;
    char padding_1E8A0[4];
    char buffer_L_236_5f;
    char buffer_L_214_5f;
    char R_input_buffer_flag_base;
    struct InputBufferButton R_input_buttons[6];
    struct InputBufferDirection R_input_directions[9];
    char buffer_R_236;
    char buffer_R_623;
    char buffer_R_214;
    char buffer_R_41236;
    char buffer_R_421;
    char buffer_R_63214;
    char buffer_R_236236;
    char buffer_R_214214;
    char padding_1E95A;
    char buffer_R_6321463214;
    char buffer_R_632146_2;
    char padding_1E95D;
    char buffer_R_2141236;
    char buffer_R_2363214;
    char buffer_R_22;
    char buffer_R_46;
    char padding_1E962;
    char buffer_R_2H6;
    char buffer_R_6428;
    char buffer_R_4H128;
    char buffer_R_64641236;
    char buffer_R_412364123641236;
    char buffer_R_KS28_;
    char buffer_R_646;
    char padding_1E96A[2];
    char buffer_R_CMASH;
    char padding_1E96D[3];
    char buffer_R_360;
    char buffer_R_720;
    char padding_1E972;
    char buffer_R_222;
    char padding_1E974;
    char buffer_R_236236LambdaSuper;
    char padding_1E976[9];
    char buffer_R_8izanamifloat;
    char buffer_R_66;
    char buffer_R_44;
    char padding_1E982[3];
    char buffer_R_22jinsekkajin;
    char padding_1E986;
    char buffer_R_44nineseconddash;
    char buffer_R_66nineseconddash;
    char padding_1E989;
    char buffer_R_88nineseconddash;
    char padding_1E98B[0x11];
    char buffer_R_1080;
    char buffer_R_1632143;
    char padding_1E99E;
    char buffer_R_632146;
    char padding_1E9A0[3];
    char buffer_R_takemikazuchi_shit;
    char buffer_R_4H6;
    char buffer_R_2H8;
    char padding_1E9A6;
    char buffer_R_4H1236;
    char buffer_R_82;
    char buffer_R_8H2;
    char padding_1E9AA[4];
    char buffer_R_236_5f;
    char buffer_R_214_5f;
    char _input_related_[0x10];
    int slot2_or_slot4;
    char pad_1E9C4[0x1748];
    int Drive1;
    char pad_20110[0xc];
    int Drive1_type;
    char pad_20120[0x10];
    int Drive2;
    char pad_20134[0x20];
    int Drive3;
    char pad_20158[0x136c];
    undefined __214c4[0x34b4];
};
static_assert(sizeof(OBJ_CCharBase) == 149880);

struct Poll {
    undefined __0000[0x510];
};
static_assert(sizeof(Poll) == 1296);

struct Udp {
    undefined* SOCKET;
    struct GGPOSessionCallbacks* _callbacks;
    struct Poll* _poll;
};
static_assert(sizeof(Udp) == 12);

struct SteamUdpProtocol___oo_packet {
    int send_time;
    struct sockaddr_in_placeholder dest_addr;
    struct UdpMsg* msg;
};
static_assert(sizeof(SteamUdpProtocol___oo_packet) == 24);

/* /0Mine/Class/RingBuffer<QueueEntry, 256> */
struct RingBuffer_ltQueueEntry__256_gt_placeholder { undefined __0000[0x180c]; };

struct SteamUdpProtocol__Union_state_sync {
    uint roundtrips_remaining;
    uint random;
};
static_assert(sizeof(SteamUdpProtocol__Union_state_sync) == 8);

struct SteamUdpProtocol__Union_state_running {
    uint last_quality_report_time;
    uint last_network_stats_interval;
    uint last_input_packet_recv_time;
};
static_assert(sizeof(SteamUdpProtocol__Union_state_running) == 12);

union SteamUdpProtocol___state {
    struct SteamUdpProtocol__Union_state_sync sync;
    struct SteamUdpProtocol__Union_state_running running;
};
static_assert(sizeof(SteamUdpProtocol___state) == 12);

/*the bits estimate needs to be checked*/
struct GameInput_ {
    int frame;
    int size;
    char bits[0x12];
};
static_assert(sizeof(GameInput_) == 26);

/*must double check GameInput! size, might be 2 bytes short*/
struct TimeSync_ {
    undefined* __0000;
    int _local[0x28];
    int _remote[0x28];
    struct GameInput_ _last_inputs[0xa];
    undefined __0248[0x14];
    int _next_prediction;
};
static_assert(sizeof(TimeSync_) == 608);

struct SteamUdpProtocol {
    void** vftable;
    struct Udp* _udp;
    struct sockaddr_in_placeholder _peer_addr;
    ushort _magic_number;
    int _queue;
    ushort _remote_magic_number;
    int _connected;
    int _send_latency;
    int _oop_percent;
    struct SteamUdpProtocol___oo_packet _oo_packet;
    struct RingBuffer_ltQueueEntry__256_gt_placeholder _send_queue;
    undefined* __1850;
    int _packets_sent;
    int _bytes_sent;
    int _kbps_sent;
    int _stats_start_time;
    undefined __1864[0x18];
    SteamUdpProtocol___state _state;
    undefined __1888[0x1c14];
    struct GameInput_ _last_received_input;
    undefined __34b6;
    undefined __34b7;
    struct GameInput_ _last_sent_input;
    undefined __34d2;
    undefined __34d3;
    struct GameInput_ _last_acked_input;
    undefined __34ee;
    undefined __34ef;
    uint _last_send_time;
    uint _last_recv_time;
    uint _shutdown_timeout;
    uint _diconnect_event_sent;
    uint _disconnect_timeout;
    uint _disconnect_notify_start;
    bool _disconnect_notify_sent;
    undefined __3509;
    undefined __350a;
    undefined __350b;
    ushort _next_send_seq;
    ushort _next_recv_seq;
    struct TimeSync_ _timesync;
    undefined __3770[0x2010];
};
static_assert(sizeof(SteamUdpProtocol) == 22400);

struct SteamSpectatorBackend_ {
    undefined* __0000;
    undefined* __0004;
    undefined* __0008;
    struct GGPOSessionCallbacks _callbacks;
    struct Poll _poll;
    undefined* __0538;
    struct Udp _udp;
    struct SteamUdpProtocol _host;
    int _synchronizing;
    int _input_size;
    int _num_players;
    int _next_input_to_send;
    undefined __5cd8[4];
    int _disconnect_timeout_mine;
    int _disconnect_notify_start_mine;
    undefined __5ce4[0x1c04];
};
static_assert(sizeof(SteamSpectatorBackend_) == 30952);

/*PlaceHolder Class Structure*/
struct GAME_CETCObjParam {
};
static_assert(sizeof(GAME_CETCObjParam) == 1);

/*PlaceHolder Class Structure*/
struct GAME_CETCManager {
    undefined** vftable;
    undefined** vftable_1;
    struct GAME_CETCObjParam* etc_obj_params[0x1c2];
    struct AA_LooseList __0710[2];
    struct AA_LooseList list_of_params;
    struct AA_LooseList* __0734;
    undefined __0738[4];
    undefined __073c[2][0x1000];
    undefined4 __273c;
    undefined __2740[4];
    struct AA_LooseList* __2744;
    undefined __2748[2][0x18];
    undefined4 hide_HUD; //probably has more flags in it
    undefined4 __277c;
    undefined4 __2780;
    undefined4 __2784;
    undefined4 __2788;
    undefined4 __278c;
    undefined4 __2790;
    undefined4 __2794;
    undefined4 __2798;
    undefined4 __279c;
    undefined4 __27a0;
    undefined4 __27a4;
    undefined4 __27a8;
    undefined4 __27ac;
    undefined4 __27b0;
    undefined4 __27b4;
    undefined4 __27b8;
    char __27bc[0x20];
    char __27dc[0x20];
    char __27fc[0x20];
    char __281c[0x20];
    char __283c[0x20];
    char __285c[0x20];
    char __287c[0x20];
    char __289c[0x20];
    char __28bc[0x20];
    char __28dc[0x20];
    char __28fc[0x20];
    char __291c[0x20];
    int* params_buffer_info;
    undefined* _Cockpit_CBase;
    undefined* _TitleLoop_CBase;
    undefined* _CharSelect_CBase;
    undefined* _Profile_CBase;
    undefined* _VSInfo_CBase;
    undefined* _Winner_CBase;
    undefined* _StageInfo_CBase;
    undefined* _Ending_CBase;
    undefined* _Ranking_CBase;
    undefined* _ModeSelect_CBase;
    undefined* _StaffRoll_CBase;
    undefined* _Result_CBase;
    undefined* _MainMenu_CBase;
    undefined* _StorySelect_CBase;
    undefined* _Lobby_CBase;
    undefined* _MyRoomTest_CBase;
    undefined* _DCC_CBase;
    struct GAME_CETCObjectBase* _ETCObjectStatic_CBase;
    struct CharSelect* char_select;
    undefined* title_loop;
    undefined* __2990;
    undefined* __2994;
    undefined* __2998;
    undefined* __299c;
    undefined* __29a0;
    undefined* __29a4;
    undefined* __29a8;
    undefined* __29ac;
    undefined* __29b0;
    struct MainMenu* main_menu;
    undefined* __29b8;
    undefined* __29bc;
    undefined* __29c0;
    undefined* net_;
};
static_assert(sizeof(GAME_CETCManager) == 10696);

struct AA_ListNode {
    undefined** vftable;
    struct AA_ListNode* next;
    struct AA_ListNode* prev;
};
static_assert(sizeof(AA_ListNode) == 12);

struct GGPOPerformanceMonitor {
    undefined __0000[0x6e8];
};
static_assert(sizeof(GGPOPerformanceMonitor) == 1768);

struct SaveUtilParams {
    int thread_id_;
    int file_format;
    int replay_index;
    int __000c;
    longlong time_64;
    void* buffer;
    ulong buffer_size;
};
static_assert(sizeof(SaveUtilParams) == 32);

struct SaveUtil {
    undefined** vftable;
    undefined4 __0004;
    undefined4 __0008;
    int done;
    undefined4 __0010;
    void* thread_handle;
    int action;
    undefined __001c[4];
    struct SaveUtilParams params;
    char* pFilePath; //should be one of the static fpaths, save_content, replay_content, replay_list
    char* pContentType; //one of "save_content", "replay_list_content", "replay_content"
    char fname[0x100];
    undefined __0148[4];
    int status_;
};
static_assert(sizeof(SaveUtil) == 336);

/*PlaceHolder Class Structure*/
struct GAME_KeyControler {
    undefined** vftable;
    undefined __0004[0x1c];
    struct GAME_CKeyRingBuffer* current_key_ring_buffer; //keeps changing
    undefined __0024[4];
    int key_hit_bits;
    undefined __002c[4];
    int key_down_bits;
    undefined __0034[8];
    undefined4 __003c;
    undefined __0040[8];
    undefined1 disabled_; //Created by retype action
    undefined __0049[0xb];
    undefined4 __0054;
};
static_assert(sizeof(GAME_KeyControler) == 88);

struct SCENE_CVSInfo {
    struct SCENE_CBase base;
    undefined4 __0060;
    undefined4 __0064;
    undefined4 __0068;
    undefined4 __006c;
    undefined4 __0070;
    undefined4 __0074;
    undefined4 __0078;
    undefined4 __007c;
    undefined4 __0080;
    undefined4 __0084;
    undefined4 __0088;
    undefined4 __008c;
    undefined4 __0090;
    undefined4 __0094;
    undefined4 __0098;
    undefined4 __009c;
    undefined4 __00a0;
    undefined4 __00a4;
    undefined4 __00a8;
    undefined4 __00ac;
    undefined4 __00b0;
    undefined4 __00b4;
    undefined4 __00b8;
    undefined4 __00bc;
    undefined4 __00c0;
    undefined4 __00c4;
    undefined4 __00c8;
    undefined4 __00cc;
};
static_assert(sizeof(SCENE_CVSInfo) == 208);

struct BBSave {
    undefined __0000[8];
    int __0008;
    int size;
    int __0010;
    int __0014[0x41]; //some ranges referenced in _init_bbsave
    int __0118[0x2b];
    int __01c4[0x100];
    int __05c4[0xf2f];
    undefined __4280[0x100];
    undefined __4380[0x61d8]; //copied to DAT_00667ff0
    undefined __a558[4];
    int training_menu_values[3]; //'Player Character'
    int menu_item_values[0xd];
    int __a59c[0x18]; //'Hit Ponts'
    int display_virtual_stick;
    int virtual_stick_type;
    int display_input_history;
    int __a608[0x5a]; //rest of training menu
    int __a770[0x512];
    int training_menu_values_extra[4];
    undefined4 __bbc8[2];
    undefined4 game_money;
    undefined4 __bbd4[0x13339];
};
static_assert(sizeof(BBSave) == 362680);

struct ReplayList {
    char pad_0x0[8];
    struct ReplayFileHeader replays[0x64];
    uint order[0x64];
    uint count;
    undefined __165dc[4];
};
static_assert(sizeof(ReplayList) == 91616);

struct CSaveDataManager {
    undefined** vftable;
    undefined __0004[4];
    struct BBSave bbsave;
    undefined bbsave_buffer[0x588b8];
    undefined bbstory_buffer[0x100078];
    int next_SaveUtil_action_running;
    int next_SaveUtil_action; //1: read, 0,2: write, 3: delete, 7: check
    int next_SaveUtil_file_format; //0: bbsave.dat    1: replay%02d.dat    2: replay_list.dat    3: bbstory%02d.dat
    int save_replay_index;
    int save_status_; //gets set to SaveUtil._0x14c
    undefined4 __1b1204;
    undefined4 __1b1208;
    undefined4 __1b120c;
    undefined4 __1b1210;
    undefined4 __1b1214;
    undefined4 __1b1218;
    undefined4 __1b121c;
    undefined4 __1b1220;
    undefined __1b1224[8];
    undefined4 __1b122c;
    struct ReplayList replay_list;
    undefined __1c7810[0x30];
};
static_assert(sizeof(CSaveDataManager) == 1865792);

struct InputQueue_ {
    int _id;
    uint _head;
    int _tail;
    int _length;
    int _first_frame;
    int _last_user_added_frame;
    int _last_added_frame;
    int _first_incorrect_frame;
    int _last_frame_requested;
    int _frame_delay;
    struct GameInput_ _inputs[0x8a];
    struct GameInput_ _prediction;
    undefined __0e46;
    undefined __0e47;
};
static_assert(sizeof(InputQueue_) == 3656);

struct GAMESTEAM_SystemKeyController {
    undefined* vftable;
    undefined __0004[0x54];
};
static_assert(sizeof(GAMESTEAM_SystemKeyController) == 88);

struct SCENE_CBattle {
    struct SCENE_CBase base;
    int __0060;
    undefined __0064[8];
    int __006c;
    undefined4 __0070;
};
static_assert(sizeof(SCENE_CBattle) == 116);

struct _20bdc_sized_plaaceholder {
    undefined __0000[0x20bdc];
};
static_assert(sizeof(_20bdc_sized_plaaceholder) == 134108);

enum MatchState_ {
    MatchState_NotStarted = 0,
    MatchState_RebelActionRoundSign = 2,
    MatchState_Fight = 3,
    MatchState_FinishSign = 4,
    MatchState_WinLoseSign = 5,
    MatchState_VictoryScreen = 7,
    MatchState_Initialization = 8,
};

struct MatchInfo {
    int flags; //8: match finished?
    int rounds_to_win;
    int round;
    int wins_p1;
    int wins_p2;
    int time_total;
    int time_in_frames;
    int time;
    undefined __0020[0x10];
    enum MatchState_ match_state;
    undefined4 __0034;
    int winner;
    undefined __003c[0x2c];
    undefined timer[0x1a8]; //of type BATTLE_CSpeedStarModeTimeCtrl
};
static_assert(sizeof(MatchInfo) == 528);

struct BATTLE_CObjectManager {
    undefined** vftable;
    undefined** vftable_1;
    undefined** vftable_2;
    int events[8];
    int events_count;
    int controls_enabled_; //1,1,1? turned off for intro/outro, by event 6?
    undefined4 __0034[2];
    undefined4 __003c[3]; //0,1,2?
    undefined* src_hash_tables[6][6];
    undefined* __00d8[0x28];
    int __0178[0x28];
    undefined __0218[0xa0];
    struct _20bdc_sized_plaaceholder _20bdc_sized_placeholders_[3];
    undefined __6264c[2][0x94];
    int entity_count;
    undefined __62778[0xc];
    struct OBJ_CCharBase** entity_list; //3 different arrays with the same values of [p1, p2] + obj_entities
    struct OBJ_CCharBase** entity_list_1; //sorted at 00158154?
    struct OBJ_CCharBase** entity_list_2;
    struct OBJ_CCharBase* obj_entities[0xfa]; //actually OBJ_CBase*, each of size 0x2248
    struct OBJ_CCharBase* player1_entity;
    struct MatchInfo match_info;
    undefined current_inputs[2][0x1c]; //first two bytes are the input for each player
    undefined __62dc4[0x30];
    undefined __62df4[0xfa0];
    int __63d94;
    int __63d98;
    int __63d9c;
    undefined __63da0[8];
    undefined __63da8[0x10020];
    undefined __73dc8[0x1007c];
    undefined* __83e44;
    undefined __83e48[0x14];
    int* entity_buffer_info; //holds [buffer_ptr, size, algnment=0x10?, offset?, offset?, index?]
};
static_assert(sizeof(BATTLE_CObjectManager) == 540256);

struct UdpMsg {
    undefined __0000[0x1020];
};
static_assert(sizeof(UdpMsg) == 4128);

/*PlaceHolder Class Structure*/
struct BATTLE_CLoadTask {
    struct AA_TaskNode base;
    struct GAME_LoadTarget tasks[0x75];
    struct AA_LooseList list_1;
    struct AA_LooseList list_2;
    undefined4 can_start_reading_;
    int state;
    undefined4 state1;
};
static_assert(sizeof(BATTLE_CLoadTask) == 33756);

/* /winnt.h/_RTL_CRITICAL_SECTION */
struct _RTL_CRITICAL_SECTION_placeholder { undefined __0000[0x18]; };

struct AAWin_CFileReader_Thread {
    undefined** vftable;
    undefined __0004[8];
    void* thread_handle;
    struct _RTL_CRITICAL_SECTION_placeholder crit;
    undefined4* buffer_of_indices_;
    undefined4 index_1_;
    undefined4 queue_index_;
    void* events[2];
    undefined __003c[0x18];
    undefined1 index_2_; //Created by retype action
    undefined __0055;
    undefined __0056;
    undefined __0057;
};
static_assert(sizeof(AAWin_CFileReader_Thread) == 88);

struct UdpMsg__connect_status {
    int disconnected:1;
    int last_frame:31;
};
static_assert(sizeof(UdpMsg__connect_status) == 4);

/*PlaceHolder Class Structure*/
struct SteamPeer2PeerBackend {
    void** vftable;
    undefined* __0004;
    undefined* __0008;
    struct GGPOSessionCallbacks _callbacks;
    undefined* __0028;
    struct Poll _poll;
    struct Sync_ _sync;
    struct Udp _udp;
    struct SteamUdpProtocol* _endpoints;
    int idk_what;
    struct SteamUdpProtocol _spectators[6];
    int _num_spectators;
    int _input_size;
    int _synchronizing;
    int _num_players;
    int _next_recommended_sleep;
    int _next_spectator_frame;
    int _disconnect_timeout;
    int _disconnect_notify_start;
    struct UdpMsg__connect_status _local_connect_status[2];
    undefined __21788[8];
};
static_assert(sizeof(SteamPeer2PeerBackend) == 137104);

struct AADX_CDynamicTextureList_ {
    undefined* vtable;
    undefined** ptr_to_list_ptrs_AA_CTexture_HIP;
    undefined __0008[4];
    uint __000c;
    undefined __0010[0x2c];
    int* __003c;
    uint __0040;
    undefined __0044[4];
    int __0048;
    int __004c;
    undefined __0050[4];
    uint __0054;
    int __0058;
};
static_assert(sizeof(AADX_CDynamicTextureList_) == 92);

struct SteamPeer2PeerBackend__VFTABLE_ {
    pointer destructor;
    pointer DoPoll;
    pointer Add_Remote_mine;
    pointer AddLocalInput;
};
static_assert(sizeof(SteamPeer2PeerBackend__VFTABLE_) == 16);

struct AASTEAM_CNetworker {
    struct AA_CNetworker base;
    undefined __1198[0x80];
    undefined __1218[0x20];
    undefined __1238[0x1a00];
    undefined __2c38[0xa08];
    undefined __3640[2][0x40][8];
    void* invite_callback; //CCallbackImpl<16> *
    ulonglong lobby_id;
    undefined __3a4c[4];
};
static_assert(sizeof(AASTEAM_CNetworker) == 14928);

struct SnapshotManager {
    struct SnapshotManager__struct _saved_states_related_struct[0xa];
    int _counter_of_some_sort;
    undefined* __02d4;
    undefined* _base_of_snapshot_mem;
    undefined __02dc[4];
};
static_assert(sizeof(SnapshotManager) == 736);

struct GAME_CNetworkTask {
    struct AA_TaskNode base;
};
static_assert(sizeof(GAME_CNetworkTask) == 24);

enum GGPO_ERRORCODE {
    GGPO_ERRORCODE_GENERAL_FAILURE = -1,
    GGPO_OK = 0,
    GGPO_ERRORCODE_INVALID_SESSION = 1,
    GGPO_ERRORCODE_INVALID_PLAYER_HANDLE = 2,
    GGPO_ERRORCODE_PLAYER_OUT_OF_RANGE = 3,
    GGPO_ERRORCODE_PREDICTION_THRESHOLD = 4,
    GGPO_ERRORCODE_UNSUPPORTED = 5,
    GGPO_ERRORCODE_NOT_SYNCHRONIZED = 6,
    GGPO_ERRORCODE_IN_ROLLBACKK = 7,
    GGPO_ERRORCODE_INPUT_DROPPED = 8,
    GGPO_ERRORCODE_PLAYER_DISCONNECTED = 9,
    GGPO_ERRORCODE_TOO_MANY_SPECTATORS = 0xa,
    GGPO_ERRORCODE_INVALID_REQUEST = 0xb,
};

struct BackendStaticStruct__0x8 {
    undefined __0000[0x20c];
    int _value_0xa_always_it_seems;
    int _initializes_to_zero_i_thinkk;
    undefined __0214[4];
    undefined* _value_was_0x535539C7_in_an_instance_long_ago;
    undefined* _value_was_0x01100001_in_an_instance_long_ago;
};
static_assert(sizeof(BackendStaticStruct__0x8) == 544);

struct GAME_BattleSynthKeyControler {
    undefined** vftable;
    int index; //0 or 1
};
static_assert(sizeof(GAME_BattleSynthKeyControler) == 8);

struct DAT_00612718_sound1_ {
    undefined __0000[0xc];
    undefined4 __000c;
    undefined __0010[0x10];
    undefined4 __0020;
    undefined __0024[4];
    undefined4 __0028;
    undefined __002c[0x108];
    void* hHandle_0061284c;
    undefined __0138[8];
    void* hEvent_00612858;
    undefined __0144[0x2c];
    undefined* __0170;
    undefined4 __0174;
    undefined4 __0178;
    undefined4 __017c;
    undefined4 __0180;
    undefined __0184[4];
};
static_assert(sizeof(DAT_00612718_sound1_) == 392);

struct NetUserData {
    ulonglong steam_id;
    wchar_t username[0x10];
    undefined __0028[0xa2];
    undefined region_[2];
    undefined4 __00cc;
    undefined __00d0[0xc4]; //start of some struct
    byte netcolor;
    byte netcolor_win_counter;
    undefined __0196[2];
    undefined __0198[0x607c];
    wchar_t quick_message_[0x14];
    undefined __623c[0x1cad4];
    undefined room[0x508]; //Room struct in BBIM
    undefined* room_member_entry;
    undefined __2321c[0x50];
    undefined __2326c[6][0x68a4];
    undefined4 __4a644;
};
static_assert(sizeof(NetUserData) == 304712);

struct ISteamUser_vftable {
    undefined* GetHSteamUser;
    undefined* BLoggedOn;
    undefined* GetSteamID;
    undefined* InitiateGameConnection;
    undefined* TerminateGameConnection;
    undefined* TrackAppUsageEvent;
    undefined* GetUserDataFolder;
    undefined* StartVoiceRecording;
    undefined* StopVoiceRecording;
    undefined* GetAvailableVoice;
    undefined* GetVoice;
    undefined* DecompressVoice;
    undefined* GetVoiceOptimalSampleRate;
    undefined* GetAuthSessionTicket;
    undefined* BeginAuthSession;
    undefined* EndAuthSession;
    undefined* CancelAuthTicket;
    undefined* UserHasLicenseForApp;
    undefined* BIsBehindNAT;
    undefined* AdvertiseGame;
    undefined* RequestEncryptedAppTicket;
    undefined* GetEncryptedAppTicket;
    undefined* GetGameBadgeLevel;
    undefined* GetPlayerSteamLevel;
    undefined* RequestStoreAuthURL;
    undefined* BIsPhoneVerified;
    undefined* BIsTwoFactorEnabled;
    undefined* BIsPhoneIdentifying;
    undefined* BIsPhoneRequiringVerification;
};
static_assert(sizeof(ISteamUser_vftable) == 116);

struct static_DAT_of_PTR_on_load_4_ {
    byte padding_0x0[0xc];
    undefined* ptr_OBJ_with_seemingly_list_managers_1;
    undefined padding_0x10[0x10];
    undefined* ptr_OBJ_with_seemingly_list_managers_2;
    undefined padding_0x24[8];
    uint size_or_counter_for_inline_managers_1;
    struct BATTLE_CObjectManager* ptr_BATTLE_CObjectManager_plus_0x4;
    undefined* ptr_BATTLE_CScreenManager;
    undefined* ptr_GAME_CEff3DInstHndlManager;
    undefined* ptr_AA_CRandomManager;
    undefined* ptr_battle_stat__StatBattleTemp;
    undefined* ptr_BATTLE_CBGManager;
    byte padding_0x48[0x68];
    int size_or_counter_for_inline_managers_2;
    undefined* ptr_BATTLE_CObjectManager_static_;
    undefined* ptr_AA_CParticleManager;
    undefined* ptr_BATTLE_CScreenManager_different_;
    undefined* ptr_AA_CCameraManager;
    undefined* ptr_AA_CRandomManager_different_;
    undefined* ptr_battle_stat__StatBattleTmp_different_;
    undefined* ptr_BATTLE_CBGManager_different_;
    undefined* ptr_game_Stat_PCoinManager;
    undefined* ptr_AA_CModelInstanceManager;
    undefined* ptr_CBattleReplayDataManager;
    undefined* ptr_GAME_CEff3DInstHndlManager_different_;
    undefined* ptr_GAME_CEventManager;
    undefined* ptr_BG_EffectManager;
    undefined* ptr_GAME_CFadeTaskManager;
    undefined* ptr_GAME_CETCManager;
    byte padding_0xf0[0x80];
    struct SnapshotManager* ptr_snapshot_manager_mine;
    void* __0174;
    void* __0178;
    void* __017c;
    void* __0180;
    char __0184;
    undefined __0185[0x14f];
};
static_assert(sizeof(static_DAT_of_PTR_on_load_4_) == 724);

struct DAT_121e450 {
    int __0000;
    int __0004;
    char strings_[0x2134][0x40];
    undefined __84d08[0x109a0];
    undefined4 __956a8;
    undefined4 __956ac;
    undefined4 __956b0;
    undefined4 __956b4;
    undefined4 __956b8;
    undefined4 __956bc;
    undefined4 __956c0;
    undefined4 __956c4;
    undefined4 __956c8;
    undefined4 __956cc;
    undefined4 __956d0;
    undefined4 __956d4;
    undefined4 __956d8;
    undefined4 __956dc;
    undefined4 __956e0;
    byte fonts_[2][0xb4];
    undefined4 __9584c;
    undefined4 __95850;
};
static_assert(sizeof(DAT_121e450) == 612436);

/*maybe the definition it gets from the steam servers?*/
struct room_settings_intemediary {
    undefined4 __0000;
    undefined __0004[0x3c];
    int __0040;
    undefined __0044[0x10];
    int __0054;
    undefined __0058[4];
    int __005c;
    undefined __0060[0xc];
    int __006c;
    int __0070;
    undefined __0074[8];
    int rematch; //0x48=unlimited, 0x49=ft2, 0x4a=ft3, 0x4b=ft5, 0x4c= ft10, if none of the previous no rematch
};
static_assert(sizeof(room_settings_intemediary) == 128);

struct Buffer {
    undefined* data;
    int size;
};
static_assert(sizeof(Buffer) == 8);

struct SCENE_CBase_vftable {
    uint* dtor;
    uint* update_task;
    uint* func_GSS_3; //this is called when GameSceneState is 3, might be called elsewhere too
    uint* func_GSS_7; //same as others
    uint* func_GSS_8_1;
    uint* func_GSS_9_update_scene;
    uint* func_GSS_10_next_scene;
    uint* func_GSS_8_2;
    uint* func_GSS_4_inc;
    uint* func_GSS_5_inc;
    uint* func_GSS_6_inc;
    uint* func_GSS_11_empty;
    uint* func_GSS_2_inc;
};
static_assert(sizeof(SCENE_CBase_vftable) == 52);

struct Factory {
    undefined** vftable;
};
static_assert(sizeof(Factory) == 4);

/*stores Cinput objects for KeyControllers*/
struct CInputList {
    undefined** items; //maybe CInput...**
    int max_count;
    int count;
    undefined* first_; //one of the items
};
static_assert(sizeof(CInputList) == 16);

struct ISteamNetworking_vftable {
    undefined* SendP2PPacket;
    undefined* IsP2PPacketAvailable;
    undefined* ReadP2PPacket;
    undefined* AcceptP2PSessionWithUser;
    undefined* CloseP2PSessionWithUser;
    undefined* CloseP2PChannelWithUser;
    undefined* GetP2PSessionState;
    undefined* AllowP2PPacketRelay;
    undefined* CreateListenSocket;
    undefined* CreateP2PConnectionSocket;
    undefined* CreateConnectionSocket;
    undefined* DestroySocket;
    undefined* DestroyListenSocket;
    undefined* SendDataOnSocket;
    undefined* IsDataAvailableOnSocket;
    undefined* RetrieveDataFromSocket;
    undefined* IsDataAvailable;
    undefined* RetrieveData;
    undefined* GetSocketInfo;
    undefined* GetListenSocketInfo;
    undefined* GetSocketConnectionType;
    undefined* GetMaxPacketSize;
};
static_assert(sizeof(ISteamNetworking_vftable) == 88);

struct ISteamMatchmaking_vftable {
    undefined* GetFavoriteGameCount;
    undefined* GetFavoriteGame;
    undefined* AddFavoriteGame;
    undefined* RemoveFavoriteGame;
    undefined* RequestLobbyList;
    undefined* AddRequestLobbyListStringFilter;
    undefined* AddRequestLobbyListNumericalFilter;
    undefined* AddRequestLobbyListNearValueFilter;
    undefined* AddRequestLobbyListFilterSlotsAvailable;
    undefined* AddRequestLobbyListDistanceFilter;
    undefined* AddRequestLobbyListResultCountFilter;
    undefined* AddRequestLobbyListCompatibleMembersFilter;
    undefined* GetLobbyByIndex;
    undefined* CreateLobby;
    undefined* JoinLobby;
    undefined* LeaveLobby;
    undefined* InviteUserToLobby;
    undefined* GetNumLobbyMembers;
    undefined* GetLobbyMemberByIndex;
    undefined* GetLobbyData;
    undefined* SetLobbyData;
    undefined* GetLobbyDataCount;
    undefined* GetLobbyDataByIndex;
    undefined* DeleteLobbyData;
    undefined* GetLobbyMemberData;
    undefined* SetLobbyMemberData;
    undefined* SendLobbyChatMsg;
    undefined* GetLobbyChatEntry;
    undefined* RequestLobbyData;
    undefined* SetLobbyGameServer;
    undefined* GetLobbyGameServer;
    undefined* SetLobbyMemberLimit;
    undefined* GetLobbyMemberLimit;
    undefined* SetLobbyType;
    undefined* SetLobbyJoinable;
    undefined* GetLobbyOwner;
    undefined* SetLobbyOwner;
    undefined* SetLinkedLobby;
    undefined* CheckForPSNGameBootInvite;
};
static_assert(sizeof(ISteamMatchmaking_vftable) == 156);

struct maybe_network_stuff_static_ {
    undefined __0000[0x110];
};
static_assert(sizeof(maybe_network_stuff_static_) == 272);

struct CharSelect {
    undefined4 __0000;
    undefined4 __0004;
    undefined4 __0008;
    float _char_to_cursor_pos[0x24][2];
    undefined __012c[0x2e0];
    int __040c[0x24];
    undefined __049c[0x4b0];
    int __094c[2];
    int flags;
    undefined __0958[4];
    int __095c;
    float p_cursor[2][2]; //p1 x, p1 y, p2 x, p2 y
    undefined __0970[0x10];
    int __0980[2];
    int __0988[2];
    undefined __0990[8];
    int __0998[2];
    int __09a0[2];
    int __09a8[2];
    int p_select_state_[2];
    int __09b8[2];
    undefined __09c0[0x4c];
    int p_char_index[2];
    int p_palette_index[2];
    undefined __0a1c[8];
    int p_stylish[2];
    int __0a2c[2];
    undefined __0a34[0x100];
    int time_left_;
    int time_left_in_frames_;
    int __0b3c[2];
    undefined __0b44[0x23c];
    int __0d80[2];
    undefined __0d88[0x208];
    char __0f90[0x100];
    char __1090[0x100];
    undefined __1190[0x100];
    undefined _stages_struct[0x31][0x50];
    int stage_cursor[2]; //x, y
    undefined __21e8[0x260];
    int stage_cursor_last_[2];
    int music_cursor[2]; //start of a music struct
    undefined __2458[0x47][0x14];
    int __29e4;
    undefined __29e8[0x14];
    int music_index_;
    undefined __2a00[0x2c];
    int __2a2c[2];
    int __2a34[2];
    int __2a3c[2];
    int __2a44[2];
    int __2a4c[2];
    int __2a54[2];
    int __2a5c[2];
    int __2a64[2];
    undefined __2a6c;
    undefined __2a6d;
    undefined __2a6e;
    undefined __2a6f;
};
static_assert(sizeof(CharSelect) == 10864);

struct PlayerDataBig {
    undefined4 valid_;
    undefined4 __0004;
    undefined4 __0008;
    undefined4 __000c;
    undefined4 __0010[0x146];
    undefined4 char_index;
    undefined4 palette_index;
    undefined4 __0530; //== small._0xc ?
    undefined4 __0534;
    undefined4 stylish;
    undefined __053c[0x1d4];
};
static_assert(sizeof(PlayerDataBig) == 1808);

/*stores flags for SaveUtil, but also something to do with UpdateWindow?*/
struct AutoSaveStruct {
    int action;
    int prev_action;
    int prev_action_1_;
    int __000c;
    int frames_in_action_;
    int __0014;
    int __0018;
    int __001c;
    int __0020;
    int __0024;
    int __0028;
    int __002c;
    int __0030;
    int replay_index;
    int __0038;
    int done_;
    int __0040;
    int __0044;
    int saveutil_flags; //1: bbsave, 2: replay. 4: replay_list
    int save_action;
    int prev_save_action;
    int __0054;
};
static_assert(sizeof(AutoSaveStruct) == 88);

struct SteamInterfaces {
    undefined*** client;
    struct ISteamUser_vftable** user;
    undefined*** friends;
    undefined*** utils;
    struct ISteamMatchmaking_vftable** matchmaking;
    undefined*** userstats;
    undefined*** apps;
    undefined*** matchmakingservers;
    struct ISteamNetworking_vftable** networking;
    undefined*** storage;
    undefined*** screenshots;
    undefined*** http;
    undefined*** messages;
    undefined*** controller;
    undefined*** ugc;
    undefined*** pplist;
    undefined*** music;
    undefined*** musicremote;
    undefined*** htmlsurface;
    undefined*** inventory;
    undefined*** video;
};
static_assert(sizeof(SteamInterfaces) == 84);

struct FPAC {
    char __0000[4]; //"FPAC"
    int first_data_offset;
    int buffer_size_;
    int item_count;
    int flags; //I see 1 in memory and many tests for 2
    int item_name_length; //to get item_length, you need to add 0x1c to this, and round down to 0x10
    undefined4 __0018;
    undefined4 __001c;
    undefined items[0x10]; //variable length
};
static_assert(sizeof(FPAC) == 48);

struct SettingsMenuItem {
    int active;
    int extra_;
    int __0008;
    int __000c;
    int __0010;
    char name[0x20];
    int bbsave_index;
};
static_assert(sizeof(SettingsMenuItem) == 56);

struct MenuItem {
    int action_;
    char id[0x20];
    char title[0x20];
};
static_assert(sizeof(MenuItem) == 68);

struct SubMenu {
    int pad_00;
    int pad_04;
    char id[0x20];
    char title[0x20];
    int item_count;
    int item_index;
    struct MenuItem items[0x18];
};
static_assert(sizeof(SubMenu) == 1712);

enum GameMode {
    GameMode_Arcade = 1,
    GameMode_Story = 4,
    GameMode_Versus = 5,
    GameMode_Training = 6,
    GameMode_Tutorial = 7,
    GameMode_Challenge = 8,
    GameMode_Gallery = 9,
    GameMode_ItemShop = 0xa,
    GameMode_ReplayTheater = 0xb,
    GameMode_TitleScreen = 0xc,
    GameMode_MainMenuScreen = 0xd,
    GameMode_Online = 0xf,
    GameMode_Abyss = 0x10,
    GameMode_DCodeEdit = 0x12,
};

enum GameState {
    GameState_Warning = 1,
    GameState_ArcsysLogo = 2,
    GameState_IntroVideoPlaying = 3,
    GameState_TitleScreen = 4,
    GameState_CharacterSelectionScreen = 6,
    GameState_ArcadeActSelectScreen = 0xb,
    GameState_ScoreAttackModeSelectScreen = 0xb,
    GameState_SpeedStarModeSelectScreen = 0xb,
    GameState_ArcadeCharInfoScreen = 0xc,
    GameState_ArcadeStageSelectScreen = 0xd,
    GameState_VersusScreen = 0xe,
    GameState_InMatch = 0xf,
    GameState_VictoryScreen = 0x10,
    GameState_VictoryScreen_1 = 0x11,
    GameState_VictoryScreen_2 = 0x12,
    GameState_StoryMenu = 0x18,
    GameState_GalleryMenu = 0x19,
    GameState_ItemMenu = 0x19,
    GameState_ReplayMenu = 0x1a,
    GameState_MainMenu = 0x1b,
    GameState_TutorialMenu = 0x1c,
    GameState_LibraryMenu = 0x1d,
    GameState_MatchResult = 0x1e,
    GameState_Lobby = 0x1f,
    GameState_CAdvCtrlTask = 0x20,
    GameState_StoryPlaying = 0x21,
    GameState_AbyssMenu = 0x22,
    GameStata_TestMyRoom = 0x26,
    GameState_DCodeEdit = 0x27,
    GameState_TestRingcommand = 0x28,
    GameState_PreTeamBattle = 0x29,
};

struct PlayerDataSmall {
    undefined4 char_index;
    undefined4 __0004; //0, 1 or 1, 0? player index?
    undefined4 palette_index;
    undefined4 __000c; //0?
    undefined4 __0010; //const 1? valid? 0 on dummy
    undefined4 __0014; //0 in Arcade?
    undefined4 __0018; //0?
    undefined4 __001c; //reset to 100
};
static_assert(sizeof(PlayerDataSmall) == 32);

struct GameVals {
    undefined4 __0000;
    undefined __0004[0x100];
    int flags_;
    enum GameMode GameMode;
    enum GameState GameScene;
    enum GameState NextGameScene;
    undefined4 __0114;
    struct PlayerDataBig player_data_big[3];
    struct PlayerDataSmall player_data_small[2];
    int active_player;
    undefined4 stage;
    undefined4 music;
    undefined __1694[4];
    undefined4 __1698;
    undefined __169c[8];
    undefined4 __16a4;
    undefined4 __16a8;
    undefined4 GameMode_copy_;
    undefined __16b0[8];
    undefined player_data_copy_[2][0x710];
    undefined player_data_1_copy_[2][0x20];
    undefined4 active_player_copy_;
    undefined _other_field_copies[0x2c];
    undefined4 __2548;
    undefined4 __254c;
    undefined4 __2550;
    undefined4 __2554;
    int _next_scene_alt; //used in 001505a0 CSceneController::_update_1
    undefined4 __255c;
    undefined4 __2560;
    undefined4 __2564;
    undefined4 __2568;
    undefined4 __256c;
    undefined4 __2570;
    undefined4 __2574;
    undefined4 __2578;
    undefined4 __257c;
    undefined4 __2580;
    undefined4 __2584;
    undefined4 __2588;
    undefined4 __258c;
    undefined4 __2590;
    undefined4 __2594;
    undefined4 __2598;
    undefined4 __259c;
    undefined4 __25a0;
    undefined4 __25a4;
    undefined4 __25a8;
    undefined4 __25ac;
    undefined4 __25b0;
    undefined4 __25b4;
    undefined4 __25b8;
    undefined4 __25bc;
    undefined __25c0[0xc];
    undefined4 __25cc;
    undefined4 __25d0;
    undefined __25d4[4];
    undefined4 __25d8;
    undefined4 __25dc;
    undefined4 __25e0;
    undefined4 __25e4;
    struct OBJ_CCharBase* player_entities_[2];
    int input_controller_index_;
    int synth_controller_index_for_player[2]; //0,1, or 1,0 if you're p2
    undefined __25fc[4];
    uint GameSceneStatus;
    struct SCENE_CBase* current_scene;
    undefined __2608[8];
};
static_assert(sizeof(GameVals) == 9744);

struct FileReaderStruct {
    struct FileReaderStructItem* queue;
    int last_1_;
    int index_2_;
    int __000c;
    int count_;
    int __0014;
    int __0018;
};
static_assert(sizeof(FileReaderStruct) == 28);

/*size should be 0x2b0, but doesn't fit in stack?*/
struct MenuMessage {
    undefined* __0000;
    char name_[0x40];
    undefined __0044[4];
    undefined4 __0048;
    char type_[0x40];
    undefined4 index_;
    undefined4 length_;
    char items_[0x40];
    char __00d4[0x40];
    undefined __0114[0x100];
    undefined4 index_1_;
    undefined4 length_1_;
    undefined __021c[0x84];
};
static_assert(sizeof(MenuMessage) == 672);

struct MainMenu {
    undefined __0000[8];
    undefined4 __0008;
    undefined __000c[0x2c];
    undefined4 __0038;
    undefined __003c[0x18];
    int menu_level;
    undefined __0058[0xc];
    int state;
    undefined __0068[8];
    struct SubMenu sub_menus[0x10];
    int __6b70;
    int sub_menu_index;
    undefined __6b78[0x88];
    int p1_controller; //-1 for CPU
    int p2_controller;
    undefined __6c08[0x64c];
    int replay_list_view[6]; //int selected_inedx, first_index_in_view, last_index_in_view, ??, ??, ??
    undefined __726c[0x1040];
};
static_assert(sizeof(MainMenu) == 33452);

struct FileReaderStructItem {
    undefined* buffer;
    char filename[0x100];
    undefined4 __0104;
    undefined4 file_start_offset_; //set_up param_4
    undefined4 file_size; //set_up param_5
    undefined4 __0110; //set_up param_6
    undefined4 __0114; //set_up 0
    undefined4 state;
    undefined4 __011c; //set_up param_8=0
    undefined4 file_offset; //set_up 0
};
static_assert(sizeof(FileReaderStructItem) == 292);

struct NetworkStruct {
    int state;
    int state_1_;
    undefined __0008[0x1c];
    undefined4 action_queue[0x20];
    int queue_start;
    int queue_end;
    undefined4 __00ac;
    undefined __00b0[8];
    int message_id;
    undefined4 __00bc;
    undefined4 __00c0;
    undefined4 __00c4;
    undefined4 __00c8;
    undefined __00cc[0x40];
    struct room_settings_intemediary room_settings;
    undefined __018c[0x60];
    int lobby_index_;
    undefined __01f0[0x80];
    undefined4 __0270;
    undefined4 __0274;
    undefined4 __0278;
    undefined __027c[0x64];
    wchar_t chat_input_copy[0x20];
    undefined __0320[8];
    wchar_t chat_input[0x20];
    undefined __0368[0xd0];
    undefined4 chat_send_flag_;
    undefined __043c[4];
};
static_assert(sizeof(NetworkStruct) == 1088);

/*PlaceHolder Class Structure*/
struct AADX_CRenderer {
    undefined** vftable;
    undefined __0004[0x2c];
    undefined*** directXDevice;
    undefined __0034[0x1214];
};
static_assert(sizeof(AADX_CRenderer) == 4680);

/*PlaceHolder Class Structure*/
struct AASTEAM_SearchResult {
    undefined** vftable;
    ulonglong steam_id;
    undefined __000c[8];
};
static_assert(sizeof(AASTEAM_SearchResult) == 20);

struct GAMESTEAM_CNetworkServer {
    undefined** vftable;
    undefined4 __0004;
    undefined __0008[0xd0];
    undefined lobby_data[8]; //param2 in CreateLobby_with_data
    wchar_t username[0x10];
    undefined __0100[8];
    wchar_t username_1[0x10];
    undefined __0128[0xb0];
    int host_netcolor;
    undefined __01dc[0xf4]; //end of lobby_data struct?
    undefined _lobby_search_params[0x108]; //filled in by FUN_000a94c0
    undefined _join_lobby_params[0x408]; //filled in by FUN_000a89d0
    undefined __07e0[0x200];
    undefined __09e0[0xca];
    undefined __0aaa[0xe];
    undefined4 __0ab8;
    undefined __0abc[0x20];
    struct AA_LooseList lobby_search_results; //list of GAMESTEAM_SearchResultNode
    int lobby_count;
    struct GAMESTEAM_SearchResultNode* lobby_node;
    undefined4 __0af0;
    int lobby_order_[0x20]; //just 0,1,2,...31
    undefined __0b74[0x4c];
    undefined _CreateLobby_arg[0x90]; //AASTEAM_CSessionCreateObj
    undefined _JoinLobby_arg[0x70]; //AASTEAM_CSessionJoinObj
    undefined _RequestLobbyList_arg[0x2c]; //AASTEAM_CSessionSearchObj ...
    undefined(* _steam_params)[0x4c];
    int search_results_count;
    struct AASTEAM_SearchResult search_results[0x32];
    undefined4 __10dc;
    undefined call_result[0x20];
    undefined __1100[0x70]; //AASTEAM_CSessionJoinObj
    undefined __1170[0x1460];
    ulonglong lobby_id;
    undefined __25d8[0x18];
};
static_assert(sizeof(GAMESTEAM_CNetworkServer) == 9712);

/*PlaceHolder Class Structure*/
struct GAMESTEAM_VirtualKeyboard {
    undefined** vftable;
    undefined __0004[0x220];
};
static_assert(sizeof(GAMESTEAM_VirtualKeyboard) == 548);

struct CNetworkUIPlayerMatchCreate {
    undefined** vftable;
    int open_;
    undefined __0008[0x1c4];
    int room_type; //1: FFA
    undefined __01d0[0xb4];
    int room_capacity; //add 2 to get real capacity
    undefined __0288[0xb4];
    int room_invitations;
    undefined __0340[0xb4];
    int room_connectivity;
    undefined __03f8[0xb4];
    int room_color;
    undefined __04b0[0xb4];
    int room_match_limit; //0: no limit
    undefined __0568[0xb4];
    int room_rematch;
    undefined __0620[0xb4];
    int room_auto_pass;
    undefined __06d8[0xb4];
    int room_skip_time;
    undefined __0790[0xb4];
    int room_chat;
    undefined __0848[0xb4];
    int room_rotation;
    undefined __0900[0xb4];
    int room_rounds_to_win;
    undefined __09b8[0xb4];
    int room_time;
    undefined __0a70[0xcdc];
    undefined4 __174c;
    undefined __1750[0x10];
    wchar_t room_name[0x10];
    undefined __1780[0x60];
    int modified;
};
static_assert(sizeof(CNetworkUIPlayerMatchCreate) == 6116);

struct BBCF {
    undefined __0000[0x4f90];
    undefined AA_LooseList__dtor_1_[0x200]; //00004f90
    undefined _get_hash_table_item_1[0x7f0]; //00005190
    undefined AA_LooseList__ctor[0x40]; //00005980
    undefined AA_ListNode__dtor[0x10]; //000059c0
    undefined AA_LooseList__dtor[0x1b0]; //000059d0
    undefined AA_LooseList__insert_before[0x160]; //00005b80
    undefined AA_LooseList__pop_head[0x20]; //00005ce0
    undefined AA_LooseList__append[0x80]; //00005d00
    undefined AA_LooseList__remove[0x780]; //00005d80
    undefined _float_rgba_to_bytes_1_[0x30]; //00006500
    undefined _float_rgba_to_bytes[0x1e0]; //00006530
    undefined _hash_0_[0x680]; //00006710
    undefined _memzero_1_[0xe0]; //00006d90
    undefined AA_TaskList__ctor[0x50]; //00006e70
    undefined AA_TaskList__dtor[0x90]; //00006ec0
    undefined AA_TaskList__dtor_1[0xc0]; //00006f50
    undefined AA_TaskSerchEngine_PriorityEqual__pred[0x40]; //00007010
    undefined AA_TaskList__run_tasks[0xc0]; //00007050
    undefined AA_TaskList__add_task[0x80]; //00007110
    undefined AA_TaskList__find_task[0x40]; //00007190
    undefined AA_CCameraController___update_task[0x10]; //000071d0
    undefined AA_CFileController__dtor_1[0xd0]; //000071e0
    undefined AA_CFileController__dtor[0x30]; //000072b0
    undefined AA_TaskNode__dtor[0x60]; //000072e0
    undefined AA_CFileController__update_task[0x230]; //00007340
    undefined AA_CFileCtrlFactory__create_thread[0x20]; //00007570
    undefined AA_CNetworkReceiver__dtor[0xf0]; //00007590
    undefined AA_CNetworkReceiver___update_task[0x40]; //00007680
    undefined AA_CNetworkReceiverFactory___create[0x120]; //000076c0
    undefined AA_CRenderController___update_task[0x140]; //000077e0
    undefined AA_CSoundController___update_task[0x20]; //00007920
    undefined _ctor_for_CSteamID_[0x30]; //00007940
    undefined _wcscpy_s_32_[0x100]; //00007970
    undefined _get_steam_interfaces[0x3e]; //00007a70
    undefined inject_SteamUser[0x15]; //00007aae
    undefined inject_SteamFriends[0x14]; //00007ac3
    undefined inject_SteamUtils[0x15]; //00007ad7
    undefined inject_SteamMatchmaking[0x2e]; //00007aec
    undefined inject_SteamUserStats[0x32]; //00007b1a
    undefined inject_SteamNetworking[0x1a4]; //00007b4c
    undefined _get_steam_id_[0x150]; //00007cf0
    undefined _get_steam_friends[0x20]; //00007e40
    undefined _steam_init[0xc0]; //00007e60
    undefined _strcat32[0x800]; //00007f20
    undefined _get_AA_CCameraManager[0x250]; //00008720
    undefined _strcpy256[0xd0]; //00008970
    undefined FileReaderStruct___check_item_state_3[0x30]; //00008a40
    undefined FileReaderStruct___get_item[0x20]; //00008a70
    undefined _get_FileReaderStruct[0xb0]; //00008a90
    undefined FileReaderStruct__init[0x180]; //00008b40
    undefined FileReaderStruct___clear[0x30]; //00008cc0
    undefined FileReaderStruct___check_and_clear_state_3[0x40]; //00008cf0
    undefined FileReaderStruct___set_up_reading_3[0x50]; //00008d30
    undefined FileReaderStruct___set_up_reading[0xf0]; //00008d80
    undefined FileReaderStruct___set_up_reading_0[0x1b0]; //00008e70
    undefined _get_DAT_006128e0_render3_[0x4c0]; //00009020
    undefined CInputList___get_by_index[0x20]; //000094e0
    undefined _get_CInputList[0x90]; //00009500
    undefined CInputList___reset[0x60]; //00009590
    undefined CInputList___add[0x70]; //000095f0
    undefined CInputList__dtor[0x60]; //00009660
    undefined CInputList___update_all[0xe0]; //000096c0
    undefined _get_AA_CModelInstanceManager[0xb0]; //000097a0
    undefined _get_DAT_00612934[0xa0]; //00009850
    undefined DAT_00623674___get_buffer_item[0x640]; //000098f0
    undefined _get_DAT_00612990[0xb0]; //00009f30
    undefined _get_DAT_00612978[0x3e0]; //00009fe0
    undefined _get_DAT_006129b0[0x660]; //0000a3c0
    undefined AA_CParticleManager__ctor[0x630]; //0000aa20
    undefined AA_CParticleManager___set_enabled_[0x240]; //0000b050
    undefined _get_AA_CParticleManager[0xa70]; //0000b290
    undefined _get_DAT_00612a68[0x160]; //0000bd00
    undefined hash_1_[0xa0]; //0000be60
    undefined hash_2_[0x90]; //0000bf00
    undefined hash_3_[0x90]; //0000bf90
    undefined AA_CRandomManager__init[0x160]; //0000c020
    undefined get_AA_CRandomManager[0x1f0]; //0000c180
    undefined _get_DAT_006135d4_render1_[0x1b50]; //0000c370
    undefined _hash_buffer_[0x90]; //0000dec0
    undefined _get_DAT_006235f4[0x100]; //0000df50
    undefined _get_DAT_00623610[0x150]; //0000e050
    undefined _get_DAT_00623630_sound_[0xb0]; //0000e1a0
    undefined _get_DAT_00623630_sound__item[0x280]; //0000e250
    undefined _get_DAT_00623658[0x1e0]; //0000e4d0
    undefined _get_DAT_00623674_palettes_[0x300]; //0000e6b0
    undefined _get_DAT_00623698_render2_[0x560]; //0000e9b0
    undefined AA_CApplication__ctor[0x60]; //0000ef10
    undefined AA_CApplication__dtor_[0x50]; //0000ef70
    undefined AA_CApplication__dtor_1[0x6b0]; //0000efc0
    undefined AA_CFontBase___call_vftable_0xc[0x3100]; //0000f670
    undefined _get_DAT_006236f0[0x1bb0]; //00012770
    undefined _get_AA_CFriendList[0xf0]; //00014320
    undefined AA_CNetworker__ctor[0x40]; //00014410
    undefined _ctor_for_chat[0x80]; //00014450
    undefined AA_CNetworker__dtor[0x2f0]; //000144d0
    undefined _get_AA_CTitleManagedStorage[0x28d0]; //000147c0
    undefined _get_lpString_00ec3910[0x50]; //00017090
    undefined _get_lpString_00ec5228[0x1e00]; //000170e0
    undefined _get_DAT_006253a0[0xe0]; //00018ee0
    undefined _get_DInput8Struct[0x70]; //00018fc0
    undefined _call_DirectInput8Create_1[0x491]; //00019030
    undefined inject_DenyKeyboardInputFromGame[0x53f]; //000194c1
    undefined AASTEAM_CInputPad__init[0xb20]; //00019a00
    undefined AASTEAM_CInputXInputPad__init[0x460]; //0001a520
    undefined _LeaveCriticalSection_1[0x390]; //0001a980
    undefined AASTEAM_ThreadObject___ctor[0x1b0]; //0001ad10
    undefined CCallbackImpl_lt16_gt__ctor[0x2e0]; //0001aec0
    undefined AASTEAM_CNetworker__ctor[0x170]; //0001b1a0
    undefined _ctor_for_lobby_search_results[0xc80]; //0001b310
    undefined _CreateLobby_[0xf0]; //0001bf90
    undefined _LeaveLobby_[0x90]; //0001c080
    undefined AASTEAM_CNetworker___JoinLobby_[0xd0]; //0001c110
    undefined _LeaveLobby_1[0x90]; //0001c1e0
    undefined AASTEAM_CSessionSearchObj___RequestLobbyList[0x350]; //0001c270
    undefined AASTEAM_CNetworker___BLoggedOn_[0x170]; //0001c5c0
    undefined _steam_copy_response_[0x1c0]; //0001c730
    undefined _read_steam_param_int[0x10]; //0001c8f0
    undefined get_AASTEAM_CNetworker[0x70]; //0001c900
    undefined _get_lobby_search_results[0x150]; //0001c970
    undefined AASTEAM_CNetworker___handle_invite_callback[0xd0]; //0001cac0
    undefined _CreateLobby_callback[0x160]; //0001cb90
    undefined _GetLobbyMemberData_[0x90]; //0001ccf0
    undefined _GetLobbyMemberData_1[0x90]; //0001cd80
    undefined _JoinLobby_callback[0x130]; //0001ce10
    undefined RequestLobbyList_callback[0x1a0]; //0001cf40
    undefined AASTEAM_CNetworker___ReadP2PPacket_[0x280]; //0001d0e0
    undefined _write_steam_param_from_str[0x30]; //0001d360
    undefined _write_steam_param_from_int[0x60]; //0001d390
    undefined _get_steam_matchmaking[0x20]; //0001d3f0
    undefined AASTEAM_CNetworker___update[0x10]; //0001d410
    undefined _SetLobbyData_[0x770]; //0001d420
    undefined _return_1[0x1750]; //0001db90
    undefined _get_AASTEAM_CRankingReader[0x18f0]; //0001f2e0
    undefined AASTEAM_CReplayDownloadTask___update_task[0x850]; //00020bd0
    undefined AASTEAM_CReplayDownloadTask___dtor[0x4a0]; //00021420
    undefined AASTEAM_CReplayUploadTask___update_task[0xb0]; //000218c0
    undefined GAMESTEAM_COnlineStorageTransfer___get_AASTEAM_CReplayUploader[0x380]; //00021970
    undefined AASTEAM_CReplayUploadTask___dtor[0xa90]; //00021cf0
    undefined AASTEAM_CUMSTask___update_task[0x550]; //00022780
    undefined GAMESTEAM_COnlineStorageTransfer___get_AASTEAM_CUserManagedStorage[0x3d0]; //00022cd0
    undefined AASTEAM_CUMSTask___dtor[0x1830]; //000230a0
    undefined _get_AASTEAM_CVoiceChatManager[0x1390]; //000248d0
    undefined _get_AASTEAM_CVoiceSound[0x160]; //00025c60
    undefined AASTEAM_CVoiceContext___empty_func[0x11b0]; //00025dc0
    undefined _get_DAT_00659e70[0x6b0]; //00026f70
    undefined _strcpy32[0x6b0]; //00027620
    undefined _get_DAT_0065a050[0x20f0]; //00027cd0
    undefined _empty_func[0x2b00]; //00029dc0
    undefined _return_6[0x4ec0]; //0002c8c0
    undefined _free_2[0x750]; //00031780
    undefined _free_1[0x6b90]; //00031ed0
    undefined _get_s_empty_[0x420]; //00038a60
    undefined _assert_0_[0x90]; //00038e80
    undefined _assert_0[0xa70]; //00038f10
    undefined CSaveDataManager___get_bbsave_0x60[0x1e00]; //00039980
    undefined AADX_CRenderer___EndScene_[0x220]; //0003b780
    undefined AADX_CRenderer___update_window_[0x150]; //0003b9a0
    undefined _get_AADX_CRenderer[0x2880]; //0003baf0
    undefined _strcpy64_zero_pad_[0x300]; //0003e370
    undefined _get_AADX_CRenderInserter[0x60]; //0003e670
    undefined _get_AADX_CRenderInserter_Less[0x820b]; //0003e6d0
    undefined inject_GetViewAndProjMatrixes[0x2f15]; //000468db
    undefined _get_DAT_0065b7e8[0x2550]; //000497f0
    undefined _get_DAT_0065b7f4[0x2f0]; //0004bd40
    undefined AADX_CDynamicTextureListFactory___create[0x850]; //0004c030
    undefined AADX_CDynamicTextrueList__reported_crashes_thread_sometimes[0x13d0]; //0004c880
    undefined _get_DAT_0065b81c[0xaf0]; //0004dc50
    undefined _WriteMiniDump[0x42]; //0004e740
    undefined inject_SetDumpfileCommentString[0x38e]; //0004e782
    undefined entry_1[0x300]; //0004eb10
    undefined _assert_fail_log[0x90]; //0004ee10
    undefined _assert_fail_and_exit_1[0xa0]; //0004eea0
    undefined AAWin_CApplication__ctor[0x90]; //0004ef40
    undefined AAWin_CApplication__dtor[0x90]; //0004efd0
    undefined AAWin_CApplication__init_[0x180]; //0004f060
    undefined AAWin_CApplication__main_game_loop[0x179]; //0004f1e0
    undefined inject_WindowMsgHandler[0x237]; //0004f359
    undefined AAWin_HQTimer__dtor[0xb0]; //0004f590
    undefined _get_AAWin_HQTimer[0xd0]; //0004f640
    undefined AAWin_HQTimer__write_replay_timestamp[0x270]; //0004f710
    undefined AAWin_CFileReader_Thread__ctor[0xb0]; //0004f980
    undefined AAWin_CFileReader_Thread__dtor[0xe0]; //0004fa30
    undefined AAWin_CFileReader_Thread__dtor_1[0x60]; //0004fb10
    undefined AAWin_CFileReader_Thread__thread_start[0x790]; //0004fb70
    undefined AAWin_CFileReader_Thread___field_0x54_1__set_event_0_[0xc0]; //00050300
    undefined AAWin_CFileReader_Thread___field_0x54__3[0x50]; //000503c0
    undefined AAWin_CFileReader_Thread__get_file_size[0xd0]; //00050410
    undefined get_AAWin_CFileReader_Thread[0x70]; //000504e0
    undefined AAWin_CFileReader_Thread___try_reading_0x1000_bytes[0x90]; //00050550
    undefined AAWin_CFileReader_Thread__create_thread[0x100]; //000505e0
    undefined AAWin_CFileReader_Thread___try_reading_0x1000_bytes_1[0xd0]; //000506e0
    undefined AAWin_CFileReader_Thread___try_reading_file[0x1c0]; //000507b0
    undefined AAWin_CFileReader_Thread___check_free_space[0x40]; //00050970
    undefined AAWin_CFileReader_Thread___field_0x54_0[0x2c0]; //000509b0
    undefined _strcpy64[0x90]; //00050c70
    undefined AA_CSoundEngine_XACT__dtor_[0x40]; //00050d00
    undefined AA_CSoundBank_XACT___AddSoundBank_[0x2a0]; //00050d40
    undefined get_AA_CSoundEngine_XACT[0x7e0]; //00050fe0
    undefined AA_CWaveBankDataBase_XACT___RegistBank_[0x1180]; //000517c0
    undefined AA_CRandomManager__hash_[0x2bb0]; //00052940
    undefined AA_CModel_MUA___get_DAT_0049959c[0x2560]; //000554f0
    undefined FPAC__get_item[0x40]; //00057a50
    undefined FPAC__get_item_size[0x30]; //00057a90
    undefined FPAC__get_item_count[0x10]; //00057ac0
    undefined FPAC__get_item_data[0x40]; //00057ad0
    undefined FPAC__get_item_data_size[0x40]; //00057b10
    undefined FPAC___find_index_by_name[0x80]; //00057b50
    undefined FPAC___find_index_by_name_1[0x1880]; //00057bd0
    undefined AA_CCollision_JON__ctor[0xa0]; //00059450
    undefined AA_CCollision_JON__dtor[0x690]; //000594f0
    undefined CHashNodeC32BYTE___ctor[0x310]; //00059b80
    undefined _str_equals_[0x5910]; //00059e90
    undefined GameVals___get_active_player_data[0x60]; //0005f7a0
    undefined _vsprintf_s_1[0x20]; //0005f800
    undefined _vsprintf_1[0x20]; //0005f820
    undefined _strcat64[0x90]; //0005f840
    undefined _strcat256[0x1100]; //0005f8d0
    undefined BG_CIndividualFactory___create[0xaf0]; //000609d0
    undefined _get_DAT_0065c612[0x1f40]; //000614c0
    undefined OBJ_CBase___bbcmd_21014_00063400[0x460]; //00063400
    undefined game_Stat_PCoinManager___init[0x950]; //00063860
    undefined GAME_SynthKeyControler___check_button_hit[0x40]; //000641b0
    undefined GAME_SynthKeyControler___check_button_down_[0x80]; //000641f0
    undefined _get_GAME_BattleSynthKeyControler_by_index[0xb0]; //00064270
    undefined GAME_BattleSynthKeyControler___get_controller[0x10]; //00064320
    undefined GAME_SynthKeyControler___get_controller[0x10]; //00064330
    undefined GAME_BattleSynthKeyControler__get_input_for_player[0x18]; //00064340
    undefined inject_P1OverwriteNetplay[0xb8]; //00064358
    undefined _get_GAME_SynthKeyControler_by_index[0xb0]; //00064410
    undefined GAME_BattleSynthKeyControler___get_system_controller[0x10]; //000644c0
    undefined GAME_SynthKeyControler___get_system_controller[0x3e0]; //000644d0
    undefined _get_DAT_0065c6d8[0x880]; //000648b0
    undefined GAMESTEAM_CClassGenerator__ctor[0x480]; //00065130
    undefined GAMESTEAM_CClassGenerator___get_GAMESTEAM_CNetworkServer[0x10]; //000655b0
    undefined GAMESTEAM_CClassGenerator___get_AASTEAM_CNetworker[0x10]; //000655c0
    undefined AADX_CMovieTextureFactory___create[0x80]; //000655d0
    undefined AA_CCollisionFactory_JON___create[0x80]; //00065650
    undefined AA_CModelFactory_MUA___create[0x80]; //000656d0
    undefined AA_CPaletteFactory_HIP___create[0x70]; //00065750
    undefined AA_CTextureFactory_CmpHIP___create[0x70]; //000657c0
    undefined AA_CTextureFactory_DDS___create[0x80]; //00065830
    undefined AA_CTextureFactory_HIP___create[0x240]; //000658b0
    undefined GAMESTEAM_CClassGeneratorFactory___create[0x160]; //00065af0
    undefined GAMESTEAM_CClassGenerator___get_GAMESTEAM_VirtualKeyboard[0x2dd0]; //00065c50
    undefined _get_GAMESTEAM_ImeEditBox[0xce0]; //00068a20
    undefined GAMESTEAM_BattleKeyControler___get_input_for_button_[0x130]; //00069700
    undefined AA_CModel_MUA___return_0[0x100]; //00069830
    undefined GAMESTEAM_SystemKeyControlerFactory__create[0xef0]; //00069930
    undefined _get_CSTEAMNetworkLobbyData[0x1660]; //0006a820
    undefined _return_100[0x4c0]; //0006be80
    undefined AASTEAM_CSessionCreateObj__ctor[0x120]; //0006c340
    undefined AASTEAM_CSessionSearchObj__ctor[0x280]; //0006c460
    undefined GAMESTEAM_CNetworkServer___ctor[0x1a0]; //0006c6e0
    undefined GAME_CNetworkServer___ctor[0x420]; //0006c880
    undefined AASTEAM_CSessionSearchObj__dtor[0x220]; //0006cca0
    undefined AASTEAM_CSessionSearchObj__dtor_1[0xc0]; //0006cec0
    undefined GAMESTEAM_SearchResultNode___dtor[0x910]; //0006cf80
    undefined GAMESTEAM_CNetworkServer___read_lobby_search_results[0x2b0]; //0006d890
    undefined GAMESTEAM_CNetworkServer___writes_ping[0xc0]; //0006db40
    undefined GAMESTEAM_SearchResultNode___init[0x270]; //0006dc00
    undefined GAMESTEAM_CNetworkServer___JoinLobby_from_invite_[0x130]; //0006de70
    undefined GAMESTEAM_CNetworkServer___JoinLobby_[0x100]; //0006dfa0
    undefined GAMESTEAM_CNetworkServer___CreateLobby_with_data[0x5c0]; //0006e0a0
    undefined GAMESTEAM_CNetworkServer___check_PLAYER_PRIVATE_NUM[0x230]; //0006e660
    undefined GAMESTEAM_SearchResultNode___get_ping[0x30]; //0006e890
    undefined GAMESTEAM_CClassGenerator___get_GAMESTEAM_CNetworkServer_1[0x1c0]; //0006e8c0
    undefined GAME_CNetworkServer___get_s_empty_[0x10]; //0006ea80
    undefined GAMESTEAM_CNetworkServer___get_chat[0xb50]; //0006ea90
    undefined _prepend_chat_message[0xa0]; //0006f5e0
    undefined GAMESTEAM_CNetworkServer___new_GAMESTEAM_SearchResultNode[0x2c0]; //0006f680
    undefined GAMESTEAM_CNetworkServer__lobby_search[0x380]; //0006f940
    undefined GAMESTEAM_SearchResultNode___GetLobbyData[0x330]; //0006fcc0
    undefined GAMESTEAM_CNetworkServer___prepare_lobby_data_1_[0x1e0]; //0006fff0
    undefined GAME_CNetworkServer___return_0[0x260]; //000701d0
    undefined _vsprintf_s_16[0x20]; //00070430
    undefined _vsprintf_s_32[0x9e0]; //00070450
    undefined _updates_steam_friends_info[0x390]; //00070e30
    undefined _check_statistics_occasionally[0x440]; //000711c0
    undefined GAMESTEAM_CRanking___return_0[0x480]; //00071600
    undefined AASTEAM_SystemManager__ctor[0x110]; //00071a80
    undefined AASTEAM_SystemManager__dtor[0x1e0]; //00071b90
    undefined AASTEAM_SystemManager__dtor_1[0x310]; //00071d70
    undefined AASTEAM_SystemManager___create_app[0x240]; //00072080
    undefined AASTEAM_SystemManager___create_pad_input_controllers[0x1e0]; //000722c0
    undefined AASTEAM_SystemManager___create_CFadeController[0x40]; //000724a0
    undefined AASTEAM_SystemManager__create_CFileController[0x70]; //000724e0
    undefined AASTEAM_SystemManager___call_DirectInput8Create[0x30]; //00072550
    undefined AASTEAM_SystemManager___create_InputTask[0x30]; //00072580
    undefined AASTEAM_SystemManager___create_CNetworkTask[0x100]; //000725b0
    undefined AASTEAM_SystemManager___create_CParticleTask[0x50]; //000726b0
    undefined AASTEAM_SystemManager___create_CRenderCtrl[0x13f0]; //00072700
    undefined AASTEAM_SystemManager___create_CSoundCtrl[0x170]; //00073af0
    undefined AASTEAM_SystemManager___init_fonts_[0x290]; //00073c60
    undefined AASTEAM_SystemManager___create_SystemKeyControler[0x160]; //00073ef0
    undefined AA_CFontFactory_Tool___create[0x1a0]; //00074050
    undefined BB_CFontFactory_SystemFont___create[0x100]; //000741f0
    undefined GAME_CNetworkTaskFactory___create[0x3f0]; //000742f0
    undefined _get_GAMESTEAM_VirtualKeyboard[0x450]; //000746e0
    undefined get_GAME_CEventFaceDataManager[0x7d0]; //00074b30
    undefined _get_DAT_006651a0_char_names_[0x10f0]; //00075300
    undefined _vsprintf_name_in_DAT_006651a0[0x990]; //000763f0
    undefined _vsprintf_s_40[0x1640]; //00076d80
    undefined _get_DAT_00667ff0[0x790]; //000783c0
    undefined GAME_CEff3DInstHndlManager___dtor[0x2030]; //00078b50
    undefined _get_DAT_006770b8[0x80]; //0007ab80
    undefined _get_GAME_CEff3DInstHndlManager[0x120]; //0007ac00
    undefined AA_CBoneMotionFactory___create[0x70]; //0007ad20
    undefined AA_CMaterialMotionFactory___create[0x70]; //0007ad90
    undefined AA_CModelInstanceFactory___create[0x70]; //0007ae00
    undefined AA_CMotionFileFactory_MMOT___create[0x550]; //0007ae70
    undefined BBFileHashNodeFactory___create[0x7f0]; //0007b3c0
    undefined _get_DAT_0087a118_item[0xa0]; //0007bbb0
    undefined _get_DAT_0087a118[0x450]; //0007bc50
    undefined FPAC___get_FPAC_item_data_by_name_[0x820]; //0007c0a0
    undefined _get_DAT_00679658[0x70]; //0007c8c0
    undefined FPACBuffers___alloc[0x70]; //0007c930
    undefined FPACBuffers___free[0x50]; //0007c9a0
    undefined FPACBuffers___free_all[0x60]; //0007c9f0
    undefined FPACBuffers___get_data[0x40]; //0007ca50
    undefined _get_FPACBuffers[0x90]; //0007ca90
    undefined FPACBuffers___init[0xa0]; //0007cb20
    undefined GameVals__ctor[0x350]; //0007cbc0
    undefined GameVals__dtor[0xa00]; //0007cf10
    undefined GameVals___duplicate_many_fields[0x1d]; //0007d910
    undefined inject_GetPaletteIndexPointers[0xc93]; //0007d92d
    undefined _get_GAME_BattleSynthKeyControler_for_player[0x110]; //0007e5c0
    undefined GameVals___get_player_data_small_by_index[0xe0]; //0007e6d0
    undefined _get_GAME_SynthKeyControler_by_index_1[0x20]; //0007e7b0
    undefined _get_GAME_SynthKeyControler_for_player[0x90]; //0007e7d0
    undefined get_GameVals[0x150]; //0007e860
    undefined GameVals___get_synth_controller_index_for_active_player[0x10]; //0007e9b0
    undefined GameVals___get_active_player[0x1d0]; //0007e9c0
    undefined GameVals___get_char_index_for_player_[0x20]; //0007eb90
    undefined GameVals___get_player_data_big_by_index[0xc0]; //0007ebb0
    undefined GameVals___get_synth_controller_index_for_other_player[0x280]; //0007ec70
    undefined _ctor_for_player_data_big[0x450]; //0007eef0
    undefined GameVals___is_boss_[0x130]; //0007f340
    undefined NetworkStruct___get_0x270[0x160]; //0007f470
    undefined GameVals___check_player_data_is_valid[0x210]; //0007f5d0
    undefined GameVals___is_Arcade_and_check_active_player_data_0xc[0x4d0]; //0007f7e0
    undefined GameVals___reset[0x2b0]; //0007fcb0
    undefined GameVals___set_music_[0x1640]; //0007ff60
    undefined GameVals___reset_player_data_1[0x1420]; //000815a0
    undefined GAME_InputTask__update_task[0xf0]; //000829c0
    undefined MenuMessage___ctor[0x150]; //00082ab0
    undefined entry_2[0x4d0]; //00082c00
    undefined AA_PreloadTask___init_3[0x1b0]; //000830d0
    undefined AA_PreloadTask___init_6[0xf0]; //00083280
    undefined AA_PreloadTask___init_2[0x200]; //00083370
    undefined AA_PreloadTask__update_task[0x90]; //00083570
    undefined _get_GAME_EventManager[0x70]; //00083600
    undefined AA_PreloadTaskFactory___create[0x70]; //00083670
    undefined CSceneControllerTaskFactory___create[0x70]; //000836e0
    undefined SCENE_CTitleLoopFactory___create[0x80]; //00083750
    undefined AA_PreloadTask___init_0[0x740]; //000837d0
    undefined AA_PreloadTask___init_6_1[0x1f0]; //00083f10
    undefined AA_PreloadTask___init_1[0x19d0]; //00084100
    undefined _get_DAT_00892aa0[0x3b0]; //00085ad0
    undefined _get_AchievementManager[0xb0]; //00085e80
    undefined _get_AchievementCountManager[0x140]; //00085f30
    undefined _get_GAMESTEAM_CClassGenerator[0x70]; //00086070
    undefined _run_factory[0x180]; //000860e0
    undefined GAME_CCreditTask___update_task[0x79b0]; //00086260
    undefined _get_DAT_00897e90[0x110]; //0008dc10
    undefined _get_DAT_008a1ed0[0x6c80]; //0008dd20
    undefined _get_GAMESTEAM_CDLCManager[0x530]; //000949a0
    undefined GAME_CFadeTaskManager___get_index_[0x20]; //00094ed0
    undefined get_GAME_CFadeTaskManager[0x1150]; //00094ef0
    undefined _get_CInstallManager_NULL[0x140]; //00096040
    undefined GAME_KeyControler__dtor[0x3a0]; //00096180
    undefined GAME_KeyControler___check_button_hit[0x40]; //00096520
    undefined GAME_KeyControler___check_button_down_[0xa0]; //00096560
    undefined GAMESTEAM_BattleKeyControler__update_task[0x120]; //00096600
    undefined GAME_SynthKeyControler___check_button_hit_1[0xc0]; //00096720
    undefined GAME_SynthKeyControler___check_button_down_1[0x100]; //000967e0
    undefined GAME_KeyControler___format_input[0xb0]; //000968e0
    undefined GAME_KeyControler___create_CKeyRingBuffer[0x130]; //00096990
    undefined _get_DAT_005de0b0_index_[0x30]; //00096ac0
    undefined GAMESTEAM_BattleKeyControler__update_1[0x3910]; //00096af0
    undefined _get_DAT_0114d000[0xaa0]; //0009a400
    undefined NetforkStruct___handle_packets_[0x394]; //0009aea0
    undefined inject_PacketProcessingFunc[0x238c]; //0009b234
    undefined _get_NetUserData_0xd0[0x182e]; //0009d5c0
    undefined inject_GetRoomTwo[0x112]; //0009edee
    undefined inject_GetRoomOne[0x17d0]; //0009ef00
    undefined NetUserData___ctor[0x511]; //000a06d0
    undefined inject_GetPlayerAvatarBaseFunc[0x3ff]; //000a0be1
    undefined _get_NetUserData[0x1a10]; //000a0fe0
    undefined _get_CSTEAMNetworkLobbyData_1[0x16b0]; //000a29f0
    undefined _get_DAT_008f7758_entry_[0x3a0]; //000a40a0
    undefined _start_ranked_lobby_search__[0xf70]; //000a4440
    undefined GAME_CNetworkServer__dtor_1[0xa0]; //000a53b0
    undefined GAMESTEAM_CNetworkServer___get_lobby_node[0x190]; //000a5450
    undefined GAMESTEAM_CNetworkServer___call_read_lobby_search_results[0xd0]; //000a55e0
    undefined GAMESTEAM_CNetworkServer___call_join_lobby[0x30]; //000a56b0
    undefined GAMESTEAM_CNetworkServer___CreateLobby[0xf0]; //000a56e0
    undefined GAMESTEAM_CNetworkServer___call_lobby_search[0x90]; //000a57d0
    undefined NetworkStruct__ctor[0x430]; //000a5860
    undefined GAME_CNetworkTask__ctor[0x90]; //000a5c90
    undefined GAME_CNetworkTask__dtor[0x900]; //000a5d20
    undefined _ping_to_connection_bars[0x740]; //000a6620
    undefined NetworkStruct___update_popup[0x210]; //000a6d60
    undefined NetworkStruct___update[0x460]; //000a6f70
    undefined GAME_CNetworkTask__update_task[0x90]; //000a73d0
    undefined _get_StatBattleTmp[0x380]; //000a7460
    undefined _get_NetworkStruct[0x1b0]; //000a77e0
    undefined get_rematch_byte[0x1b0]; //000a7990
    undefined GAMESTEAM_CNetworkServer___get_lobby_node_1[0xf0]; //000a7b40
    undefined NetworkStruct___reset_state_[0x200]; //000a7c30
    undefined NetworkStruct___state_not_in_0_2_3_0xe[0x140]; //000a7e30
    undefined NetworkStruct___get_0x278__1[0x20]; //000a7f70
    undefined NetworkStruct___check_0xac_or_0xac_c[0x1c0]; //000a7f90
    undefined NetworkStruct___pop_first_action[0x780]; //000a8150
    undefined NetworkStruct___push_action[0x100]; //000a88d0
    undefined NetworkStruct___start_join_lobby[0xaf0]; //000a89d0
    undefined NetworkStruct___start_player_lobby_search[0x860]; //000a94c0
    undefined NetworkStruct___clear_action_queue[0x29a0]; //000a9d20
    undefined NetworkStruct___update_player_menu_[0x810]; //000ac6c0
    undefined _update_in_room_[0x1800]; //000aced0
    undefined NetworkStruct___update_ranked_menu_[0x15c0]; //000ae6d0
    undefined NetworkStruct___log_in_[0x520]; //000afc90
    undefined NetworkStruct___update_default_[0x7c0]; //000b01b0
    undefined NetworkStruct___log_in_2_[0x8d0]; //000b0970
    undefined maybe_reverts_room_settings_on_player_enter_leave[0x1220]; //000b1240
    undefined AASTEAM_SystemManager___init_CParticleTask_[0x270]; //000b2460
    undefined AASTEAM_SystemManager___create_CParticleTask_1[0x610]; //000b26d0
    undefined GAME_CParticleTaskFactory___create[0x3e0]; //000b2ce0
    undefined _return_0[0xc10]; //000b30c0
    undefined _get_DAT_008f7e30[0x52a0]; //000b3cd0
    undefined _get_GAMESTEAM_COnlineStorageTransfer[0x330]; //000b8f70
    undefined ReplayFileHeader__ctor[0xb0]; //000b92a0
    undefined CSaveDataManager__ctor[0x180]; //000b9350
    undefined CSaveDataManager__dtor[0x1b0]; //000b94d0
    undefined CSaveDataManager___set_bbsave_0xac7e_flag[0x80]; //000b9680
    undefined get_CSaveDataManager_bbsave[0x70]; //000b9700
    undefined get_CSaveDataManager[0x654]; //000b9770
    undefined inject_GetMoneyAddr[0x1ac]; //000b9dc4
    undefined GAME_CSaveTask__update_task[0x8e0]; //000b9f70
    undefined CSaveDataManager___get_save_status[0x10]; //000ba850
    undefined CSaveDataManager___get_ReplayList[0x10]; //000ba860
    undefined CSaveDataManager___get_replay_by_index[0x30]; //000ba870
    undefined _get_DAT_005df4d0[0x60]; //000ba8a0
    undefined _get_s_SVID_NoSaveData_004aa1d8[0x1c0]; //000ba900
    undefined ReplayFileHeader__init[0x100]; //000baac0
    undefined CSaveDataManager___init_bbsave_1[0x20]; //000babc0
    undefined CSaveDataManager___parent_ctor_[0x430]; //000babe0
    undefined CSaveDataManager___is_next_SaveUtil_action_running[0x2b0]; //000bb010
    undefined CSaveDataManager___set_next_SaveUtil_action_7_check[0x150]; //000bb2c0
    undefined CSaveDataManager___set_next_SaveUtil_action_to_1_read[0x50]; //000bb410
    undefined CSaveDataManager___set_next_SaveUtil_action_0_write[0xaa0]; //000bb460
    undefined CSaveDataManager___init_bbsave[0x1490]; //000bbf00
    undefined CSaveDataManager___copy_bbsave_to_buffer[0x2e0]; //000bd390
    undefined ReplayList___remove_at_index[0x30]; //000bd670
    undefined ReplayList___find_free_index_[0xa0]; //000bd6a0
    undefined ReplayList___get_replay_by_index[0x20]; //000bd740
    undefined ReplayList___get_count[0x10]; //000bd760
    undefined ReplayList__init[0x330]; //000bd770
    undefined ReplayList__sort[0x1c90]; //000bdaa0
    undefined _play_wav[0x5e0]; //000bf730
    undefined _call_DAT_00623630_0___gtvftable_3__Music[0x30]; //000bfd10
    undefined _call_DAT_00623630_0___gtvftable_3__BgSe[0x30]; //000bfd40
    undefined _call_DAT_00623630_0___gtvftable_3__Se[0x30]; //000bfd70
    undefined _call_DAT_00623630_0___gtvftable_3__SVoice[0x30]; //000bfda0
    undefined _call_DAT_00623630_0___gtvftable_3__Voice[0xca0]; //000bfdd0
    undefined _calls_DAT_00623630_item_methods_with_FPAC_items[0x4b0]; //000c0a70
    undefined _get_DAT_00abfff8[0x310]; //000c0f20
    undefined AA_SystemManager__ctor[0x50]; //000c1230
    undefined AA_SystemManager__dtor[0x10]; //000c1280
    undefined AA_SystemManager__dtor_[0xf0]; //000c1290
    undefined AASTEAM_SystemManager___get_input_controller_by_index[0x10]; //000c1380
    undefined AASTEAM_SystemManager___create_CSaveTask_and_CEventControlTask[0x210]; //000c1390
    undefined _return_1_1[0x10]; //000c15a0
    undefined GAME_CEventControlTaskFactory___create[0x6a0]; //000c15b0
    undefined _get_GAME_CTextEffectObject[0x1680]; //000c1c50
    undefined _get_GAMESTEAM_CVoiceChatManager[0x5ce0]; //000c32d0
    undefined GAME_CFadeTaskManager__init_[0x560]; //000c8fb0
    undefined _fade_out_[0x2a0]; //000c9510
    undefined GAME_CFadeTaskManager___start_fade_[0x590]; //000c97b0
    undefined GAME_CFadeTask___update_task[0x4c0]; //000c9d40
    undefined GAMEDX_CFadeTaskFactory___create[0x960]; //000ca200
    undefined ReplayList___copy_header[0x110]; //000cab60
    undefined SaveUtil__dtor[0x30]; //000cac70
    undefined SaveUtil__start_file_action[0x260]; //000caca0
    undefined SaveUtil___start_check[0x40]; //000caf00
    undefined SaveUtil___start_delete[0x40]; //000caf40
    undefined SaveUtil___start_action_3[0x40]; //000caf80
    undefined SaveUtil___start_read[0x40]; //000cafc0
    undefined SaveUtil___start_write[0x40]; //000cb000
    undefined SaveUtil__init[0x70]; //000cb040
    undefined inject_UploadReplayToEndpoint[0x80]; //000cb0b0
    undefined SaveUtil___format_filename[0x50]; //000cb130
    undefined ReplayList___copy_selected_header[0x20]; //000cb180
    undefined ReplayFileHeader___write_end_time[0x70]; //000cb1a0
    undefined SaveUtil__setup_data_to_write[0x110]; //000cb210
    undefined ReplayFileHeader___write_end_time_1[0x120]; //000cb320
    undefined SaveUtil___actually_delete_file[0x170]; //000cb440
    undefined SaveUtil___actually_write_file[0xd0]; //000cb5b0
    undefined SaveUtil__thread_start[0x10]; //000cb680
    undefined SaveUtil__thread_action[0x5bd0]; //000cb690
    undefined GAMESTEAM_BattleKeyControler__dtor[0x200]; //000d1260
    undefined AAJVS_SystemManager__ctor[0xf0]; //000d1460
    undefined AA_CRenderController__dtor[0x400]; //000d1550
    undefined GAMEJVS_CCreditTaskFactory___create[0x130]; //000d1950
    undefined CAdvCtrTaskFactory___create[0xe0]; //000d1a80
    undefined _assert_fail_and_exit_2[0x40]; //000d1b60
    undefined _assert_fail_log_1[0x420]; //000d1ba0
    undefined _battle_is_paused_1_[0x150]; //000d1fc0
    undefined SCENE_CAbyssUIFactory___create[0x70]; //000d2110
    undefined SCENE_CMatchResultFactory___create[0x80]; //000d2180
    undefined SCENE_TestRingCommandFactory___create[0x200]; //000d2200
    undefined _inc_frame_counter_[9]; //000d2400
    undefined inject_GetFrameCounter[0x197]; //000d2409
    undefined _get_BG_EffectManager[0x1560]; //000d25a0
    undefined AllianceModeEnemyData___load_from_FPAC[0x140]; //000d3b00
    undefined _get_AllianceModeEnemyData[0x9cc0]; //000d3c40
    undefined _ctor_for_DAT_013b02f0_item_1[0x40]; //000dd900
    undefined _empty_func_1[0x80]; //000dd940
    undefined _is_Arcade_and_SPA_Posing_[0x90]; //000dd9c0
    undefined _maybe_get_training_menu_default_values_1_[0x740]; //000dda50
    undefined _is_Arcade_and_check_active_player_data_0xc[0x1d80]; //000de190
    undefined _get_DAT_00ac3540[0x1700]; //000dff10
    undefined _get_menu_item_configs_by_name_00ade548[0x2280]; //000e1610
    undefined _get_DAT_00adecc0[0x1d50]; //000e3890
    undefined SCENE_CBattle__run_frames_online_[0xa0]; //000e55e0
    undefined _get_game_Stat_PCoinManager[0x70]; //000e5680
    undefined maybe_network_stuff_init_FUN_008356f0_[0x370]; //000e56f0
    undefined maybe_manager_static_init_FUN_00d65a60[0x6d0]; //000e5a60
    undefined SCENE_CBattle__run_1_frame_online_1_[0x31d6]; //000e6130
    undefined inject_GetMusicSelectAddr[0x143a]; //000e9306
    undefined _get_DAT_00ae0338[0x100]; //000ea740
    undefined _get_DAT_00ae21c8[0x19e0]; //000ea840
    undefined CMenuItemManager__init[0x510]; //000ec220
    undefined _get_DAT_00b452e0[0x3a0]; //000ec730
    undefined _get_DAT_00b48598[0xe10]; //000ecad0
    undefined _get_DAT_00b48718[0xfe0]; //000ed8e0
    undefined _get_DAT_00b4b190[0x530]; //000ee8c0
    undefined _get_DAT_00b70394[0x750]; //000eedf0
    undefined _get_DAT_00b70400[0x9d0]; //000ef540
    undefined _get_DAT_00b70768[0x6b0]; //000eff10
    undefined _get_DAT_00b73250_ABY_BOOK__03d[0x1c50]; //000f05c0
    undefined _get_DAT_00bf6af8[0x10]; //000f2210
    undefined _get_DAT_00c0b500[0x10]; //000f2220
    undefined _get_DAT_00c59b38[0x10]; //000f2230
    undefined _get_DAT_00b75530[0x10]; //000f2240
    undefined _get_DAT_00b7fa50[0x10]; //000f2250
    undefined _get_DAT_00c01fc8[0x10]; //000f2260
    undefined _get_DAT_00cd0bd8[0x10]; //000f2270
    undefined _get_DAT_00bfed58[0x10]; //000f2280
    undefined _get_DAT_00c08230[0x10]; //000f2290
    undefined _get_DAT_00d47c78[0x10]; //000f22a0
    undefined _get_DAT_00c0d770[0x5290]; //000f22b0
    undefined _get_DAT_00d47e7e[0xee0]; //000f7540
    undefined _get_DAT_00d47ee4[0x8960]; //000f8420
    undefined _get_DAT_00d47f68[0xb30]; //00100d80
    undefined _get_DAT_00d49648[0x640]; //001018b0
    undefined _get_DAT_00d49e78[0xc40]; //00101ef0
    undefined _get_DAT_00d590dc[0x360]; //00102b30
    undefined _get_DAT_00d59128[0x1350]; //00102e90
    undefined _get_DAT_00d5a678[0x11d0]; //001041e0
    undefined SCENE_CGallery___dtor[0x1d0]; //001053b0
    undefined SCENE_CGallery___init_8_1[0x1070]; //00105580
    undefined SCENE_CGallery___set_next_scene[0x200]; //001065f0
    undefined SCENE_CGallery___update_scene[0x160]; //001067f0
    undefined _get_DAT_00d5b138_by_index[0xa0]; //00106950
    undefined SCENE_CGallery___init_8_2[0xc0]; //001069f0
    undefined _return_1_2[0x10]; //00106ab0
    undefined SCENE_CGallery___init_7[0x200]; //00106ac0
    undefined SCENE_CGallery___init_3[0x7a0]; //00106cc0
    undefined SCENE_CBase__inc_GameSceneState[0x120]; //00107460
    undefined _get_DAT_00d5b1a0[0x8dc0]; //00107580
    undefined _get_DAT_00d5b258[0x39]; //00110340
    undefined inject_P2ReadNetplay[0x4267]; //00110379
    undefined _get_DAT_00d61670[0x540]; //001145e0
    undefined _get_DAT_00d9a4b0[0xb0]; //00114b20
    undefined _get_DAT_00d9a4d8[0x1e50]; //00114bd0
    undefined _return_0_1[0xd0]; //00116a20
    undefined _return_0_2[0x4de0]; //00116af0
    undefined _get_DAT_00d9a540_lobby_[0x7640]; //0011b8d0
    undefined _get_DAT_00da2378[0x2a60]; //00122f10
    undefined _get_CPlayerBattleInformation[0x70]; //00125970
    undefined _get_CPlayerRoomMember[0x5ad]; //001259e0
    undefined inject_GetFFAMatchThisPlayerIndex[0x2cc3]; //00125f8d
    undefined _get_DAT_00da4d88[0x19c0]; //00128c50
    undefined _get_DAT_00da5c18[0xd60]; //0012a610
    undefined _get_DAT_00da5c78[0x4f80]; //0012b370
    undefined CNetworkUILobbyRegionSelect__select_[0x60]; //001302f0
    undefined _get_CNetworkLobbyWorld[0x80]; //00130350
    undefined _get_CNetworkUILobbyRegionSelect[0x1c0]; //001303d0
    undefined CNetworkUIPlayerMatchCreate___clear[0xb4c0]; //00130590
    undefined CNetworkUIPlayerMatchCreate___dtor[0x2060]; //0013ba50
    undefined get_CNetworkUIPlayerMatchCreate[0xa0]; //0013dab0
    undefined _get_CNetworkUIPlayerMatchSearch[0xb0]; //0013db50
    undefined CNetworkUIPlayerMatchCreate___init[0x28d0]; //0013dc00
    undefined _get_CNetworkUIPlayerMatchTopMenu[0x590]; //001404d0
    undefined CNetworkUIRankMatchTopMenu__dtor[0xe80]; //00140a60
    undefined _get_CNetworkUIRankMatchTopMenu[0x90]; //001418e0
    undefined _get_CNetworkUIRankParam[0x830]; //00141970
    undefined CNetworkUIRankMatchTopMenu__update_[0x27d0]; //001421a0
    undefined _get_CNetworkUIRankMatchCharSele[0xd0]; //00144970
    undefined CNetworkUIRankMatchCharSele__init[0x65a0]; //00144a40
    undefined SCENE_CDcc___init_8_1[0x460]; //0014afe0
    undefined SCENE_CDcc___set_next_scene[0x50]; //0014b440
    undefined SCENE_CDcc___update_scene[0x80]; //0014b490
    undefined SCENE_CDcc___init_8_2[0xb0]; //0014b510
    undefined AA_CPaletteFactory_Custom___create[0x80]; //0014b5c0
    undefined SCENE_CDcc___init_3[0xdc0]; //0014b640
    undefined SCENE_PreTeamBattle___init_8_1[0x220]; //0014c400
    undefined SCENE_PreTeamBattle___set_next_scene[0xc0]; //0014c620
    undefined SCENE_PreTeamBattle___update_scene[0x410]; //0014c6e0
    undefined SCENE_PreTeamBattle___init_7[0x30]; //0014caf0
    undefined SCENE_PreTeamBattle___init_3[0x2bb0]; //0014cb20
    undefined SCENE_CBase__ctor[0xd0]; //0014f6d0
    undefined SCENE_CBase__dtor_1[0xb0]; //0014f7a0
    undefined SCENE_CBase__dtor[0x160]; //0014f850
    undefined SCENE_CBase__update_task[0x173]; //0014f9b0
    undefined inject_GetIsHUDHidden[0x1cd]; //0014fb23
    undefined _destroy_many_static_objects_1[0x1d0]; //0014fcf0
    undefined _destroy_many_static_objects[0x20]; //0014fec0
    undefined AAWin_CFileReader_Thread___get_file_size[0x50]; //0014fee0
    undefined FPAC___set_up_reading[0x560]; //0014ff30
    undefined CSceneController__ctor[0x70]; //00150490
    undefined CSceneController__dtor[0xa0]; //00150500
    undefined CSceneController___update_1[0x750]; //001505a0
    undefined CSceneController__add_task_for_GameScene[0x240]; //00150cf0
    undefined CSceneController__update_task[0xb0]; //00150f30
    undefined SCENE_CBattleFactory___create[0x70]; //00150fe0
    undefined SCENE_CCharSelectFactory___create[0x80]; //00151050
    undefined SCENE_CDccFactory___create[0x80]; //001510d0
    undefined SCENE_CEndingFactory___create[0x80]; //00151150
    undefined SCENE_CExtraFactory___create[0x80]; //001511d0
    undefined SCENE_CGalleryFactory___create[0x80]; //00151250
    undefined SCENE_CLegionFactory___create[0x80]; //001512d0
    undefined SCENE_CLibraryModeFactory___create[0x70]; //00151350
    undefined SCENE_CLobbyFactory___create[0x80]; //001513c0
    undefined SCENE_CMainMenuEXFactory___create[0x80]; //00151440
    undefined SCENE_CModeSelectFactory___create[0x80]; //001514c0
    undefined SCENE_CPreTeamBattleFactory___create[0x80]; //00151540
    undefined SCENE_CProfileFactory___create[0x80]; //001515c0
    undefined SCENE_CRankingFactory___create[0x80]; //00151640
    undefined SCENE_CReplayTheaterFactory___create[0x80]; //001516c0
    undefined SCENE_CResultFactory___create[0x70]; //00151740
    undefined SCENE_CStaffRollFactory___create[0x80]; //001517b0
    undefined SCENE_CStageInfoFactory___create[0x80]; //00151830
    undefined SCENE_CStorySelectFactory___create[0x80]; //001518b0
    undefined SCENE_CTestMyRoomFactory___create[0x80]; //00151930
    undefined SCENE_CTutorialUIFactory___create[0x70]; //001519b0
    undefined SCENE_CVSInfoFactory___create[0x80]; //00151a20
    undefined SCENE_CWarningFactory___create[0x70]; //00151aa0
    undefined SCENE_CWinnerFactory___create[0xe0]; //00151b10
    undefined BATTLE_CBGManager__ctor[0xd60]; //00151bf0
    undefined get_BATTLE_CBGManager[0xee0]; //00152950
    undefined flip_input[0x80]; //00153830
    undefined _reset_current_inputs[0x1f0]; //001538b0
    undefined BATTLE_CLoadTask__ctor[0x180]; //00153aa0
    undefined BATTLE_CLoadTask__dtor[0xd0]; //00153c20
    undefined BATTLE_CLoadTask___dtor[0x30]; //00153cf0
    undefined FPACBuffers___free_battle_buffers[0x40]; //00153d20
    undefined BATTLE_CLoadTask___update_task[0x70]; //00153d60
    undefined BATTLE_CLoadTask___init_1[0x100]; //00153dd0
    undefined BATTLE_CLoadTask___init_1_0[0x1b50]; //00153ed0
    undefined BATTLE_CLoadTask___init_1_1[0x17d0]; //00155a20
    undefined BATTLE_CLoadTask___init_2[0x130]; //001571f0
    undefined _free_battle_buffers_1_[0x40]; //00157320
    undefined BATTLE_CLoadTask___set_state_1[0x3a0]; //00157360
    undefined BATTLE_CObjectManager__ctor[0x280]; //00157700
    undefined _hash_table_ctor_[0x9b0]; //00157980
    undefined BATTLE_CObjectManager__reset_[0x1bf]; //00158330
    undefined inject_GetCharObjPointers[0x357]; //001584ef
    undefined inject_MatchIntroStartsPlaying[0x1a]; //00158846
    undefined BATTLE_CObjectManager___init[0x1c0]; //00158860
    undefined BATTLE_CObjectManager__alloc_entity_list[0x1d4]; //00158a20
    undefined inject_GetEntityListAddr[0xfc]; //00158bf4
    undefined BATTLE_CObjectManager__update_entities[0x12f0]; //00158cf0
    undefined BATTLE_CObjectManager__update_inputs[0x142]; //00159fe0
    undefined inject_P2Input[0x70]; //0015a122
    undefined inject_P1Input[0x24e]; //0015a192
    undefined BATTLE_CObjectManager___update_inputs_online_[0x164]; //0015a3e0
    undefined inject_P1InputNetplayInput[0x174c]; //0015a544
    undefined BATTLE_CObjectManager__process_events[0x260]; //0015bc90
    undefined BATTLE_CObjectManager___find_entity_with_some_status_[0x650]; //0015bef0
    undefined get_BATTLE_CObjectManager[0x290]; //0015c540
    undefined BATTLE_CObjectManager___get_entity_for_player[0xc0]; //0015c7d0
    undefined OBJ_CCharBase___compare_1[0x150]; //0015c890
    undefined BATTLE_CObjectManager__dtor_[0x243]; //0015c9e0
    undefined inject_GetEntityListDeleteAddr[0xd6d]; //0015cc23
    undefined OBJ_CBase___set_status_to_2_or_3[0x30]; //0015d990
    undefined BATTLE_CObjectManager___request_event[0x370]; //0015d9c0
    undefined BATTLE_CObjectManager___build_src_hash_table[0xbb0]; //0015dd30
    undefined MatchInfo__ctor[0xb0]; //0015e8e0
    undefined MatchInfo___mark_winner[0x650]; //0015e990
    undefined MatchInfo___update_NotStarted[0x230]; //0015efe0
    undefined GAME_CETCManager___get_CETCObject_by_index[0x3b0]; //0015f210
    undefined MatchInfo___init[0xbf]; //0015f5c0
    undefined inject_GetMatchVariables[0x101]; //0015f67f
    undefined MatchInfo___update_Fight[0x1fa0]; //0015f780
    undefined MatchInfo___update[0x1420]; //00161720
    undefined MatchInfo___next_round_[0x830]; //00162b40
    undefined MatchInfo___update_WinLoseSign[0x4f0]; //00163370
    undefined MatchInfo___update_FinishSign[0xb80]; //00163860
    undefined MatchInfo___update_RebelActionRoundSign[0x1f0]; //001643e0
    undefined MatchInfo___update_state_6[0x3d0]; //001645d0
    undefined MatchInfo___update_state_1[0x730]; //001649a0
    undefined BATTLE_CScreenManager__ctor[0xed0]; //001650d0
    undefined get_BATTLE_CScreenManager[0x1a90]; //00165fa0
    undefined SCENE_CBattle__dtor_1[0x2d0]; //00167a30
    undefined SCENE_CBattle__dtor[0x30]; //00167d00
    undefined SCENE_CBattle___init_8_1[0x1160]; //00167d30
    undefined SCENE_CBattle__set_next_scene[0x2160]; //00168e90
    undefined SCENE_CBattle__update_scene[0x200]; //0016aff0
    undefined SCENE_CBattle__run_1_frame[0x660]; //0016b1f0
    undefined SCENE_CBattle___init_8_2[0x460]; //0016b850
    undefined SCENE_CBattle___state_11[0x330]; //0016bcb0
    undefined SCENE_CBattle__maybe_Build_Network_battle_init_4[0x3100]; //0016bfe0
    undefined SCENE_CBattle___init_5_and_7[0x80]; //0016f0e0
    undefined SCENE_CLoadTask_Factory___create[0x200]; //0016f160
    undefined SCENE_CBattle___init_3[0x1c0]; //0016f360
    undefined SCENE_CBattle___init_6[0x1300]; //0016f520
    undefined OBJ_CCharBase__facing_left_1__1[0x26d0]; //00170820
    undefined OBJ_CBase__ctor[0x840]; //00172ef0
    undefined OBJ_CBase__dtor[0x330]; //00173730
    undefined OBJ_CCharBase__maybe_move_range_check[0x310]; //00173a60
    undefined OBJ_CCharBase__get_closest_throw_range_distance[0x960]; //00173d70
    undefined OBJ_CBase___update_5[0x8257]; //001746d0
    undefined inject_ForceBloomOn[0x1f9]; //0017c927
    undefined OBJ_CCharBase__get_current_input[0x60]; //0017cb20
    undefined OBJ_CCharBase___check_hp_[0x67]; //0017cb80
    undefined inject_vampire_HealthDrain[0x15b9]; //0017cbe7
    undefined OBJ_CCharBase__get_position_x_for_check[0x50]; //0017e1a0
    undefined OBJ_CCharBase__get_position_x_for_check_1[0x10]; //0017e1f0
    undefined OBJ_CCharBase__get_position_y_for_check[0x180]; //0017e200
    undefined OBJ_CCharBase__get_BoundingX_or_BoundingFixX[0x3e0]; //0017e380
    undefined OBJ_CCharBase___check_hp_is_0[0x490]; //0017e760
    undefined OBJ_CBase__init_[0x820]; //0017ebf0
    undefined _assert_BB_Assert_and_exit[0x60]; //0017f410
    undefined OBJ_CBase__dtor_[0x26e0]; //0017f470
    undefined OBJ_CBase__maybe_BBscript_frame_loop[0x4b0]; //00181b50
    undefined OBJ_CCharBase___enterState[0x1f0]; //00182000
    undefined _build_hash_table_from_bbscript_src_[0x570]; //001821f0
    undefined OBJ_CBase___bbscript_line_loop[0x450]; //00182760
    undefined OBJ_CBase__BBScriptFuncExecute[0x85d0]; //00182bb0
    undefined OBJ_CBase___get_scr_hash_table_item[0xd0]; //0018b180
    undefined _bbscript_cmd_length[0x190]; //0018b250
    undefined _bbscript_move_forward_[0x60]; //0018b3e0
    undefined OBJ_CBase___bbscript_run_next_line_[0x2c0]; //0018b440
    undefined CHashNodeFactoryC32BYTEtoU32___create[0x5f0]; //0018b700
    undefined OBJ_CCharBase__maybe_bounds_check_calculator[0x1230]; //0018bcf0
    undefined OBJ_CBase___bbcmd_0__State[0x10]; //0018cf20
    undefined OBJ_CBase___bbcmd_23042_PassHitSignalToParentAlt[0x20]; //0018cf30
    undefined OBJ_CBase___bbcmd_1_endState[0x10]; //0018cf50
    undefined OBJ_CBase___bbcmd_23035_DoNotResetRotation[0x20]; //0018cf60
    undefined OBJ_CBase___bbcmd_23025_KeepBackground[0x20]; //0018cf80
    undefined OBJ_CBase___bbcmd_21_enterState[0x20]; //0018cfa0
    undefined OBJ_CBase___bbcmd_22_EnterStateIfNotInState[0x10]; //0018cfc0
    undefined OBJ_CBase___bbcmd_25_EnterStateIfNotInStateWithPriority[0x80]; //0018cfd0
    undefined OBJ_CBase___bbcmd_23_EnterState2IfInState1[0x60]; //0018d050
    undefined OBJ_CBase___bbcmd_24_EnterStateWithPriority[0x30]; //0018d0b0
    undefined OBJ_CBase___bbcmd_1030_AddAccelerationRelative[0x50]; //0018d0e0
    undefined OBJ_CBase___bbcmd_1031_AddAccelerationAbsolute[0x10]; //0018d130
    undefined OBJ_CBase___bbcmd_19012_AddAccelerationZ[0x10]; //0018d140
    undefined OBJ_CBase___bbcmd_2038_AddActionMark[0x10]; //0018d150
    undefined OBJ_CBase___bbcmd_22003_AddAirDashes[0x20]; //0018d160
    undefined OBJ_CBase___bbcmd_22001_AddAirJumps[0x20]; //0018d180
    undefined OBJ_CBase___bbcmd_3002_AddAlpha[0x10]; //0018d1a0
    undefined OBJ_CBase___bbcmd_3005_AddAlphaPerFrame[0x10]; //0018d1b0
    undefined OBJ_CBase___bbcmd_1073_AddAngle[0x20]; //0018d1c0
    undefined OBJ_CBase___bbcmd_1075_AddAnglePerFrame[0x10]; //0018d1e0
    undefined OBJ_CBase___bbcmd_2067_BarrierChange[0xc0]; //0018d1f0
    undefined OBJ_CBase___bbcmd_3020_AddBlue[0x10]; //0018d2b0
    undefined OBJ_CBase___bbcmd_3023_AddBluePerFrame[0x10]; //0018d2c0
    undefined OBJ_CBase___bbcmd_2043_AddCommonActionMark[0x10]; //0018d2d0
    undefined OBJ_CBase___bbcmd_1036_AddGravity[0x10]; //0018d2e0
    undefined OBJ_CBase___bbcmd_3014_AddGreen[0x10]; //0018d2f0
    undefined OBJ_CBase___bbcmd_3017_AddGreenPerFrame[0x10]; //0018d300
    undefined OBJ_CBase___bbcmd_23094_0018d310[0x10]; //0018d310
    undefined OBJ_CBase___bbcmd_23097_0018d320[0x10]; //0018d320
    undefined OBJ_CBase___bbcmd_23112_AddAbsoluteBlue[0x10]; //0018d330
    undefined OBJ_CBase___bbcmd_23115_AddAbsoluteBluePerFrame[0x10]; //0018d340
    undefined OBJ_CBase___bbcmd_23106_AddAbsoluteGreen[0x10]; //0018d350
    undefined OBJ_CBase___bbcmd_23109_AddAbsoluteGreenPerFrame[0x10]; //0018d360
    undefined OBJ_CBase___bbcmd_23100_AddAbsoluteRed[0x10]; //0018d370
    undefined OBJ_CBase___bbcmd_23103_AddAbsoluteRedPerFrame[0x10]; //0018d380
    undefined OBJ_CBase___bbcmd_23011_AddCombo[0x30]; //0018d390
    undefined OBJ_CBase___bbcmd_21000_AddToCurrentHP[0x20]; //0018d3c0
    undefined OBJ_CBase___bbcmd_21001_HPManipulationNoKill[0x60]; //0018d3e0
    undefined OBJ_CBase___bbcmd_1047_AddInertia[0x50]; //0018d440
    undefined OBJ_CBase___bbcmd_1048_AddInertiaOverride[0x10]; //0018d490
    undefined OBJ_CBase___bbcmd_23153_InitialPush[0x50]; //0018d4a0
    undefined OBJ_CBase___bbcmd_23149_0018d4f0[0xd0]; //0018d4f0
    undefined OBJ_CBase___bbcmd_1002_AddPosXRelative[0x50]; //0018d5c0
    undefined OBJ_CBase___bbcmd_1003_AddPosXAbsolute[0x10]; //0018d610
    undefined OBJ_CBase___bbcmd_1007_AddPosY[0x10]; //0018d620
    undefined OBJ_CBase___bbcmd_19001_AddPositionZ[0x10]; //0018d630
    undefined OBJ_CBase___bbcmd_18005_AddPrivateValue[0x30]; //0018d640
    undefined OBJ_CBase___bbcmd_3008_AddRed[0x10]; //0018d670
    undefined OBJ_CBase___bbcmd_3011_AddRedPerFrame[0x10]; //0018d680
    undefined OBJ_CBase___bbcmd_1097_AddScale[0x30]; //0018d690
    undefined OBJ_CBase___bbcmd_1100_AddScalePerFrame[0x30]; //0018d6c0
    undefined OBJ_CBase___bbcmd_1060_AddScaleXPerFrame[0x10]; //0018d6f0
    undefined OBJ_CBase___bbcmd_1068_AddScaleYPerFrame[0x10]; //0018d700
    undefined OBJ_CBase___bbcmd_1092_AddScaleZPerFrame[0x10]; //0018d710
    undefined OBJ_CBase___bbcmd_1057_AddScaleX[0x10]; //0018d720
    undefined OBJ_CBase___bbcmd_1065_AddScaleY[0x10]; //0018d730
    undefined OBJ_CBase___bbcmd_1089_AddScaleZ[0x10]; //0018d740
    undefined OBJ_CBase___bbcmd_1015_AddSpeedXRelative[0x50]; //0018d750
    undefined OBJ_CBase___bbcmd_1016_AddSpeedXAbsolute[0x10]; //0018d7a0
    undefined OBJ_CBase___bbcmd_1021_AddSpeedY[0x10]; //0018d7b0
    undefined OBJ_CBase___bbcmd_19006_AddSpeedZ[0x10]; //0018d7c0
    undefined OBJ_CBase___bbcmd_2058_HeatChange[0x40]; //0018d7d0
    undefined OBJ_CBase___bbcmd_2020_AddZVal[0x20]; //0018d810
    undefined OBJ_CBase___bbcmd_3029_Afterimage[0x30]; //0018d830
    undefined OBJ_CBase___bbcmd_3069_AfterImageBlendMode[0x10]; //0018d860
    undefined OBJ_CBase___bbcmd_3074_AfterimageFirstAddColor[0x30]; //0018d870
    undefined OBJ_CBase___bbcmd_3072_AfterimageFirstColor[0x30]; //0018d8a0
    undefined OBJ_CBase___bbcmd_3076_AfterimageFirstScale[0x10]; //0018d8d0
    undefined OBJ_CBase___bbcmd_3075_AfterimageLastAddColor[0x30]; //0018d8e0
    undefined OBJ_CBase___bbcmd_3073_AfterimageLastColor[0x30]; //0018d910
    undefined OBJ_CBase___bbcmd_3077_AfterimageLastScale[0x10]; //0018d940
    undefined OBJ_CBase___bbcmd_3071_AfterimageMaxAmount[0x10]; //0018d950
    undefined OBJ_CBase___bbcmd_3070_AfterimageInterval[0x10]; //0018d960
    undefined OBJ_CBase___bbcmd_3030_AfterimageType[0x10]; //0018d970
    undefined OBJ_CBase___bbcmd_3078_AfterimagePulse[0x10]; //0018d980
    undefined OBJ_CBase___bbcmd_23091_DisableHurtboxes[0x30]; //0018d990
    undefined OBJ_CBase___bbcmd_23157_PlayPlayAstralBGM[0x20]; //0018d9c0
    undefined OBJ_CBase___bbcmd_23088_AstralHeatCleanup[0x3b0]; //0018d9e0
    undefined OBJ_CBase___bbcmd_23147_EmptyHeat[0x20]; //0018dd90
    undefined OBJ_CBase___bbcmd_11059_MoveAttributeLevel[0x10]; //0018ddb0
    undefined OBJ_CBase___bbcmd_11058_MoveAttributes[0x40]; //0018ddc0
    undefined OBJ_CBase___bbcmd_11040_AirGuardCrush[0x40]; //0018de00
    undefined OBJ_CBase___bbcmd_11037_HitAirUnblockable[0x40]; //0018de40
    undefined OBJ_CBase___bbcmd_11029_AirBlockstun[0x10]; //0018de80
    undefined OBJ_CBase___bbcmd_9334_AirHitstunAnimation[0x10]; //0018de90
    undefined OBJ_CBase___bbcmd_9336_CHAirHitstunAnimation[0x10]; //0018dea0
    undefined OBJ_CBase___bbcmd_9337_CHAirHitstunAnimationReset[0x10]; //0018deb0
    undefined OBJ_CBase___bbcmd_9335_AirHitstunAnimationReset[0x10]; //0018dec0
    undefined OBJ_CBase___bbcmd_11046_PreventGroundedHit[0x10]; //0018ded0
    undefined OBJ_CBase___bbcmd_11094_Restand[0x30]; //0018dee0
    undefined OBJ_CBase___bbcmd_9166_AirUntechableTime[0x10]; //0018df10
    undefined OBJ_CBase___bbcmd_9168_CHAirUntechableTime[0x10]; //0018df20
    undefined OBJ_CBase___bbcmd_9169_CHAirUntechableTimeReset[0x10]; //0018df30
    undefined OBJ_CBase___bbcmd_9167_AirUntechableTimeReset[0x10]; //0018df40
    undefined OBJ_CBase___bbcmd_9000_DefaultAttack[0x30]; //0018df50
    undefined OBJ_CBase___bbcmd_11085_NextHitAstralOnly[0x30]; //0018df80
    undefined OBJ_CBase___bbcmd_11023_HitAnywhere[0x10]; //0018dfb0
    undefined OBJ_CBase___bbcmd_11052_BarrierDepleteOnHit[0x10]; //0018dfc0
    undefined OBJ_CBase___bbcmd_11073_KeepBounceGravity[0x10]; //0018dfd0
    undefined OBJ_CBase___bbcmd_9118_BouncePercentage[0x10]; //0018dfe0
    undefined OBJ_CBase___bbcmd_9120_CHBouncePercentage[0x10]; //0018dff0
    undefined OBJ_CBase___bbcmd_9121_CHBouncePercentageReset[0x10]; //0018e000
    undefined OBJ_CBase___bbcmd_9119_BouncePercentageReset[0x10]; //0018e010
    undefined OBJ_CBase___bbcmd_9106_WallbounceStrength[0x10]; //0018e020
    undefined OBJ_CBase___bbcmd_9108_CHWallbounceReboundTime[0x10]; //0018e030
    undefined OBJ_CBase___bbcmd_9109_CHWallbounceReboundTimeReset[0x10]; //0018e040
    undefined OBJ_CBase___bbcmd_9107_WallbounceReboundTimeReset[0x10]; //0018e050
    undefined OBJ_CBase___bbcmd_9358_AirHitstunAfterWallbounce[0x10]; //0018e060
    undefined OBJ_CBase___bbcmd_9360_CHAirHitstunAfterWallbounce[0x10]; //0018e070
    undefined OBJ_CBase___bbcmd_9361_CHAirHitstunAfterWallbounceReset[0x10]; //0018e080
    undefined OBJ_CBase___bbcmd_9359_AirHitstunAfterWallbounceReset[0x10]; //0018e090
    undefined OBJ_CBase___bbcmd_9238_ComboCorrectionPoint[0x10]; //0018e0a0
    undefined OBJ_CBase___bbcmd_9240_CHComboCorrectionPoint[0x10]; //0018e0b0
    undefined OBJ_CBase___bbcmd_9241_CHComboCorrectionPointReset[0x10]; //0018e0c0
    undefined OBJ_CBase___bbcmd_9239_ComboCorrectionPointReset[0x10]; //0018e0d0
    undefined OBJ_CBase___bbcmd_11090_BonusProration[0x10]; //0018e0e0
    undefined OBJ_CBase___bbcmd_9274_AttackP1[0x20]; //0018e0f0
    undefined OBJ_CBase___bbcmd_9276_CHAttackP1[0x10]; //0018e110
    undefined OBJ_CBase___bbcmd_9277_CHResetAttackP1[0x10]; //0018e120
    undefined OBJ_CBase___bbcmd_9275_ResetAttackP1[0x10]; //0018e130
    undefined OBJ_CBase___bbcmd_9286_AttackP2[0x10]; //0018e140
    undefined OBJ_CBase___bbcmd_9288_CHAttackP2[0x10]; //0018e150
    undefined OBJ_CBase___bbcmd_9289_CHResetAttackP2[0x10]; //0018e160
    undefined OBJ_CBase___bbcmd_11092_SingleProration[0x30]; //0018e170
    undefined OBJ_CBase___bbcmd_9287_ResetAttackP2[0x10]; //0018e1a0
    undefined OBJ_CBase___bbcmd_11089_SameMoveProration[0x30]; //0018e1b0
    undefined OBJ_CBase___bbcmd_11108_CHStateIfCHStart[0x30]; //0018e1e0
    undefined OBJ_CBase___bbcmd_11042_CounterHitSetting[0x10]; //0018e210
    undefined OBJ_CBase___bbcmd_9003_Damage[0x10]; //0018e220
    undefined OBJ_CBase___bbcmd_11048_LifestealOld[0x10]; //0018e230
    undefined OBJ_CBase___bbcmd_11050_DamageEffect[0x30]; //0018e240
    undefined OBJ_CBase___bbcmd_10000_DamageMultiplier[0x50]; //0018e270
    undefined OBJ_CBase___bbcmd_9004_ResetDamage[0x10]; //0018e2c0
    undefined OBJ_CBase___bbcmd_11087_GuardCrush[0x20]; //0018e2d0
    undefined OBJ_CBase___bbcmd_11056_AttackDirection[0x10]; //0018e2f0
    undefined OBJ_CBase___bbcmd_11099_FlipOnHit[0x30]; //0018e300
    undefined OBJ_CBase___bbcmd_11025_DoNotHitKnockedDownOpp[0x30]; //0018e330
    undefined OBJ_CBase___bbcmd_11097_Lifesteal[0x20]; //0018e360
    undefined OBJ_CBase___bbcmd_11098_0018e380[0x50]; //0018e380
    undefined OBJ_CBase___bbcmd_9018_UseElectricHitspark[0x10]; //0018e3d0
    undefined OBJ_CBase___bbcmd_9017_UseFireHitspark[0x10]; //0018e3e0
    undefined OBJ_CBase___bbcmd_9019_UseFreezeHitspark[0x10]; //0018e3f0
    undefined OBJ_CBase___bbcmd_11055_ReduceHitEffects[0x10]; //0018e400
    undefined OBJ_CBase___bbcmd_11057_HitsparkSize[0x10]; //0018e410
    undefined OBJ_CBase___bbcmd_9016_UseSlashHitspark[0x10]; //0018e420
    undefined OBJ_CBase___bbcmd_9020_UseStrongHitspark[0x10]; //0018e430
    undefined OBJ_CBase___bbcmd_9015_UsePunchHitspark[0x10]; //0018e440
    undefined OBJ_CBase___bbcmd_11083_NoAttackOffset[0x30]; //0018e450
    undefined OBJ_CBase___bbcmd_11001_EnemyHitstopAddition[0x30]; //0018e480
    undefined OBJ_CBase___bbcmd_11081_DollAttackAttributes[0x20]; //0018e4b0
    undefined OBJ_CBase___bbcmd_11074_ExtraHitParameters[0x50]; //0018e4d0
    undefined OBJ_CBase___bbcmd_11103_OppState[0x20]; //0018e520
    undefined OBJ_CBase___bbcmd_11088_FatalCounter[0x30]; //0018e540
    undefined OBJ_CBase___bbcmd_11071_FinishBG[0x10]; //0018e570
    undefined OBJ_CBase___bbcmd_11091_MinimumDamage[0x10]; //0018e580
    undefined OBJ_CBase___bbcmd_11101_DisableAirTech[0x10]; //0018e590
    undefined OBJ_CBase___bbcmd_9250_FreezeCount[0x10]; //0018e5a0
    undefined OBJ_CBase___bbcmd_9252_CHFreezeCount[0x10]; //0018e5b0
    undefined OBJ_CBase___bbcmd_9253_CHFreezeCountReset[0x10]; //0018e5c0
    undefined OBJ_CBase___bbcmd_9251_FreezeCountReset[0x10]; //0018e5d0
    undefined OBJ_CBase___bbcmd_9262_FreezeTime[0x30]; //0018e5e0
    undefined OBJ_CBase___bbcmd_9264_CHFreezeTime[0x30]; //0018e610
    undefined OBJ_CBase___bbcmd_9265_CHFreezeTimeReset[0x10]; //0018e640
    undefined OBJ_CBase___bbcmd_9263_FreezeTimeReset[0x10]; //0018e650
    undefined OBJ_CBase___bbcmd_9094_GravityOnHit[0x10]; //0018e660
    undefined OBJ_CBase___bbcmd_9096_CHGravity[0x10]; //0018e670
    undefined OBJ_CBase___bbcmd_9097_ResetCHGravity[0x10]; //0018e680
    undefined OBJ_CBase___bbcmd_9095_ResetGravity[0x10]; //0018e690
    undefined OBJ_CBase___bbcmd_11030_GuardCrushHitstun[0x10]; //0018e6a0
    undefined OBJ_CBase___bbcmd_11051_GuardEffect[0x30]; //0018e6b0
    undefined OBJ_CBase___bbcmd_11028_Blockstun[0x10]; //0018e6e0
    undefined OBJ_CBase___bbcmd_9218_BounceOffWall[0x30]; //0018e6f0
    undefined OBJ_CBase___bbcmd_9219_CHApplyGroundPushbackOnAirHit[0x30]; //0018e720
    undefined OBJ_CBase___bbcmd_9214_PushbackX[0x10]; //0018e750
    undefined OBJ_CBase___bbcmd_9216_CHPushbackX[0x10]; //0018e760
    undefined OBJ_CBase___bbcmd_9217_CHResetPushbackX[0x10]; //0018e770
    undefined OBJ_CBase___bbcmd_9215_ResetPushbackX[0x10]; //0018e780
    undefined OBJ_CBase___bbcmd_11078_SingleComboCorrection[0x20]; //0018e790
    undefined OBJ_CBase___bbcmd_11107_OnlyHitIfHitstun[0x30]; //0018e7b0
    undefined OBJ_CBase___bbcmd_11072_OppPositionOnHit[0x30]; //0018e7e0
    undefined OBJ_CBase___bbcmd_11000_Hitstop[0x10]; //0018e810
    undefined OBJ_CBase___bbcmd_11044_IgnoreBurst[0x10]; //0018e820
    undefined OBJ_CBase___bbcmd_11063_PreventBlocking[0x10]; //0018e830
    undefined OBJ_CBase___bbcmd_11062_OppMovementUnlock[0x10]; //0018e840
    undefined OBJ_CBase___bbcmd_11026_BurstAttribute[0x10]; //0018e850
    undefined OBJ_CBase___bbcmd_11024_ThrowAttack[0x10]; //0018e860
    undefined OBJ_CBase___bbcmd_11066_OnlyHitPlayer[0x10]; //0018e870
    undefined OBJ_CBase___bbcmd_11067_DisableOppPushCollision[0x10]; //0018e880
    undefined OBJ_CBase___bbcmd_11079_IgnoreExternalForce[0x20]; //0018e890
    undefined OBJ_CBase___bbcmd_11049_ChipLifestealOld[0x10]; //0018e8b0
    undefined OBJ_CBase___bbcmd_11031_ChipPercentage[0x10]; //0018e8c0
    undefined OBJ_CBase___bbcmd_11064_DefeatOpponentBehavior[0x10]; //0018e8d0
    undefined OBJ_CBase___bbcmd_9190_GroundBounce[0x10]; //0018e8e0
    undefined OBJ_CBase___bbcmd_9192_CHGroundBounce[0x10]; //0018e8f0
    undefined OBJ_CBase___bbcmd_9193_CHGroundBounceReset[0x10]; //0018e900
    undefined OBJ_CBase___bbcmd_9191_GroundBounceReset[0x10]; //0018e910
    undefined OBJ_CBase___bbcmd_9322_GroundedHitstunAnimation[0x10]; //0018e920
    undefined OBJ_CBase___bbcmd_9324_CHGroundedHitstunAnimation[0x10]; //0018e930
    undefined OBJ_CBase___bbcmd_9325_CHResetGroundedHitstunAnimation[0x10]; //0018e940
    undefined OBJ_CBase___bbcmd_9323_ResetGroundedHitstunAnimation[0x10]; //0018e950
    undefined OBJ_CBase___bbcmd_11045_PreventAirborneHit[0x10]; //0018e960
    undefined OBJ_CBase___bbcmd_9154_Hitstun[0x10]; //0018e970
    undefined OBJ_CBase___bbcmd_9156_CHHitstun[0x10]; //0018e980
    undefined OBJ_CBase___bbcmd_9157_CHHitstunReset[0x10]; //0018e990
    undefined OBJ_CBase___bbcmd_9155_HitstunReset[0x10]; //0018e9a0
    undefined OBJ_CBase___bbcmd_9002_AttackLevel_[0x30]; //0018e9b0
    undefined OBJ_CBase___bbcmd_11047_GotoStateIfThrow[0x20]; //0018e9e0
    undefined OBJ_CBase___bbcmd_11076_ThrowRejectType[0x10]; //0018ea00
    undefined OBJ_CBase___bbcmd_11002_ThrowTechWindow[0x10]; //0018ea10
    undefined OBJ_CBase___bbcmd_11038_LowGuardCrush[0x40]; //0018ea20
    undefined OBJ_CBase___bbcmd_11035_HitLow[0x40]; //0018ea60
    undefined OBJ_CBase___bbcmd_11053_AddMagnetism[0x10]; //0018eaa0
    undefined OBJ_CBase___bbcmd_11077_IgnoreComboCorrection[0x20]; //0018eab0
    undefined OBJ_CBase___bbcmd_11039_MidGuardCrush[0x40]; //0018ead0
    undefined OBJ_CBase___bbcmd_11036_HitOverhead[0x40]; //0018eb10
    undefined OBJ_CBase___bbcmd_11095_MultihitCounterhit[0x30]; //0018eb50
    undefined OBJ_CBase___bbcmd_11069_DamageFromStateOnly[0x20]; //0018eb80
    undefined OBJ_CBase___bbcmd_11086_NotDownedAttribute[0x30]; //0018eba0
    undefined OBJ_CBase___bbcmd_11034_StrikeProjectileLevel[0x10]; //0018ebd0
    undefined OBJ_CBase___bbcmd_11061_CrouchWhiff[0x10]; //0018ebe0
    undefined OBJ_CBase___bbcmd_11104_EndAttackAfterFirstHit[0x30]; //0018ebf0
    undefined OBJ_CBase___bbcmd_11082_LockOpponentState[0x20]; //0018ec20
    undefined OBJ_CBase___bbcmd_11070_DisableNonLethalCombo[0x10]; //0018ec40
    undefined OBJ_CBase___bbcmd_11109_OnlyHitDuringCombo[0x10]; //0018ec50
    undefined OBJ_CBase___bbcmd_11084_PassByArmor[0x30]; //0018ec60
    undefined OBJ_CBase___bbcmd_11096_Curse[0x20]; //0018ec90
    undefined OBJ_CBase___bbcmd_11105_0018ecb0[0x10]; //0018ecb0
    undefined OBJ_CBase___bbcmd_10001_CopyAttackValues[0xf0]; //0018ecc0
    undefined OBJ_CBase___bbcmd_11100_IgnoreComboRate[0x30]; //0018edb0
    undefined OBJ_CBase___bbcmd_11075_Poison[0x20]; //0018ede0
    undefined OBJ_CBase___bbcmd_11054_RangeCheck[0x10]; //0018ee00
    undefined OBJ_CBase___bbcmd_11102_WallbounceOnAllHits[0x30]; //0018ee10
    undefined OBJ_CBase___bbcmd_11032_DistanceCheck[0x30]; //0018ee40
    undefined OBJ_CBase___bbcmd_11033_ProjectileLevel[0x10]; //0018ee70
    undefined OBJ_CBase___bbcmd_11080_ShutUp[0x20]; //0018ee80
    undefined OBJ_CBase___bbcmd_11065_EnableEmergencyTechAirHit[0x10]; //0018eea0
    undefined OBJ_CBase___bbcmd_9310_HardKnockdown[0x10]; //0018eeb0
    undefined OBJ_CBase___bbcmd_9312_CHHardKnockdown[0x10]; //0018eec0
    undefined OBJ_CBase___bbcmd_9313_CHHardKnockdownReset[0x10]; //0018eed0
    undefined OBJ_CBase___bbcmd_9311_HardKnockdownReset[0x10]; //0018eee0
    undefined OBJ_CBase___bbcmd_9202_Floorslide[0x10]; //0018eef0
    undefined OBJ_CBase___bbcmd_9204_CHFloorslide[0x10]; //0018ef00
    undefined OBJ_CBase___bbcmd_9205_CHFloorslideReset[0x10]; //0018ef10
    undefined OBJ_CBase___bbcmd_9203_FloorslideReset[0x10]; //0018ef20
    undefined OBJ_CBase___bbcmd_9130_Stagger[0x10]; //0018ef30
    undefined OBJ_CBase___bbcmd_9132_CHStagger[0x10]; //0018ef40
    undefined OBJ_CBase___bbcmd_9133_CHStaggerReset[0x10]; //0018ef50
    undefined OBJ_CBase___bbcmd_9131_StaggerReset[0x10]; //0018ef60
    undefined OBJ_CBase___bbcmd_9142_Crumple[0x10]; //0018ef70
    undefined OBJ_CBase___bbcmd_9144_CHCrumple[0x10]; //0018ef80
    undefined OBJ_CBase___bbcmd_9145_CHCrumpleReset[0x10]; //0018ef90
    undefined OBJ_CBase___bbcmd_9143_CrumpleReset[0x10]; //0018efa0
    undefined OBJ_CBase___bbcmd_11043_HeatGainMultiplier[0x10]; //0018efb0
    undefined OBJ_CBase___bbcmd_11106_OppHeatGainMultiplier[0x10]; //0018efc0
    undefined OBJ_CBase___bbcmd_11068_IgnoreComboTime[0x20]; //0018efd0
    undefined OBJ_CBase___bbcmd_9001_AttackType[0x30]; //0018eff0
    undefined OBJ_CBase___bbcmd_9070_AirPushbackX[0x10]; //0018f020
    undefined OBJ_CBase___bbcmd_9072_CHAirPushbackX[0x10]; //0018f030
    undefined OBJ_CBase___bbcmd_9073_ResetCHPushback[0x10]; //0018f040
    undefined OBJ_CBase___bbcmd_9071_ResetPushback[0x10]; //0018f050
    undefined OBJ_CBase___bbcmd_9082_AirPushbackY[0x10]; //0018f060
    undefined OBJ_CBase___bbcmd_9084_CHAirPushbackY[0x10]; //0018f070
    undefined OBJ_CBase___bbcmd_9085_ResetCHVerticalDrag[0x10]; //0018f080
    undefined OBJ_CBase___bbcmd_9083_ResetVerticalDrag[0x10]; //0018f090
    undefined OBJ_CBase___bbcmd_9178_Wallbounce[0x10]; //0018f0a0
    undefined OBJ_CBase___bbcmd_9180_CHWallbounce[0x10]; //0018f0b0
    undefined OBJ_CBase___bbcmd_9181_CHResetWallbounce[0x10]; //0018f0c0
    undefined OBJ_CBase___bbcmd_9179_ResetWallbounce[0x10]; //0018f0d0
    undefined OBJ_CBase___bbcmd_11093_StickToWall[0x30]; //0018f0e0
    undefined OBJ_CBase___bbcmd_9346_NonCornerWallbounce[0x10]; //0018f110
    undefined OBJ_CBase___bbcmd_9348_NonCornerCHWallbounce[0x10]; //0018f120
    undefined OBJ_CBase___bbcmd_9349_NonCornerCHWallbounceReset[0x10]; //0018f130
    undefined OBJ_CBase___bbcmd_9347_NonCornerWallbounceReset[0x10]; //0018f140
    undefined OBJ_CBase___bbcmd_9362_Wallstick[0x10]; //0018f150
    undefined OBJ_CBase___bbcmd_9363_CHWallstick[0x10]; //0018f160
    undefined OBJ_CBase___bbcmd_9364_WallstickDuration[0x10]; //0018f170
    undefined OBJ_CBase___bbcmd_9365_CHWallstickDuration[0x10]; //0018f180
    undefined OBJ_CBase___bbcmd_2001_EndAttack[0x30]; //0018f190
    undefined OBJ_CBase___bbcmd_23027_AttackOff[0x10]; //0018f1c0
    undefined OBJ_CBase___bbcmd_7009_RandomCommonVoiceline[0x140]; //0018f1d0
    undefined OBJ_CBase___bbcmd_2061_0018f310[0x30]; //0018f310
    undefined OBJ_CBase___bbcmd_23066_AutoHitSignalSending[0x20]; //0018f340
    undefined OBJ_CBase___bbcmd_23078_AutoHitStopSending[0x30]; //0018f360
    undefined OBJ_CBase___bbcmd_30014_EndTimestopEffect[0x140]; //0018f390
    undefined OBJ_CBase___bbcmd_30015_StopTimestopEffect[0x140]; //0018f4d0
    undefined OBJ_CBase___bbcmd_30013_TimestopEffect[0x170]; //0018f610
    undefined OBJ_CBase___bbcmd_13054_BeginCancelLevel[0x30]; //0018f780
    undefined OBJ_CBase___bbcmd_23180_TurnIntoTransparentShadow[0x30]; //0018f7b0
    undefined OBJ_CBase___bbcmd_23043_PhysicsPull[0x30]; //0018f7e0
    undefined OBJ_CBase___bbcmd_3033_BlendMode_Add[0x20]; //0018f810
    undefined OBJ_CBase___bbcmd_3034_BlendMode_HalfAdd[0x20]; //0018f830
    undefined OBJ_CBase___bbcmd_3032_BlendMode_Normal[0x20]; //0018f850
    undefined OBJ_CBase___bbcmd_3031_BlendMode_Off[0x10]; //0018f870
    undefined OBJ_CBase___bbcmd_3035_BlendModeSub[0x20]; //0018f880
    undefined OBJ_CBase___bbcmd_15019_CPURetreatPriority[0x20]; //0018f8a0
    undefined OBJ_CBase___bbcmd_15018_CPUJumpPriority[0x4b0]; //0018f8c0
    undefined OBJ_CBase___bbcmd_59_ObjectDistanceCheckByRefPoint[0x90]; //0018fd70
    undefined OBJ_CBase___bbcmd_23045_XOffsetOnScreenCheck[0xb0]; //0018fe00
    undefined OBJ_CBase___bbcmd_23046_YOffsetOnScreenCheck[0x70]; //0018feb0
    undefined OBJ_CBase___bbcmd_61_RNGFunction[0x100]; //0018ff20
    undefined OBJ_CBase___bbcmd_45_MultiplyBy[0x50]; //00190020
    undefined OBJ_CBase___bbcmd_20004_CameraControlInfinity[0x30]; //00190070
    undefined OBJ_CBase___bbcmd_20001_CameraFast[0x30]; //001900a0
    undefined OBJ_CBase___bbcmd_20005_CameraForAstralHeat[0x20]; //001900d0
    undefined OBJ_CBase___bbcmd_20007_CameraLookAtEnemy[0x30]; //001900f0
    undefined OBJ_CBase___bbcmd_20000_CameraControlEnable[0x30]; //00190120
    undefined OBJ_CBase___bbcmd_20002_CameraNoCeiling[0x30]; //00190150
    undefined OBJ_CBase___bbcmd_20008_CameraBelowFloor[0x30]; //00190180
    undefined OBJ_CBase___bbcmd_20003_CameraNoScreenCollision[0x30]; //001901b0
    undefined OBJ_CBase___bbcmd_20009_CameraPosition[0x30]; //001901e0
    undefined OBJ_CBase___bbcmd_20011_CameraFollowTarget[0xb0]; //00190210
    undefined OBJ_CBase___bbcmd_20006_CameraWinnerControlStop[0x20]; //001902c0
    undefined OBJ_CBase___bbcmd_2_sprite[0x80]; //001902e0
    undefined OBJ_CBase___bbcmd_23183_SpriteCall[0xb0]; //00190360
    undefined OBJ_CBase___bbcmd_51_EndSprite[0x20]; //00190410
    undefined OBJ_CBase___bbcmd_5003_ThrowLock[0x30]; //00190430
    undefined OBJ_CBase___bbcmd_20_ReplaceSpriteIf[0x30]; //00190460
    undefined OBJ_CBase___bbcmd_23159_ChallengeModeMarker[0x30]; //00190490
    undefined OBJ_CBase___bbcmd_23148_CurrentStateCheck[0x80]; //001904c0
    undefined OBJ_CBase___bbcmd_68_StatusCheck[0x130]; //00190540
    undefined OBJ_CBase___bbcmd_46_CheckObjectPresence[0x40]; //00190670
    undefined OBJ_CBase___bbcmd_66_001906b0[0x100]; //001906b0
    undefined OBJ_CBase___bbcmd_64_001907b0[0x50]; //001907b0
    undefined OBJ_CBase___bbcmd_44_CheckCondition[0x40]; //00190800
    undefined OBJ_CBase___bbcmd_2065_00190840[0x50]; //00190840
    undefined OBJ_CBase___bbcmd_65_ObjectCounterhit[0x40]; //00190890
    undefined OBJ_CBase___bbcmd_23036_GuardpointNonProjectileCheck[0x30]; //001908d0
    undefined OBJ_CBase___bbcmd_23037_GuardpointProjetcileCheck[0x30]; //00190900
    undefined OBJ_CBase___bbcmd_50_CheckAttackButton[0x40]; //00190930
    undefined OBJ_CBase___bbcmd_23166_OppStateCheck[0xa0]; //00190970
    undefined OBJ_CBase___bbcmd_42_CharacterIDCheck[0x70]; //00190a10
    undefined OBJ_CBase___bbcmd_62_00190a80[0xf0]; //00190a80
    undefined OBJ_CBase___bbcmd_60_CharacterIDCheckNoUnlimited[0x90]; //00190b70
    undefined OBJ_CBase___bbcmd_63_CharacterIDCheckNoCostume[0x90]; //00190c00
    undefined OBJ_CBase___bbcmd_23167_00190c90[0x80]; //00190c90
    undefined OBJ_CBase___bbcmd_43_CheckInput[0x1e0]; //00190d10
    undefined OBJ_CBase___bbcmd_23146_IfObjectHitByStateLast[0x80]; //00190ef0
    undefined OBJ_CBase___bbcmd_67_GuardpointAttributeCheck[0x160]; //00190f70
    undefined OBJ_CBase___bbcmd_23177_TimerDepleted[0x40]; //001910d0
    undefined OBJ_CBase___bbcmd_23145_PreviousStateCheck[0x60]; //00191110
    undefined OBJ_CBase___bbcmd_23156_EnteredState[0x60]; //00191170
    undefined OBJ_CBase___bbcmd_23082_CheckStageName[0x70]; //001911d0
    undefined OBJ_CBase___bbcmd_17_ClearUpon[0x80]; //00191240
    undefined OBJ_CBase___bbcmd_23083_StopClock[0x80]; //001912c0
    undefined OBJ_CBase___bbcmd_23084_HudVisibility[0x30]; //00191340
    undefined OBJ_CBase___bbcmd_23132_00191370[0x10]; //00191370
    undefined OBJ_CBase___bbcmd_23081_ColorIsAdditive[0x30]; //00191380
    undefined OBJ_CBase___bbcmd_21008_ColorParent[0x30]; //001913b0
    undefined OBJ_CBase___bbcmd_23182_VoodooDamageMultiplier[0x10]; //001913e0
    undefined OBJ_CBase___bbcmd_23170_001913f0[0x40]; //001913f0
    undefined OBJ_CBase___bbcmd_7013_CommonDamageVoiceline[0xa0]; //00191430
    undefined OBJ_CBase___bbcmd_7000_CommonSE[0xa0]; //001914d0
    undefined OBJ_CBase___bbcmd_7011_SmartCommonVoicelines[0x20]; //00191570
    undefined OBJ_CBase___bbcmd_36_ControlObject[0x40]; //00191590
    undefined OBJ_CBase___bbcmd_35_EndControlObject[0x10]; //001915d0
    undefined OBJ_CBase___bbcmd_2064_001915e0[0x30]; //001915e0
    undefined OBJ_CBase___bbcmd_2060_Recovery[0x10]; //00191610
    undefined OBJ_CBase___bbcmd_4004_CreateCommonObject[0xa0]; //00191620
    undefined OBJ_CBase___bbcmd_4018_CreateDecal[0x60]; //001916c0
    undefined OBJ_CBase___bbcmd_4000_CreateObject[0xa0]; //00191720
    undefined OBJ_CBase___bbcmd_4001_CreateParticle[0x80]; //001917c0
    undefined OBJ_CBase___bbcmd_4045_CreateArgumentedParticle[0xa0]; //00191840
    undefined OBJ_CBase___bbcmd_4048_ParticleAngle[0x30]; //001918e0
    undefined OBJ_CBase___bbcmd_4055_00191910[0x40]; //00191910
    undefined OBJ_CBase___bbcmd_4046_ParticleColor[0x30]; //00191950
    undefined OBJ_CBase___bbcmd_4047_ParticleColorFromPalette[0x40]; //00191980
    undefined OBJ_CBase___bbcmd_4051_ParticleDirection[0x10]; //001919c0
    undefined OBJ_CBase___bbcmd_4050_ParticlePaletteIndex[0x40]; //001919d0
    undefined OBJ_CBase___bbcmd_4052_ParticleRandomAngle[0x30]; //00191a10
    undefined OBJ_CBase___bbcmd_4053_ParticleRandomAngleMinMax[0x50]; //00191a40
    undefined OBJ_CBase___bbcmd_4054_ParticleLayer[0x30]; //00191a90
    undefined OBJ_CBase___bbcmd_4049_ParticleScale[0x30]; //00191ac0
    undefined OBJ_CBase___bbcmd_23074_CtrlDirectionLeft[0x20]; //00191af0
    undefined OBJ_CBase___bbcmd_23072_CtrlDirectionReverse[0x20]; //00191b10
    undefined OBJ_CBase___bbcmd_23075_CtrlDirectionRight[0x20]; //00191b30
    undefined OBJ_CBase___bbcmd_23073_CtrlDirectionVsTarget[0x70]; //00191b50
    undefined OBJ_CBase___bbcmd_3053_UseCustomPalette[0xb0]; //00191bc0
    undefined OBJ_CBase___bbcmd_1055_DefaultToDamageGravity[0x10]; //00191c70
    undefined OBJ_CBase___bbcmd_7008_DamageVoiceline[0x130]; //00191c80
    undefined OBJ_CBase___bbcmd_14086_GroundHitOrBlockSpecialCancel[0x30]; //00191db0
    undefined OBJ_CBase___bbcmd_14087_GroundHitSpecialCancel[0x30]; //00191de0
    undefined OBJ_CBase___bbcmd_23031_DashKeepInputTime[0x80]; //00191e10
    undefined OBJ_CBase___bbcmd_9013_SubVoiceline2[0x30]; //00191e90
    undefined OBJ_CBase___bbcmd_22005_debugPrint[0x50]; //00191ec0
    undefined OBJ_CBase___bbcmd_14085_AttackOffCancel[0x10]; //00191f10
    undefined OBJ_CBase___bbcmd_2063_00191f20[0x20]; //00191f20
    undefined OBJ_CBase___bbcmd_13_DeleteObjectByID[0x30]; //00191f40
    undefined OBJ_CBase___bbcmd_26_DeleteObjectByName[0x70]; //00191f70
    undefined OBJ_CBase___bbcmd_18008_EndDemo[0x20]; //00191fe0
    undefined OBJ_CBase___bbcmd_23018_DemoEndOnVoiceEnd[0x30]; //00192000
    undefined OBJ_CBase___bbcmd_21011_DemoTimer[0x20]; //00192030
    undefined OBJ_CBase___bbcmd_2007_FaceLeft[0x20]; //00192050
    undefined OBJ_CBase___bbcmd_23023_CopyPlayerDirection[0x50]; //00192070
    undefined OBJ_CBase___bbcmd_2005_ReverseDirection[0x60]; //001920c0
    undefined OBJ_CBase___bbcmd_2008_FaceRight[0x20]; //00192120
    undefined OBJ_CBase___bbcmd_2006_FaceTarget[0x80]; //00192140
    undefined OBJ_CBase___bbcmd_3057_DistortionTexture[0x30]; //001921c0
    undefined OBJ_CBase___bbcmd_1044_DefaultGravity[0x10]; //001921f0
    undefined OBJ_CBase___bbcmd_14035_CSStylishRouting[0xa0]; //00192200
    undefined OBJ_CBase___bbcmd_14044_CSStylishButtonRegister[0xb0]; //001922a0
    undefined OBJ_CBase___bbcmd_14046_CSStylishSomething[0x40]; //00192350
    undefined OBJ_CBase___bbcmd_14043_00192390[0x30]; //00192390
    undefined OBJ_CBase___bbcmd_14045_001923c0[0x40]; //001923c0
    undefined OBJ_CBase___bbcmd_14047_00192400[0x30]; //00192400
    undefined OBJ_CBase___bbcmd_14067_00192430[0x20]; //00192430
    undefined OBJ_CBase___bbcmd_8009_AirDashEffects[0x110]; //00192450
    undefined OBJ_CBase___bbcmd_8004_AltKnockdownEffects[0xf0]; //00192560
    undefined OBJ_CBase___bbcmd_8007_PreDashEffects[0xb0]; //00192650
    undefined OBJ_CBase___bbcmd_8006_DashEffects[0xb0]; //00192700
    undefined OBJ_CBase___bbcmd_8010_SkidEffects[0xb0]; //001927b0
    undefined OBJ_CBase___bbcmd_8001_JumpEffects[0x200]; //00192860
    undefined OBJ_CBase___bbcmd_8002_JumpSoundEffects[0x90]; //00192a60
    undefined OBJ_CBase___bbcmd_8003_KnockdownEffects[0x110]; //00192af0
    undefined OBJ_CBase___bbcmd_8000_LandingEffects[0xc0]; //00192c00
    undefined OBJ_CBase___bbcmd_8005_RecoveryEffects[0x50]; //00192cc0
    undefined OBJ_CBase___bbcmd_8011_SFX_FOOTSTEP_[0x100]; //00192d10
    undefined OBJ_CBase___bbcmd_8008_WallbounceEffects[0x160]; //00192e10
    undefined OBJ_CBase___bbcmd_30007_EnableBloom[0xb0]; //00192f70
    undefined OBJ_CBase___bbcmd_30009_ApplyBloomToPaletteColor[0xc0]; //00193020
    undefined OBJ_CBase___bbcmd_30011_LimitBloomToPalette[0x80]; //001930e0
    undefined OBJ_CBase___bbcmd_30010_00193160[0x70]; //00193160
    undefined OBJ_CBase___bbcmd_30008_001931d0[0x70]; //001931d0
    undefined OBJ_CBase___bbcmd_30018_00193240[0x60]; //00193240
    undefined OBJ_CBase___bbcmd_13042_EndArakuneInvisibilityEffect[0x20]; //001932a0
    undefined OBJ_CBase___bbcmd_23150_OutlineFlash[0x20]; //001932c0
    undefined OBJ_CBase___bbcmd_23003_ResourceGauge[0x70]; //001932e0
    undefined OBJ_CBase___bbcmd_23007_ResourceBarColor[0x30]; //00193350
    undefined OBJ_CBase___bbcmd_23005_NumberGauge[0x40]; //00193380
    undefined OBJ_CBase___bbcmd_23008_ResourceBarVisibility2[0x30]; //001933c0
    undefined OBJ_CBase___bbcmd_23041_ResourceGaugeFlash[0x40]; //001933f0
    undefined OBJ_CBase___bbcmd_23010_ResourceBarFlashIfFull[0x40]; //00193430
    undefined OBJ_CBase___bbcmd_23006_ResourceBarIcon[0x30]; //00193470
    undefined OBJ_CBase___bbcmd_23039_ResourceItemAmount[0x40]; //001934a0
    undefined OBJ_CBase___bbcmd_23038_ResourceBarFullColor[0x30]; //001934e0
    undefined OBJ_CBase___bbcmd_23009_ResourceBarVisibility[0x40]; //00193510
    undefined OBJ_CBase___bbcmd_23040_BackupGaugeText[0x40]; //00193550
    undefined OBJ_CBase___bbcmd_23044_DisableResourceGaugeBar[0x40]; //00193590
    undefined OBJ_CBase___bbcmd_23047_ResourceGaugeX[0x40]; //001935d0
    undefined OBJ_CBase___bbcmd_23004_HideResourceGauge[0x40]; //00193610
    undefined OBJ_CBase___bbcmd_23144_Homing[0x80]; //00193650
    undefined OBJ_CBase___bbcmd_2040_FlagAddActionMark[0x10]; //001936d0
    undefined OBJ_CBase___bbcmd_2045_FlagAddCommonActionMark[0x10]; //001936e0
    undefined OBJ_CBase___bbcmd_2041_FlagDelActionMark[0x20]; //001936f0
    undefined OBJ_CBase___bbcmd_2046_FlagDelCommonActionMark[0x20]; //00193710
    undefined OBJ_CBase___bbcmd_23013_FloorCollision[0x30]; //00193730
    undefined OBJ_CBase___bbcmd_2049_CreateDecalOn[0x30]; //00193760
    undefined OBJ_CBase___bbcmd_13044_ForceBloomMaskOn[0x30]; //00193790
    undefined OBJ_CBase___bbcmd_23169_DisableInput[0x130]; //001937c0
    undefined OBJ_CBase___bbcmd_23162_DisableBackdash[0x30]; //001938f0
    undefined OBJ_CBase___bbcmd_23163_DisableOverdrive1[0x90]; //00193920
    undefined OBJ_CBase___bbcmd_23164_DisableCrouch[0x30]; //001939b0
    undefined OBJ_CBase___bbcmd_23161_DisableDash[0x30]; //001939e0
    undefined OBJ_CBase___bbcmd_23160_DisableBarrier[0x30]; //00193a10
    undefined OBJ_CBase___bbcmd_23168_DisableAllExceptWalk[0xf0]; //00193a40
    undefined OBJ_CBase___bbcmd_2047_ForceFloorReflectOff[0x30]; //00193b30
    undefined OBJ_CBase___bbcmd_2062_DepleteODIfActive[0xd0]; //00193b60
    undefined OBJ_CBase___bbcmd_2035_ForceShadowOff[0x30]; //00193c30
    undefined OBJ_CBase___bbcmd_23069_ForceShadowOn[0x30]; //00193c60
    undefined OBJ_CBase___bbcmd_10_CallSubroutine[0x1b0]; //00193c90
    undefined OBJ_CBase___bbcmd_58_TrainingModeSLOT[0x120]; //00193e40
    undefined OBJ_CBase___bbcmd_7_GotoLabel[0x30]; //00193f60
    undefined OBJ_CBase___bbcmd_18_GoToLabelIf[0x30]; //00193f90
    undefined OBJ_CBase___bbcmd_19_GoToLabelIfNot[0x30]; //00193fc0
    undefined OBJ_CBase___bbcmd_23124_PreventOppJump[0x30]; //00193ff0
    undefined OBJ_CBase___bbcmd_5001_EnemyThrowPosition[0x280]; //00194020
    undefined OBJ_CBase___bbcmd_23071_GuardSeal[0x30]; //001942a0
    undefined OBJ_CBase___bbcmd_23034_MugenTime[0x70]; //001942d0
    undefined OBJ_CBase___bbcmd_23121_00194340[0x10]; //00194340
    undefined OBJ_CBase___bbcmd_23086_HitBackReturnIgnore[0x30]; //00194350
    undefined OBJ_CBase___bbcmd_23079_HitBackReturnObject[0x30]; //00194380
    undefined OBJ_CBase___bbcmd_2070_TrackObjectToPrivatePoint[0x110]; //001943b0
    undefined OBJ_CBase___bbcmd_2071_TrackObjectByDistance[0x110]; //001944c0
    undefined OBJ_CBase___bbcmd_2072_TrackObjectBySpeed[0x120]; //001945d0
    undefined OBJ_CBase___bbcmd_4_if[0x20]; //001946f0
    undefined OBJ_CBase___bbcmd_54_ifnot[0x30]; //00194710
    undefined OBJ_CBase___bbcmd_21010_IgnoreFinishStop[0x20]; //00194740
    undefined OBJ_CBase___bbcmd_1052_IgnoreInertia[0x30]; //00194760
    undefined OBJ_CBase___bbcmd_21005_IgnoreTurn[0x30]; //00194790
    undefined OBJ_CBase___bbcmd_2054_IgnoreScreenfreeze[0x30]; //001947c0
    undefined OBJ_CBase___bbcmd_12037_AirBDashGravity[0x20]; //001947f0
    undefined OBJ_CBase___bbcmd_12039_AirDashBSpeed[0x20]; //00194810
    undefined OBJ_CBase___bbcmd_12035_AIRBDashDuration[0x20]; //00194830
    undefined OBJ_CBase___bbcmd_12013_AirDashCount[0x50]; //00194850
    undefined OBJ_CBase___bbcmd_12040_AirDashMinimumHeight[0x20]; //001948a0
    undefined OBJ_CBase___bbcmd_12036_AirFDashGravity[0x20]; //001948c0
    undefined OBJ_CBase___bbcmd_12038_AirDashFSpeed[0x20]; //001948e0
    undefined OBJ_CBase___bbcmd_12034_AirFDashDuration[0x20]; //00194900
    undefined OBJ_CBase___bbcmd_12012_DoubleJumpCount[0x20]; //00194920
    undefined OBJ_CBase___bbcmd_12028_JumpStart[0x20]; //00194940
    undefined OBJ_CBase___bbcmd_13040_ArakuneSpriteOverlay[0x80]; //00194960
    undefined OBJ_CBase___bbcmd_12027_DashBGravity[0x20]; //001949e0
    undefined OBJ_CBase___bbcmd_12025_DashBVelocity[0x20]; //00194a00
    undefined OBJ_CBase___bbcmd_12026_DashBHeight[0x20]; //00194a20
    undefined OBJ_CBase___bbcmd_12010_BackwardSuperJumpVelocity[0x20]; //00194a40
    undefined OBJ_CBase___bbcmd_12009_BackwardJumpVelocity[0x20]; //00194a60
    undefined OBJ_CBase___bbcmd_12001_WalkBSpeed[0x70]; //00194a80
    undefined OBJ_CBase___bbcmd_12019_CharacterID[0x40]; //00194af0
    undefined OBJ_CBase___bbcmd_12041_DefaultAttackP2Table[0x70]; //00194b30
    undefined OBJ_CBase___bbcmd_7010_CommonVoicelines[0x50]; //00194ba0
    undefined OBJ_CBase___bbcmd_12050_EnableDashBuffer[0x30]; //00194bf0
    undefined OBJ_CBase___bbcmd_12023_CharacterDamageMultiplier[0x20]; //00194c20
    undefined OBJ_CBase___bbcmd_12017_ComboRate[0x20]; //00194c40
    undefined OBJ_CBase___bbcmd_12029_DefaultZLine[0x20]; //00194c60
    undefined OBJ_CBase___bbcmd_12044_GuardPrimers[0x40]; //00194c80
    undefined OBJ_CBase___bbcmd_12045_SoulEaterMultiplier[0x110]; //00194cc0
    undefined OBJ_CBase___bbcmd_12056_TakemikazuchiIntroFlag[0x30]; //00194dd0
    undefined OBJ_CBase___bbcmd_12024_FootstepType[0x20]; //00194e00
    undefined OBJ_CBase___bbcmd_12014_GroundDashCount[0x20]; //00194e20
    undefined OBJ_CBase___bbcmd_12003_DashFAccel[0x20]; //00194e40
    undefined OBJ_CBase___bbcmd_12004_DashFMaxVelocity[0x20]; //00194e60
    undefined OBJ_CBase___bbcmd_12002_DashFInitialVelocity[0x20]; //00194e80
    undefined OBJ_CBase___bbcmd_12008_ForwardSuperJumpVelocity[0x20]; //00194ea0
    undefined OBJ_CBase___bbcmd_12007_ForwardJumpVelocity[0x20]; //00194ec0
    undefined OBJ_CBase___bbcmd_12000_WalkFSpeed[0x20]; //00194ee0
    undefined OBJ_CBase___bbcmd_12047_00194f00[0x30]; //00194f00
    undefined OBJ_CBase___bbcmd_12032_OldGuardCrushLevelTable[0x10]; //00194f30
    undefined OBJ_CBase___bbcmd_12054_00194f40[0x70]; //00194f40
    undefined OBJ_CBase___bbcmd_12006_SuperJumpYVelocity[0x20]; //00194fb0
    undefined OBJ_CBase___bbcmd_12042_LandPreJumpActionNormalAttackAvailable[0x20]; //00194fd0
    undefined OBJ_CBase___bbcmd_12015_Health[0x150]; //00194ff0
    undefined OBJ_CBase___bbcmd_12005_JumpYVelocity[0x20]; //00195140
    undefined OBJ_CBase___bbcmd_12049_00195160[0x30]; //00195160
    undefined OBJ_CBase___bbcmd_12048_EnableFallingAnimation[0x30]; //00195190
    undefined OBJ_CBase___bbcmd_12043_001951c0[0x20]; //001951c0
    undefined OBJ_CBase___bbcmd_9012_SetCommonSubVoiceline[0x50]; //001951e0
    undefined OBJ_CBase___bbcmd_12031_NegativePenaltyResistance[0x30]; //00195230
    undefined OBJ_CBase___bbcmd_12053_OverdriveDuration[0xb0]; //00195260
    undefined OBJ_CBase___bbcmd_12022_PushHeight[0x20]; //00195310
    undefined OBJ_CBase___bbcmd_12021_PushWidth[0x20]; //00195330
    undefined OBJ_CBase___bbcmd_12046_ExtendCollisionX[0x20]; //00195350
    undefined OBJ_CBase___bbcmd_23152_MagatanaGainMultiplier[0x20]; //00195370
    undefined OBJ_CBase___bbcmd_13039_FootstepSE[0x20]; //00195390
    undefined OBJ_CBase___bbcmd_12030_HeatGainFactor[0x20]; //001953b0
    undefined OBJ_CBase___bbcmd_12011_Gravity[0x20]; //001953d0
    undefined OBJ_CBase___bbcmd_2012_AttackDefaults_AstralheatProjectile[0x20]; //001953f0
    undefined OBJ_CBase___bbcmd_17012_AttackDefaults_SuccessThrow[0x260]; //00195410
    undefined OBJ_CBase___bbcmd_17011_AttackDefaults_Throw[0x360]; //00195670
    undefined OBJ_CBase___bbcmd_2022_AttackDefaults_ThrowProjectile[0x20]; //001959d0
    undefined OBJ_CBase___bbcmd_2009_AttackDefaults_Projectile[0x20]; //001959f0
    undefined OBJ_CBase___bbcmd_2010_AttackDefaults_SpecialProjectile[0x20]; //00195a10
    undefined OBJ_CBase___bbcmd_2011_AttackDefaults_SuperProjectile[0x1a0]; //00195a30
    undefined OBJ_CBase___bbcmd_23028_InstantProjectileInit[0x170]; //00195bd0
    undefined OBJ_CBase___bbcmd_28_IfUponThenEnterState[0x50]; //00195d40
    undefined OBJ_CBase___bbcmd_15_upon_[0x50]; //00195d90
    undefined OBJ_CBase___bbcmd_29_IfUponThenGotoLabel[0x40]; //00195de0
    undefined OBJ_CBase___bbcmd_30_UponArgument1[0x40]; //00195e20
    undefined OBJ_CBase___bbcmd_31_UponArgument2[0x40]; //00195e60
    undefined OBJ_CBase___bbcmd_23002_JumpInputLimit[0x20]; //00195ea0
    undefined OBJ_CBase___bbcmd_2057_AirDashMomentum[0x30]; //00195ec0
    undefined OBJ_CBase___bbcmd_23141_ExtendNonCornerWallbounce[0x30]; //00195ef0
    undefined OBJ_CBase___bbcmd_23089_HitsPerCall[0x70]; //00195f20
    undefined OBJ_CBase___bbcmd_23026_LandingHeight[0x10]; //00195f90
    undefined OBJ_CBase___bbcmd_22004_ForcedLandingRecovery[0x40]; //00195fa0
    undefined OBJ_CBase___bbcmd_21009_PaletteMapping[0x80]; //00195fe0
    undefined OBJ_CBase___bbcmd_30000_Create3DObject[0x40]; //00196060
    undefined OBJ_CBase___bbcmd_4016_LinkCommonPolyline[0x60]; //001960a0
    undefined OBJ_CBase___bbcmd_4014_LinkFloorDecal[0x50]; //00196100
    undefined OBJ_CBase___bbcmd_4003_LinkModel[0x530]; //00196150
    undefined OBJ_CBase___bbcmd_4015_DisableLinkModelPerspective[0x10]; //00196680
    undefined OBJ_CBase___bbcmd_21002_AEff3DEffectResetTime[0x10]; //00196690
    undefined OBJ_CBase___bbcmd_4025_LinkAlpha[0x20]; //001966a0
    undefined OBJ_CBase___bbcmd_4006_LinkObjectAngle[0x20]; //001966c0
    undefined OBJ_CBase___bbcmd_4010_RemoveOnLinkObjectStateChange[0x20]; //001966e0
    undefined OBJ_CBase___bbcmd_4020_LinkObjectCollision[0x20]; //00196700
    undefined OBJ_CBase___bbcmd_4011_RemoveIfLinkObjectHit[0x20]; //00196720
    undefined OBJ_CBase___bbcmd_4008_LinkObjectDirection[0x20]; //00196740
    undefined OBJ_CBase___bbcmd_4024_00196760[0x20]; //00196760
    undefined OBJ_CBase___bbcmd_4007_LinkObjectPosition[0xb0]; //00196780
    undefined OBJ_CBase___bbcmd_4012_LinkObjectPositionCenter[0x20]; //00196830
    undefined OBJ_CBase___bbcmd_4022_LinkObjectX[0x90]; //00196850
    undefined OBJ_CBase___bbcmd_4023_LinkObjectY[0x90]; //001968e0
    undefined OBJ_CBase___bbcmd_4013_LinkObjectScale[0x20]; //00196970
    undefined OBJ_CBase___bbcmd_4009_LinkObjectTimestop[0x20]; //00196990
    undefined OBJ_CBase___bbcmd_4019_LinkObjectZ[0x20]; //001969b0
    undefined OBJ_CBase___bbcmd_4002_LinkParticle[0x110]; //001969d0
    undefined OBJ_CBase___bbcmd_23067_CallPrivateEffect[0x100]; //00196ae0
    undefined OBJ_CBase___bbcmd_4021_LinkPolyline[0x60]; //00196be0
    undefined OBJ_CBase___bbcmd_5005_DisableOppSprite[0x40]; //00196c40
    undefined OBJ_CBase___bbcmd_5004_ClipThroughOpp[0x20]; //00196c80
    undefined OBJ_CBase___bbcmd_21003_MagnetismLevel[0x20]; //00196ca0
    undefined OBJ_CBase___bbcmd_23158_MakotoDriveGauge[0x50]; //00196cc0
    undefined OBJ_CBase___bbcmd_18003_Mouth[0xb0]; //00196d10
    undefined OBJ_CBase___bbcmd_18011_MouthToVoice[0x30]; //00196dc0
    undefined OBJ_CBase___bbcmd_21006_Move2CenterOffset[0x60]; //00196df0
    undefined OBJ_CBase___bbcmd_23070_MovementSeal[0x30]; //00196e50
    undefined OBJ_CBase___bbcmd_1086_MoveTo[0x50]; //00196e80
    undefined OBJ_CBase___bbcmd_1085_MoveToCollision[0x60]; //00196ed0
    undefined OBJ_CBase___bbcmd_23151_EffectPosition[0x50]; //00196f30
    undefined OBJ_CBase___bbcmd_1087_MoveObjectTo[0x60]; //00196f80
    undefined OBJ_CBase___bbcmd_23142_MultiAttackObjectStopOnGuardpoint[0x30]; //00196fe0
    undefined OBJ_CBase___bbcmd_22007_setInvincible[0x20]; //00197010
    undefined OBJ_CBase___bbcmd_22019_SpecificInvincibility[0x80]; //00197030
    undefined OBJ_CBase___bbcmd_22034_AirAttackInvincibility[0x30]; //001970b0
    undefined OBJ_CBase___bbcmd_22026_BurstInvincibility[0x30]; //001970e0
    undefined OBJ_CBase___bbcmd_22006_CollidableInvincibility[0x20]; //00197110
    undefined OBJ_CBase___bbcmd_22009_InvincibleState[0x20]; //00197130
    undefined OBJ_CBase___bbcmd_22028_BlockOverhead[0x30]; //00197150
    undefined OBJ_CBase___bbcmd_22035_ArmorChipDamage[0x10]; //00197180
    undefined OBJ_CBase___bbcmd_22036_DollInvincibility[0x30]; //00197190
    undefined OBJ_CBase___bbcmd_22029_BlockLow[0x30]; //001971c0
    undefined OBJ_CBase___bbcmd_22030_GuardpointBlocksUnblockable[0x30]; //001971f0
    undefined OBJ_CBase___bbcmd_22024_ArmorHealth[0x10]; //00197220
    undefined OBJ_CBase___bbcmd_22031_GuardpointHitstop[0x20]; //00197230
    undefined OBJ_CBase___bbcmd_22027_BlockMid[0x30]; //00197250
    undefined OBJ_CBase___bbcmd_22021_InvincibilityLevel[0x10]; //00197280
    undefined OBJ_CBase___bbcmd_22033_ThrowInvincible[0x30]; //00197290
    undefined OBJ_CBase___bbcmd_22022_StrikeProjectileBypass[0x30]; //001972c0
    undefined OBJ_CBase___bbcmd_22025_SpecificGuardPoint[0x30]; //001972f0
    undefined OBJ_CBase___bbcmd_22032_PretendNoGuardPointIfFail[0x30]; //00197320
    undefined OBJ_CBase___bbcmd_22023_ProjectileInvincibility[0x30]; //00197350
    undefined OBJ_CBase___bbcmd_22037_PreventProjectile[0x30]; //00197380
    undefined OBJ_CBase___bbcmd_22008_InvincibilityDuration[0x10]; //001973b0
    undefined OBJ_CBase___bbcmd_22020_GuardPoint_[0x10]; //001973c0
    undefined OBJ_CBase___bbcmd_7002_NarrationSE[0x70]; //001973d0
    undefined OBJ_CBase___bbcmd_23076_NegativeForBackDash[0xc0]; //00197440
    undefined OBJ_CBase___bbcmd_2003_NoAttackDuringAction[0x30]; //00197500
    undefined OBJ_CBase___bbcmd_2002_NoAttackDuringSprite[0x10]; //00197530
    undefined OBJ_CBase___bbcmd_23022_NoDamageAction[0x30]; //00197540
    undefined OBJ_CBase___bbcmd_3038_Hide2DObject[0x20]; //00197570
    undefined OBJ_CBase___bbcmd_1043_DefaultToObjectGravity[0x20]; //00197590
    undefined OBJ_CBase___bbcmd_52_ObjectPriority[0xed0]; //001975b0
    undefined OBJ_CBase___bbcmd_39_OperateOneOperand[0x90]; //00198480
    undefined OBJ_CBase___bbcmd_40_OperateTwoOperands[0x130]; //00198510
    undefined OBJ_CBase___bbcmd_49_OperandAandBtoA[0x30]; //00198640
    undefined OBJ_CBase___bbcmd_47_OperandAandBtoC[0x80]; //00198670
    undefined OBJ_CBase___bbcmd_41_StoreValue[0x30]; //001986f0
    undefined OBJ_CBase___bbcmd_48_OperandCopy[0x50]; //00198720
    undefined OBJ_CBase___bbcmd_9011_OrderedVoiceline[0x70]; //00198770
    undefined OBJ_CBase___bbcmd_23001_ExternalForcesRate[0x20]; //001987e0
    undefined OBJ_CBase___bbcmd_23165_DisableOverdrive2[0x20]; //00198800
    undefined OBJ_CBase___bbcmd_3041_PaletteParam0[0x10]; //00198820
    undefined OBJ_CBase___bbcmd_3042_PaletteParam1[0x10]; //00198830
    undefined OBJ_CBase___bbcmd_3043_PaletteParam2[0x10]; //00198840
    undefined OBJ_CBase___bbcmd_3044_PaletteParam3[0x10]; //00198850
    undefined OBJ_CBase___bbcmd_3045_EffFadeParam0[0x10]; //00198860
    undefined OBJ_CBase___bbcmd_3046_EffFadeParam1[0x10]; //00198870
    undefined OBJ_CBase___bbcmd_3047_EffFadeParam2[0x10]; //00198880
    undefined OBJ_CBase___bbcmd_3048_EffFadeParam3[0x10]; //00198890
    undefined OBJ_CBase___bbcmd_3040_PaletteEffType[0x10]; //001988a0
    undefined OBJ_CBase___bbcmd_1034_AccelerationPercentage[0x30]; //001988b0
    undefined OBJ_CBase___bbcmd_19015_PerAccelerationZ[0x30]; //001988e0
    undefined OBJ_CBase___bbcmd_2039_ActionMarkPercentage[0x20]; //00198910
    undefined OBJ_CBase___bbcmd_3003_AlphaPercentage[0x30]; //00198930
    undefined OBJ_CBase___bbcmd_3006_AlphaPercentagePerFrame[0x30]; //00198960
    undefined OBJ_CBase___bbcmd_1076_AnglePercentagePerFrame[0x30]; //00198990
    undefined OBJ_CBase___bbcmd_3021_BluePercentage[0x30]; //001989c0
    undefined OBJ_CBase___bbcmd_3024_BluePercentagePerFrame[0x30]; //001989f0
    undefined OBJ_CBase___bbcmd_2044_CommonActionMarkPercentage[0x20]; //00198a20
    undefined OBJ_CBase___bbcmd_1039_GravityPercentage[0x30]; //00198a40
    undefined OBJ_CBase___bbcmd_3015_GreenPercentage[0x30]; //00198a70
    undefined OBJ_CBase___bbcmd_3018_PerConstantGreenModifier[0x30]; //00198aa0
    undefined OBJ_CBase___bbcmd_23095_00198ad0[0x30]; //00198ad0
    undefined OBJ_CBase___bbcmd_23098_00198b00[0x30]; //00198b00
    undefined OBJ_CBase___bbcmd_23113_AbsoluteBluePercentage[0x30]; //00198b30
    undefined OBJ_CBase___bbcmd_23116_AbsoluteBluePercentagePerFrame[0x30]; //00198b60
    undefined OBJ_CBase___bbcmd_23107_AbsoluteGreenPercentage[0x30]; //00198b90
    undefined OBJ_CBase___bbcmd_23110_AbsoluteGreenPercentagePerFrame[0x30]; //00198bc0
    undefined OBJ_CBase___bbcmd_23101_AbsoluteRedPercentage[0x30]; //00198bf0
    undefined OBJ_CBase___bbcmd_23104_AbsoluteRedPercentagePerFrame[0x30]; //00198c20
    undefined OBJ_CBase___bbcmd_1051_InertiaPercentage[0x30]; //00198c50
    undefined OBJ_CBase___bbcmd_23019_PerExternalForces[0x50]; //00198c80
    undefined OBJ_CBase___bbcmd_18006_PerPrivateValue[0x50]; //00198cd0
    undefined OBJ_CBase___bbcmd_3009_RedPercentage[0x30]; //00198d20
    undefined OBJ_CBase___bbcmd_3012_PerConstantRedModifier[0x30]; //00198d50
    undefined OBJ_CBase___bbcmd_1098_ScalePercentage[0x30]; //00198d80
    undefined OBJ_CBase___bbcmd_1101_ScalePercentagePerFrame[0x30]; //00198db0
    undefined OBJ_CBase___bbcmd_1061_ScaleXPercentagePerFrame[0x30]; //00198de0
    undefined OBJ_CBase___bbcmd_1069_ScaleYPercentagePerFrame[0x30]; //00198e10
    undefined OBJ_CBase___bbcmd_1093_ScaleZPercentagePerFrame[0x30]; //00198e40
    undefined OBJ_CBase___bbcmd_1058_ScaleXPercentage[0x30]; //00198e70
    undefined OBJ_CBase___bbcmd_1066_ScaleYPercentage[0x30]; //00198ea0
    undefined OBJ_CBase___bbcmd_1090_ScaleZPercentage[0x30]; //00198ed0
    undefined OBJ_CBase___bbcmd_1019_SpeedXPercentage[0x30]; //00198f00
    undefined OBJ_CBase___bbcmd_1024_SpeedYPercentage[0x30]; //00198f30
    undefined OBJ_CBase___bbcmd_19009_PerSpeedZ[0x30]; //00198f60
    undefined OBJ_CBase___bbcmd_7014_SpecialSE[0x90]; //00198f90
    undefined OBJ_CBase___bbcmd_23048_TrailParameter[0x80]; //00199020
    undefined OBJ_CBase___bbcmd_23049_TrailColor[0x20]; //001990a0
    undefined OBJ_CBase___bbcmd_23052_TrailFade[0x30]; //001990c0
    undefined OBJ_CBase___bbcmd_23051_TrailSprite[0x60]; //001990f0
    undefined OBJ_CBase___bbcmd_23050_TrailScale[0x20]; //00199150
    undefined OBJ_CBase___bbcmd_1033_PopAcceleration[0x10]; //00199170
    undefined OBJ_CBase___bbcmd_19014_PopAccelerationZ[0x10]; //00199180
    undefined OBJ_CBase___bbcmd_1038_PopGravity[0x10]; //00199190
    undefined OBJ_CBase___bbcmd_1050_PopInertia[0x10]; //001991a0
    undefined OBJ_CBase___bbcmd_1005_PopPosX[0x10]; //001991b0
    undefined OBJ_CBase___bbcmd_1009_PopPosY[0x10]; //001991c0
    undefined OBJ_CBase___bbcmd_19003_PopPositionZ[0x10]; //001991d0
    undefined OBJ_CBase___bbcmd_1018_PopSpeedX[0x10]; //001991e0
    undefined OBJ_CBase___bbcmd_1023_PopSpeedY[0x10]; //001991f0
    undefined OBJ_CBase___bbcmd_19008_PopSpeedZ[0x10]; //00199200
    undefined OBJ_CBase___bbcmd_2066_00199210[0x20]; //00199210
    undefined OBJ_CBase___bbcmd_23030_CallPrivateFunction[0x30]; //00199230
    undefined OBJ_CBase___bbcmd_7003_PrivateSE[0x90]; //00199260
    undefined OBJ_CBase___bbcmd_1032_PushAcceleration[0x10]; //001992f0
    undefined OBJ_CBase___bbcmd_19013_PushAccelerationZ[0x10]; //00199300
    undefined OBJ_CBase___bbcmd_2017_EnableCollision[0x20]; //00199310
    undefined OBJ_CBase___bbcmd_2016_SetYCollisionFromOrigin[0x10]; //00199330
    undefined OBJ_CBase___bbcmd_23087_PushCollisionHeightLow[0x10]; //00199340
    undefined OBJ_CBase___bbcmd_2015_SetXCollisionFromOrigin[0x10]; //00199350
    undefined OBJ_CBase___bbcmd_2021_PreventSelfPush[0x30]; //00199360
    undefined OBJ_CBase___bbcmd_1037_PushGravity[0x10]; //00199390
    undefined OBJ_CBase___bbcmd_1049_PushInertia[0x10]; //001993a0
    undefined OBJ_CBase___bbcmd_1004_PushPosX[0x10]; //001993b0
    undefined OBJ_CBase___bbcmd_1008_PushPosY[0x10]; //001993c0
    undefined OBJ_CBase___bbcmd_19002_PushPositionZ[0x10]; //001993d0
    undefined OBJ_CBase___bbcmd_1017_PushSpeedX[0x10]; //001993e0
    undefined OBJ_CBase___bbcmd_1022_PushSpeedY[0x10]; //001993f0
    undefined OBJ_CBase___bbcmd_19007_PushSpeedZ[0x10]; //00199400
    undefined OBJ_CBase___bbcmd_1040_RandomAddAccelerationRelative[0x80]; //00199410
    undefined OBJ_CBase___bbcmd_1042_RandomAddGravityAbsolute[0x40]; //00199490
    undefined OBJ_CBase___bbcmd_19016_RandAddAccelerationZ[0x40]; //001994d0
    undefined OBJ_CBase___bbcmd_1077_RandomAddAngle[0x50]; //00199510
    undefined OBJ_CBase___bbcmd_1078_RandomAddAnglePerFrame[0x40]; //00199560
    undefined OBJ_CBase___bbcmd_1041_RandomAddGravity[0x40]; //001995a0
    undefined OBJ_CBase___bbcmd_1053_RandomAddInertia[0x80]; //001995e0
    undefined OBJ_CBase___bbcmd_1054_RandomAddInertiaAbsolute[0x40]; //00199660
    undefined OBJ_CBase___bbcmd_1010_RandomAddPosXRelative[0x80]; //001996a0
    undefined OBJ_CBase___bbcmd_1012_RandomAddPosXAbsolute[0x40]; //00199720
    undefined OBJ_CBase___bbcmd_1011_RandomAddPosY[0x40]; //00199760
    undefined OBJ_CBase___bbcmd_19004_RandAddPositionZ[0x40]; //001997a0
    undefined OBJ_CBase___bbcmd_18007_RandAddPrivateValue[0x60]; //001997e0
    undefined OBJ_CBase___bbcmd_1102_RandomAddScale[0x40]; //00199840
    undefined OBJ_CBase___bbcmd_1103_RandomAddScalePerFrame[0x40]; //00199880
    undefined OBJ_CBase___bbcmd_1063_RandomAddScaleXPerFrame[0x40]; //001998c0
    undefined OBJ_CBase___bbcmd_1071_RandomAddScaleYPerFrame[0x40]; //00199900
    undefined OBJ_CBase___bbcmd_1095_RandomAddScaleZPerFrame[0x40]; //00199940
    undefined OBJ_CBase___bbcmd_1062_RandomAddScaleX[0x40]; //00199980
    undefined OBJ_CBase___bbcmd_1070_RandomAddScaleY[0x40]; //001999c0
    undefined OBJ_CBase___bbcmd_1094_RandomAddScaleZ[0x40]; //00199a00
    undefined OBJ_CBase___bbcmd_4017_RandomAddSpeedInSphere[0x110]; //00199a40
    undefined OBJ_CBase___bbcmd_1025_RandomAddSpeedXRelative[0x80]; //00199b50
    undefined OBJ_CBase___bbcmd_1027_RandomAddSpeedXAbsolute[0x40]; //00199bd0
    undefined OBJ_CBase___bbcmd_1026_SetAccelerationRelative[0x40]; //00199c10
    undefined OBJ_CBase___bbcmd_19010_RandAddSpeedZ[0x40]; //00199c50
    undefined OBJ_CBase___bbcmd_1079_RandomAngle[0x30]; //00199c90
    undefined OBJ_CBase___bbcmd_9009_RandomSubVoiceline[0xa0]; //00199cc0
    undefined OBJ_CBase___bbcmd_7006_RandomVoiceline[0xa0]; //00199d60
    undefined OBJ_CBase___bbcmd_2000_RefreshHit[0x30]; //00199e00
    undefined OBJ_CBase___bbcmd_23015_RenderLayer[0x10]; //00199e30
    undefined OBJ_CBase___bbcmd_12_ExitState[0x10]; //00199e40
    undefined OBJ_CBase___bbcmd_23024_SetBackground[0x10]; //00199e50
    undefined OBJ_CBase___bbcmd_11_SendToLabel[0x10]; //00199e60
    undefined OBJ_CBase___bbcmd_23090_DisableObjectHitbox[0x50]; //00199e70
    undefined OBJ_CBase___bbcmd_3028_ScreenShake[0x50]; //00199ec0
    undefined OBJ_CBase___bbcmd_22002_ResetAirDashes[0x20]; //00199f10
    undefined OBJ_CBase___bbcmd_22000_ResetAirJumps[0x20]; //00199f30
    undefined OBJ_CBase___bbcmd_14089_ResetEasyInputA[0x10]; //00199f50
    undefined OBJ_CBase___bbcmd_14090_ResetEasyInputB[0x10]; //00199f60
    undefined OBJ_CBase___bbcmd_14091_ResetEasyInputC[0x10]; //00199f70
    undefined OBJ_CBase___bbcmd_14092_ResetEasyInputD[0x10]; //00199f80
    undefined OBJ_CBase___bbcmd_14093_ResetEasyInputE[0x10]; //00199f90
    undefined OBJ_CBase___bbcmd_23014_ResetExternalForces[0x20]; //00199fa0
    undefined OBJ_CBase___bbcmd_1084_ResetSpeed[0x70]; //00199fc0
    undefined OBJ_CBase___bbcmd_2034_ScreenCollision[0x20]; //0019a030
    undefined OBJ_CBase___bbcmd_3079_0019a050[0x30]; //0019a050
    undefined OBJ_CBase___bbcmd_6001_ScreenPosition[0x20]; //0019a080
    undefined OBJ_CBase___bbcmd_23012_SelfWindRate[0x30]; //0019a0a0
    undefined OBJ_CBase___bbcmd_21007_ObjectUpon[0x40]; //0019a0d0
    undefined OBJ_CBase___bbcmd_21012_TriggerUponForState[0x80]; //0019a110
    undefined OBJ_CBase___bbcmd_21013_DisableEffect[0x80]; //0019a190
    undefined OBJ_CBase___bbcmd_23029_ObjectUpon2[0x60]; //0019a210
    undefined OBJ_CBase___bbcmd_23155_ObjectGotoState[0x90]; //0019a270
    undefined OBJ_CBase___bbcmd_1028_SetAccelerationRelative[0x50]; //0019a300
    undefined OBJ_CBase___bbcmd_1029_SetAccelerationAbsolute[0x10]; //0019a350
    undefined OBJ_CBase___bbcmd_19011_SetAccelerationZ[0x10]; //0019a360
    undefined OBJ_CBase___bbcmd_2037_SetActionMark[0x10]; //0019a370
    undefined OBJ_CBase___bbcmd_2055_MaxActionTime[0x10]; //0019a380
    undefined OBJ_CBase___bbcmd_3027_AddColor[0x10]; //0019a390
    undefined OBJ_CBase___bbcmd_3001_SetAlpha[0x10]; //0019a3a0
    undefined OBJ_CBase___bbcmd_3004_SetAlphaPerFrame[0x10]; //0019a3b0
    undefined OBJ_CBase___bbcmd_1072_SetAngle[0x20]; //0019a3c0
    undefined OBJ_CBase___bbcmd_23068_SetAngleByDirSign[0x60]; //0019a3e0
    undefined OBJ_CBase___bbcmd_1108_SetAngleBySpeed[0x40]; //0019a440
    undefined OBJ_CBase___bbcmd_1074_SetAnglePerFrame[0x10]; //0019a480
    undefined OBJ_CBase___bbcmd_1104_0019a490[0x30]; //0019a490
    undefined OBJ_CBase___bbcmd_1105_0019a4c0[0x30]; //0019a4c0
    undefined OBJ_CBase___bbcmd_2056_MaxObjectHitInteraction[0x30]; //0019a4f0
    undefined OBJ_CBase___bbcmd_20010_CameraCoordinates[0x40]; //0019a520
    undefined OBJ_CBase___bbcmd_3055_SetBasePaletteAlpha[0x80]; //0019a560
    undefined OBJ_CBase___bbcmd_3056_SetBasePaletteColor[0x70]; //0019a5e0
    undefined OBJ_CBase___bbcmd_3019_SetBlue[0x10]; //0019a650
    undefined OBJ_CBase___bbcmd_3022_SetBluePerFrame[0x10]; //0019a660
    undefined OBJ_CBase___bbcmd_1083_SetBounceRate[0x10]; //0019a670
    undefined OBJ_CBase___bbcmd_23178_FreezeSprite[0x30]; //0019a680
    undefined OBJ_CBase___bbcmd_30005_Char3DVisiblity[0x30]; //0019a6b0
    undefined OBJ_CBase___bbcmd_30001_Char3DAnim[0x30]; //0019a6e0
    undefined OBJ_CBase___bbcmd_30006_0019a710[0x40]; //0019a710
    undefined OBJ_CBase___bbcmd_30002_0019a750[0x50]; //0019a750
    undefined OBJ_CBase___bbcmd_30004_Char3DRotation[0x30]; //0019a7a0
    undefined OBJ_CBase___bbcmd_30003_Char3DScale[0x40]; //0019a7d0
    undefined OBJ_CBase___bbcmd_3026_SetColor[0x40]; //0019a810
    undefined OBJ_CBase___bbcmd_21004_ColorFromPaletteIndex[0x20]; //0019a850
    undefined OBJ_CBase___bbcmd_2042_SetCommonActionMark[0x10]; //0019a870
    undefined OBJ_CBase___bbcmd_3061_SetCompositeBlendMode[0x20]; //0019a880
    undefined OBJ_CBase___bbcmd_3068_SetCompositeColor[0x40]; //0019a8a0
    undefined OBJ_CBase___bbcmd_3060_SetCompositeImage[0x30]; //0019a8e0
    undefined OBJ_CBase___bbcmd_3062_SetCompositePosX[0x20]; //0019a910
    undefined OBJ_CBase___bbcmd_3063_SetCompositePosY[0x20]; //0019a930
    undefined OBJ_CBase___bbcmd_3066_SetCompositeScaleX[0x20]; //0019a950
    undefined OBJ_CBase___bbcmd_3067_SetCompositeScaleY[0x20]; //0019a970
    undefined OBJ_CBase___bbcmd_3064_SetCompositeSpeedX[0x20]; //0019a990
    undefined OBJ_CBase___bbcmd_3065_SetCompositeSpeedY[0x20]; //0019a9b0
    undefined OBJ_CBase___bbcmd_18009_CrouchState[0x30]; //0019a9d0
    undefined OBJ_CBase___bbcmd_3054_SetCustomPaletteAlpha[0x70]; //0019aa00
    undefined OBJ_CBase___bbcmd_12018_HitSprites[0x10]; //0019aa70
    undefined OBJ_CBase___bbcmd_3025_SetDestinationColor[0x70]; //0019aa80
    undefined OBJ_CBase___bbcmd_23130_ApplyNegativeTint[0x70]; //0019aaf0
    undefined OBJ_CBase___bbcmd_3058_SetDistortionTextureLevel[0x10]; //0019ab60
    undefined OBJ_CBase___bbcmd_3059_SetDistortionTextureLevelSpeed[0x10]; //0019ab70
    undefined OBJ_CBase___bbcmd_30016_BloomFade[0x30]; //0019ab80
    undefined OBJ_CBase___bbcmd_30017_BloomPower[0x30]; //0019abb0
    undefined OBJ_CBase___bbcmd_5000_EnemyThrowAnimation[0x60]; //0019abe0
    undefined OBJ_CBase___bbcmd_23123_ReverseApplyTintOverDuration[0x80]; //0019ac40
    undefined OBJ_CBase___bbcmd_2048_SetFloorReflectColor[0x10]; //0019acc0
    undefined OBJ_CBase___bbcmd_1035_SetGravity[0x10]; //0019acd0
    undefined OBJ_CBase___bbcmd_3013_PerConstantRedModifier[0x10]; //0019ace0
    undefined OBJ_CBase___bbcmd_3016_SetGreenPerFrame[0x10]; //0019acf0
    undefined OBJ_CBase___bbcmd_23093_0019ad00[0x10]; //0019ad00
    undefined OBJ_CBase___bbcmd_23096_0019ad10[0x10]; //0019ad10
    undefined OBJ_CBase___bbcmd_23111_SetAbsoluteBlue[0x10]; //0019ad20
    undefined OBJ_CBase___bbcmd_23114_SetAbsoluteBluePerFrame[0x10]; //0019ad30
    undefined OBJ_CBase___bbcmd_23118_StopCharacterFlash1[0x40]; //0019ad40
    undefined OBJ_CBase___bbcmd_23122_TintRelativeToPalette[0x20]; //0019ad80
    undefined OBJ_CBase___bbcmd_23117_CharacterFlash2[0x70]; //0019ada0
    undefined OBJ_CBase___bbcmd_23119_CharacterFlash[0x70]; //0019ae10
    undefined OBJ_CBase___bbcmd_23105_SetAbsoluteGreen[0x10]; //0019ae80
    undefined OBJ_CBase___bbcmd_23108_SetAbsoluteGreenPerFrame[0x10]; //0019ae90
    undefined OBJ_CBase___bbcmd_23099_SetAbsoluteRed[0x10]; //0019aea0
    undefined OBJ_CBase___bbcmd_23102_SetAbsoluteRedPerFrame[0x10]; //0019aeb0
    undefined OBJ_CBase___bbcmd_23021_CurrentHP[0x21]; //0019aec0
    undefined inject_steroid_HealthModify[0xf]; //0019aee1
    undefined OBJ_CBase___bbcmd_23020_MaxHP[0x80]; //0019aef0
    undefined OBJ_CBase___bbcmd_1045_SetInertia[0x50]; //0019af70
    undefined OBJ_CBase___bbcmd_1046_SetInertiaOverride[0x10]; //0019afc0
    undefined OBJ_CBase___bbcmd_23154_SmoothSetX[0x50]; //0019afd0
    undefined OBJ_CBase___bbcmd_3000_SetPaletteID[0x10]; //0019b020
    undefined OBJ_CBase___bbcmd_4061_PaletteIndex[0x20]; //0019b030
    undefined OBJ_CBase___bbcmd_30012_LoadSpritePalette[0x20]; //0019b050
    undefined OBJ_CBase___bbcmd_1000_SetPosXRelative[0x50]; //0019b070
    undefined OBJ_CBase___bbcmd_23032_SetPosXBytScreenPer[0x90]; //0019b0c0
    undefined OBJ_CBase___bbcmd_1001_SetPosXAbsolute[0x10]; //0019b150
    undefined OBJ_CBase___bbcmd_1006_SetPosY[0x10]; //0019b160
    undefined OBJ_CBase___bbcmd_23033_SetPosYBytScreenPer[0x60]; //0019b170
    undefined OBJ_CBase___bbcmd_19000_SetPositionZ[0x10]; //0019b1d0
    undefined OBJ_CBase___bbcmd_32_CallPublicFunction[0x30]; //0019b1e0
    undefined OBJ_CBase___bbcmd_18004_SetPrivateValue[0x30]; //0019b210
    undefined OBJ_CBase___bbcmd_3007_SetRed[0x10]; //0019b240
    undefined OBJ_CBase___bbcmd_3010_SetRedPerFrame[0x10]; //0019b250
    undefined OBJ_CBase___bbcmd_12051_StarterRating[0x10]; //0019b260
    undefined OBJ_CBase___bbcmd_12055_SkipThrowRequirement[0x30]; //0019b270
    undefined OBJ_CBase___bbcmd_12052_SameMoveProrationAlt[0x10]; //0019b2a0
    undefined OBJ_CBase___bbcmd_1096_SetScale[0x30]; //0019b2b0
    undefined OBJ_CBase___bbcmd_1099_SetScalePerFrame[0x30]; //0019b2e0
    undefined OBJ_CBase___bbcmd_1059_SetScaleXPerFrame[0x10]; //0019b310
    undefined OBJ_CBase___bbcmd_1067_SetScaleYPerFrame[0x10]; //0019b320
    undefined OBJ_CBase___bbcmd_1091_SetScaleZPerFrame[0x10]; //0019b330
    undefined OBJ_CBase___bbcmd_1056_SetScaleX[0x10]; //0019b340
    undefined OBJ_CBase___bbcmd_1064_SetScaleY[0x10]; //0019b350
    undefined OBJ_CBase___bbcmd_1088_SetScaleZ[0x10]; //0019b360
    undefined OBJ_CBase___bbcmd_13043_ShadowOffsetY[0x20]; //0019b370
    undefined OBJ_CBase___bbcmd_23080_ObjectMoveCheck[0x60]; //0019b390
    undefined OBJ_CBase___bbcmd_1109_SetSpeedByPolarCoordinate[0x80]; //0019b3f0
    undefined OBJ_CBase___bbcmd_1110_SetSpeedByAngle[0xd0]; //0019b470
    undefined OBJ_CBase___bbcmd_1111_SetSpeedTowardsObject[0x80]; //0019b540
    undefined OBJ_CBase___bbcmd_1082_ResetSpeedAfterDuration[0x10]; //0019b5c0
    undefined OBJ_CBase___bbcmd_1013_SetSpeedXRelative[0x1e0]; //0019b5d0
    undefined OBJ_CBase___bbcmd_1014_SetSpeedXAbsolute[0x10]; //0019b7b0
    undefined OBJ_CBase___bbcmd_1020_SetSpeedY[0x10]; //0019b7c0
    undefined OBJ_CBase___bbcmd_19005_SetSpeedZ[0x10]; //0019b7d0
    undefined OBJ_CBase___bbcmd_1080_SetTransformCenterX[0x30]; //0019b7e0
    undefined OBJ_CBase___bbcmd_1081_SetTransformCenterY[0x60]; //0019b810
    undefined OBJ_CBase___bbcmd_2018_SetZLine[0x70]; //0019b870
    undefined OBJ_CBase___bbcmd_2019_SetZVal[0x20]; //0019b8e0
    undefined OBJ_CBase___bbcmd_23016_SkeletonPaletteEffectOnNoStopIdling[0x3d0]; //0019b900
    undefined OBJ_CBase___bbcmd_14003_Move_Condition[0x50]; //0019bcd0
    undefined OBJ_CBase___bbcmd_14012_Move_Input_[0x50]; //0019bd20
    undefined OBJ_CBase___bbcmd_14001_Move_Register[0xe0]; //0019bd70
    undefined OBJ_CBase___bbcmd_15001_CPUInput[0xd0]; //0019be50
    undefined OBJ_CBase___bbcmd_15000_CPUUsable[0x50]; //0019bf20
    undefined OBJ_CBase___bbcmd_14027_SharedGatling[0x50]; //0019bf70
    undefined OBJ_CBase___bbcmd_14022_MoveMaxChainRepeat[0x40]; //0019bfc0
    undefined OBJ_CBase___bbcmd_14005_FollowupOnly[0x50]; //0019c000
    undefined OBJ_CBase___bbcmd_14011_0019c050[0x1030]; //0019c050
    undefined OBJ_CBase___bbcmd_14024_CallSkillConditions[0x50]; //0019d080
    undefined OBJ_CBase___bbcmd_14030_HitstunCancelOnly[0x50]; //0019d0d0
    undefined OBJ_CBase___bbcmd_14029_AddChain[0x40]; //0019d120
    undefined OBJ_CBase___bbcmd_14002_Move_EndRegister__[0x20]; //0019d160
    undefined OBJ_CBase___bbcmd_14023_Move_HPThreshold[0x40]; //0019d180
    undefined OBJ_CBase___bbcmd_15007_MoveAttackFrame[0x40]; //0019d1c0
    undefined OBJ_CBase___bbcmd_15005_MovePriority[0x40]; //0019d200
    undefined OBJ_CBase___bbcmd_15020_MoveCPULevel[0x60]; //0019d240
    undefined OBJ_CBase___bbcmd_15024_TempPriorityMultiplier[0xe0]; //0019d2a0
    undefined OBJ_CBase___bbcmd_15015_MoveCancellableFrames[0x50]; //0019d380
    undefined OBJ_CBase___bbcmd_15008_MoveMid[0x40]; //0019d3d0
    undefined OBJ_CBase___bbcmd_15006_MoveComboPriority[0x40]; //0019d410
    undefined OBJ_CBase___bbcmd_14017_SkillEstimateDistanceRate[0x60]; //0019d450
    undefined OBJ_CBase___bbcmd_15021_AirborneOpponentPriority[0x40]; //0019d4b0
    undefined OBJ_CBase___bbcmd_15014_OpponentAttackPriority[0x40]; //0019d4f0
    undefined OBJ_CBase___bbcmd_15013_DamageStunPriority[0x40]; //0019d530
    undefined OBJ_CBase___bbcmd_15012_GuardStunPriority[0x40]; //0019d570
    undefined OBJ_CBase___bbcmd_16001_0019d5b0[0x60]; //0019d5b0
    undefined OBJ_CBase___bbcmd_16000_0019d610[0x50]; //0019d610
    undefined OBJ_CBase___bbcmd_15025_TempPriorityMultiplierInterval[0x60]; //0019d660
    undefined OBJ_CBase___bbcmd_15009_MoveLow[0x40]; //0019d6c0
    undefined OBJ_CBase___bbcmd_15016_MoveButtonHoldTime[0x50]; //0019d700
    undefined OBJ_CBase___bbcmd_15017_MoveEstimatedInterval[0x50]; //0019d750
    undefined OBJ_CBase___bbcmd_15011_JumpAvoidPriority[0x40]; //0019d7a0
    undefined OBJ_CBase___bbcmd_14015_SkillEstimateRange[0x70]; //0019d7e0
    undefined OBJ_CBase___bbcmd_15026_CPUPressInputDuringFrames[0x50]; //0019d850
    undefined OBJ_CBase___bbcmd_15023_MoveTarget[0x40]; //0019d8a0
    undefined OBJ_CBase___bbcmd_15022_MoveHeatValuePriority[0x60]; //0019d8e0
    undefined OBJ_CBase___bbcmd_15010_MoveThrow[0x40]; //0019d940
    undefined OBJ_CBase___bbcmd_15027_0019d980[0x40]; //0019d980
    undefined OBJ_CBase___bbcmd_14007_FromBeginCancelLevel[0x40]; //0019d9c0
    undefined OBJ_CBase___bbcmd_14019_CallFunction[0x50]; //0019da00
    undefined OBJ_CBase___bbcmd_14006_GuardCancelOnly[0x50]; //0019da50
    undefined OBJ_CBase___bbcmd_15003_PlayerUsable[0x50]; //0019daa0
    undefined OBJ_CBase___bbcmd_14008_RemoveHitstop[0x50]; //0019daf0
    undefined OBJ_CBase___bbcmd_14018_IndependentInput[0x80]; //0019db40
    undefined OBJ_CBase___bbcmd_14000_SkillInit[0x840]; //0019dbc0
    undefined OBJ_CBase___bbcmd_14025_InputDirectionBaseObject[0x40]; //0019e400
    undefined OBJ_CBase___bbcmd_14021_ForceSkillDirection[0x50]; //0019e440
    undefined OBJ_CBase___bbcmd_14004_NeutralOnly[0x50]; //0019e490
    undefined OBJ_CBase___bbcmd_14033_0019e4e0[0x50]; //0019e4e0
    undefined OBJ_CBase___bbcmd_14020_DisallowSameSkill[0x50]; //0019e530
    undefined OBJ_CBase___bbcmd_15004_SkillRecipe[0x50]; //0019e580
    undefined OBJ_CBase___bbcmd_14013_StateCall[0x50]; //0019e5d0
    undefined OBJ_CBase___bbcmd_14014_StateCallFlag[0x40]; //0019e620
    undefined OBJ_CBase___bbcmd_14026_OldSpecialInput[0x40]; //0019e660
    undefined OBJ_CBase___bbcmd_14010_SkillCommonInitialize[0x40]; //0019e6a0
    undefined OBJ_CBase___bbcmd_14009_SkillType[0x40]; //0019e6e0
    undefined OBJ_CBase___bbcmd_23077_SlowField[0x30]; //0019e720
    undefined OBJ_CBase___bbcmd_7012_CommonVoicelineNoRepeat[0x40]; //0019e750
    undefined OBJ_CBase___bbcmd_9010_RandomSubVoicelineNoRepeat[0xa0]; //0019e790
    undefined OBJ_CBase___bbcmd_7007_SmartRandomVoiceline[0xa0]; //0019e830
    undefined OBJ_CBase___bbcmd_9008_SubVoicelineNoRepeat[0x40]; //0019e8d0
    undefined OBJ_CBase___bbcmd_7004_SmartVoiceline[0x40]; //0019e910
    undefined OBJ_CBase___bbcmd_23017_GuardCancel[0x30]; //0019e950
    undefined OBJ_CBase___bbcmd_38_RegisterObject[0x130]; //0019e980
    undefined OBJ_CBase___bbcmd_13041_ArakuneInvisibilityEffect[0x90]; //0019eab0
    undefined OBJ_CBase___bbcmd_7015_EndSE[0x10]; //0019eb40
    undefined OBJ_CBase___bbcmd_23131_0019eb50[0x10]; //0019eb50
    undefined OBJ_CBase___bbcmd_23120_StopCharacterFlash2[0x10]; //0019eb60
    undefined OBJ_CBase___bbcmd_14049_StylishModeCancels[0x80]; //0019eb70
    undefined OBJ_CBase___bbcmd_14048_StylishModeSpecialButton[0x80]; //0019ebf0
    undefined OBJ_CBase___bbcmd_9007_SubVoiceline[0x30]; //0019ec70
    undefined OBJ_CBase___bbcmd_14_EndObject[0x10]; //0019eca0
    undefined OBJ_CBase___bbcmd_2052_EnableExternalForces[0x30]; //0019ecb0
    undefined OBJ_CBase___bbcmd_12020_SyncEntry[0x20]; //0019ece0
    undefined OBJ_CBase___bbcmd_23181_HeatCooldown[0x20]; //0019ed00
    undefined OBJ_CBase___bbcmd_23085_TurnLimitByInitialize[0x30]; //0019ed20
    undefined OBJ_CBase___bbcmd_2036_DistortionSettings[0x150]; //0019ed50
    undefined OBJ_CBase___bbcmd_2004_UseCenterOffset[0xb0]; //0019eea0
    undefined OBJ_CBase___bbcmd_23179_0019ef50[0x30]; //0019ef50
    undefined OBJ_CBase___bbcmd_7001_Voiceline[0x70]; //0019ef80
    undefined OBJ_CBase___bbcmd_18010_WinAction[0x20]; //0019eff0
    undefined OBJ_CBase___bbcmd_23000_Wind[0x1d0]; //0019f010
    undefined OBJ_CBase___bbcmd_2053_WallCollisionDetection[0x20]; //0019f1e0
    undefined OBJ_CBase___bbcmd_53_DestroyOnStageEdge[0x30]; //0019f200
    undefined OBJ_CBase___bbcmd_23143_TravelFromPointToPoint[0x17142]; //0019f230
    undefined inject_GetPalBaseAddresses[0xa1e]; //001b6372
    undefined OBJ_CCharBase___check_to_enter_CmnActAirTurn[0x390]; //001b6d90
    undefined OBJ_CCharBase___check_to_enter_CmnActCrouchTurn[0x50]; //001b7120
    undefined OBJ_CCharBase___check_to_entrer_CmnActStand2Crouch[0x1cb0]; //001b7170
    undefined OBJ_CCharBase___check_to_enter_CmnActStandTurn[0x818c]; //001b8e20
    undefined inject_steroid_OverdriveCharge[0x94]; //001c0fac
    undefined OBJ_CCharBase__add_heat[0x168]; //001c1040
    undefined inject_steroid_HeatModify[0x1228]; //001c11a8
    undefined GAME_CBB4VersionInfo1_00_00___return_10000[0x2800]; //001c23d0
    undefined OBJ_CCharBase__ctor[0x780]; //001c4bd0
    undefined OBJ_CCharBase__dtor[0x4e90]; //001c5350
    undefined OBJ_CCharBase___check_input_buffer_[0x180]; //001ca1e0
    undefined OBJ_CCharBase___bbcmd_18001_ClearCommonActionControl[0x3a0]; //001ca360
    undefined OBJ_CCharBase__update_input_buffer_fields[0x20f]; //001ca700
    undefined OBJ_CCharBase___update_input_buffer_fields_1[0x1b81]; //001ca90f
    undefined OBJ_CCharBase___update_from_settings_6_[0xa64]; //001cc490
    undefined inject_training_healthModifyFix[0x109]; //001ccef4
    undefined inject_getTrainingHealthModifyFixJmpBackAddr[0x1223]; //001ccffd
    undefined OBJ_CCharBase___bbcmd_13032_Enable2GetUp[0x20]; //001ce220
    undefined OBJ_CCharBase___bbcmd_13034_Enable2UkemiAir[0x20]; //001ce240
    undefined OBJ_CCharBase___bbcmd_13033_Enable2UkemiLand[0x20]; //001ce260
    undefined OBJ_CCharBase___bbcmd_13036_Enable2UkemiLeap[0x20]; //001ce280
    undefined OBJ_CCharBase___bbcmd_13037_Enable2UkemiLeapEx[0x20]; //001ce2a0
    undefined OBJ_CCharBase___bbcmd_13035_Enable2UkemiStagger[0x20]; //001ce2c0
    undefined OBJ_CCharBase___bbcmd_13012_WhiffBAirDashCancel[0x30]; //001ce2e0
    undefined OBJ_CCharBase___bbcmd_13011_WhiffFAirDashCancel[0x30]; //001ce310
    undefined OBJ_CCharBase___bbcmd_13010_WhiffAirJumpCancel[0x30]; //001ce340
    undefined OBJ_CCharBase___bbcmd_13022_EndIfChangeFacing[0x30]; //001ce370
    undefined OBJ_CCharBase___bbcmd_13030_EnableAutoBlock[0x30]; //001ce3a0
    undefined OBJ_CCharBase___bbcmd_13027_AutomaticThrowReject[0x30]; //001ce3d0
    undefined OBJ_CCharBase___bbcmd_13007_WhiffBCrouchWalkCancel[0x30]; //001ce400
    undefined OBJ_CCharBase___bbcmd_13006_WhiffBDashCancel[0x20]; //001ce430
    undefined OBJ_CCharBase___bbcmd_13005_WhiffBWalkCancel[0x20]; //001ce450
    undefined OBJ_CCharBase___bbcmd_13026_WhiffBarrierCancel2[0x30]; //001ce470
    undefined OBJ_CCharBase___bbcmd_13009_WhiffBarrierCancel[0x30]; //001ce4a0
    undefined OBJ_CCharBase___bbcmd_13031_WhiffOverdriveCancel[0x30]; //001ce4d0
    undefined OBJ_CCharBase___bbcmd_13017_WhiffCrouchingTurnCancel[0x30]; //001ce500
    undefined OBJ_CCharBase___bbcmd_13001_WhiffCrouchCancel[0x20]; //001ce530
    undefined OBJ_CCharBase___bbcmd_13021_WhiffCrouchBlockCancel[0x30]; //001ce550
    undefined OBJ_CCharBase___bbcmd_13018_WhiffDeathCancel[0x30]; //001ce580
    undefined OBJ_CCharBase___bbcmd_13028_EnableDemo[0x30]; //001ce5b0
    undefined OBJ_CCharBase___bbcmd_13004_WhiffFCrouchWalkCancel[0x20]; //001ce5e0
    undefined OBJ_CCharBase___bbcmd_13003_WhiffFDashCancel[0x20]; //001ce600
    undefined OBJ_CCharBase___bbcmd_13002_WhiffFWalkCancel[0x20]; //001ce620
    undefined OBJ_CCharBase___bbcmd_13019_WhiffDeathCancel[0x30]; //001ce640
    undefined OBJ_CCharBase___bbcmd_13008_WhiffJumpCancel[0x30]; //001ce670
    undefined OBJ_CCharBase___bbcmd_13013_WhiffThrowRejectCancel[0x30]; //001ce6a0
    undefined OBJ_CCharBase___bbcmd_13014_WhiffNormalCancel[0x30]; //001ce6d0
    undefined OBJ_CCharBase___bbcmd_13029_EnablePreGuard[0x30]; //001ce700
    undefined OBJ_CCharBase___bbcmd_13024_EnableRapidCancel[0x30]; //001ce730
    undefined OBJ_CCharBase___bbcmd_13015_WhiffSpecialCancel[0x30]; //001ce760
    undefined OBJ_CCharBase___bbcmd_13016_WhiffStandingTurnCancel[0x30]; //001ce790
    undefined OBJ_CCharBase___bbcmd_13000_WhiffStandCancel[0x4b0]; //001ce7c0
    undefined OBJ_CCharBase___reset[0x530]; //001cec70
    undefined OBJ_CCharBase__dtor_[0x2050]; //001cf1a0
    undefined OBJ_CCharBase___update_inputs_[0x5c0]; //001d11f0
    undefined OBJ_CCharBase___bbcmd_14069_HitOrBlockCancel[0x60]; //001d17b0
    undefined OBJ_CCharBase___bbcmd_14068_WhiffCancel[0x60]; //001d1810
    undefined OBJ_CCharBase___bbcmd_14080_HitCancel[0xc00]; //001d1870
    undefined OBJ_CCharBase___bbcmd_14077_ChainCancel[0x20]; //001d2470
    undefined OBJ_CCharBase___bbcmd_14084_AttackOffOrBlockCancel[0x60]; //001d2490
    undefined OBJ_CCharBase___bbcmd_14083_PreventWhiffCancel[0x60]; //001d24f0
    undefined OBJ_CCharBase___bbcmd_14085_AttackOffCancel[0x60]; //001d2550
    undefined OBJ_CCharBase___bbcmd_14074_DisallowGoto[0x60]; //001d25b0
    undefined OBJ_CCharBase___bbcmd_14070_BeginBuffer[0x60]; //001d2610
    undefined OBJ_CCharBase___bbcmd_14071_DisallowGoto3[0x60]; //001d2670
    undefined OBJ_CCharBase___bbcmd_14072_BufferedOrPressedGoto[0x60]; //001d26d0
    undefined OBJ_CCharBase___bbcmd_14073_DisallowGoto2[0x60]; //001d2730
    undefined OBJ_CCharBase___bbcmd_13038_EnableAll[0x120]; //001d2790
    undefined OBJ_CCharBase___bbcmd_14076_WhiffCancelEnable[0xc0]; //001d28b0
    undefined OBJ_CCharBase___bbcmd_17018_AttackDefaults_AirAstral[0xb0]; //001d2970
    undefined OBJ_CCharBase___bbcmd_17015_AttackDefaults_AirNoAttack[0x70]; //001d2a20
    undefined OBJ_CCharBase___bbcmd_17001_AttackDefaults_AirNormal[0xe0]; //001d2a90
    undefined OBJ_CCharBase___bbcmd_17003_AttackDefaults_AirSpecial[0xd0]; //001d2b70
    undefined OBJ_CCharBase___bbcmd_17005_AttackDefaults_AirDD[0x120]; //001d2c40
    undefined OBJ_CCharBase___bbcmd_17016_AttackDefaults_Sweep[0x90]; //001d2d60
    undefined OBJ_CCharBase___bbcmd_17022_AttackDefaults_Stage2ExceedAccel[0x280]; //001d2df0
    undefined OBJ_CCharBase___bbcmd_17021_AttackDefaults_Stage1ExceedAccel[0x180]; //001d3070
    undefined OBJ_CCharBase___bbcmd_17014_AttackDefaults_CrouchNoAttack[0x90]; //001d31f0
    undefined OBJ_CCharBase___bbcmd_17006_AttackDefaults_CrouchingNormal[0x130]; //001d3280
    undefined OBJ_CCharBase___bbcmd_17010_AttackDefaults_DeadAngle[0x210]; //001d33b0
    undefined OBJ_CCharBase___bbcmd_17009_AttackDefault_IntroAction[0x80]; //001d35c0
    undefined OBJ_CCharBase___bbcmd_17019_ScriptEndGroundBounce_[0x60]; //001d3640
    undefined OBJ_CCharBase___bbcmd_17020_ScriptSameAttackComboNoSpecialCancel[0x140]; //001d36a0
    undefined OBJ_CCharBase___bbcmd_17017_AttackDefaults_Astral[0xb0]; //001d37e0
    undefined OBJ_CCharBase___bbcmd_17013_AttackDefaults_NoAttack[0x80]; //001d3890
    undefined OBJ_CCharBase___bbcmd_17000_AttackDefaults_StandingNormal[0x110]; //001d3910
    undefined OBJ_CCharBase___bbcmd_17002_AttackDefaults_StandingSpecial[0xd0]; //001d3a20
    undefined OBJ_CCharBase___bbcmd_17008_AttackDefaults_AirThrow[0x20]; //001d3af0
    undefined OBJ_CCharBase___bbcmd_17004_AttackDefaults_StandingDD[0x120]; //001d3b10
    undefined OBJ_CCharBase___bbcmd_14078_HitOrBlockJumpCancel[0x20]; //001d3c30
    undefined OBJ_CCharBase___bbcmd_14081_HitJumpCancel[0x20]; //001d3c50
    undefined OBJ_CCharBase___bbcmd_12018_HitSprites[0xa0]; //001d3c70
    undefined OBJ_CCharBase___bbcmd_14079_SpecialCancel[0x20]; //001d3d10
    undefined OBJ_CCharBase___bbcmd_14082_SpecialCancelOnhit[0x93e0]; //001d3d30
    undefined BG_CBascule___get_DAT_005f9010[0x50]; //001dd110
    undefined BG_CBridge___get_DAT_005f9388[0xf0]; //001dd160
    undefined BG_CBridge_Night___get_DAT_005f9660[0x140]; //001dd250
    undefined BG_CChinatown___get_DAT_005f9938[0xe0]; //001dd390
    undefined BG_CChinatown_Night___get_DAT_005f9c10[0x2e0]; //001dd470
    undefined BG_CChurch___get_DAT_005f9ee8[0xb0]; //001dd750
    undefined BG_CCircus___get_DAT_005fa1c0[0x3c0]; //001dd800
    undefined BG_CGarden___get_DAT_005fa498[0x290]; //001ddbc0
    undefined BG_CGate_1___return_0[0x2b0]; //001dde50
    undefined BG_CGate_2___get_DAT_005faa20[0x1a60]; //001de100
    undefined BG_CMain_Daylight___get_DAT_005fb7f8[0x290]; //001dfb60
    undefined BG_CMain_Night___get_DAT_005fbad0[0xf10]; //001dfdf0
    undefined BG_CMain_Rain___get_DAT_005fbe48[0x310]; //001e0d00
    undefined BG_CMonolis___get_DAT_005fc1c0[0x600]; //001e1010
    undefined BG_CPipe___get_DAT_005fc498[0x620]; //001e1610
    undefined BG_CPipe_2___get_DAT_005fc788[0x1c0]; //001e1c30
    undefined BG_CTown___get_DAT_005fca60[0x190]; //001e1df0
    undefined BG_CVillage___get_DAT_005fcd38[0x54a0]; //001e1f80
    undefined BB_CFontFactory_Cockpit___create[0xb410]; //001e7420
    undefined SCENE_TestRingCommand___init_8_1[0x2d0]; //001f2830
    undefined SCENE_TestRingCommand___update_scene[0x860]; //001f2b00
    undefined SCENE_TestRingCommand___init_7[0x30]; //001f3360
    undefined SCENE_TestRingCommand___init_3[0x2ca0]; //001f3390
    undefined SCENE_CTestMyRoom___init_8_1[0x470]; //001f6030
    undefined SCENE_CTestMyRoom___set_next_scene[0x60]; //001f64a0
    undefined SCENE_CTestMyRoom___update_scene[0xa0]; //001f6500
    undefined SCENE_CDcc___init_7[0x60]; //001f65a0
    undefined SCENE_CTestMyRoom___init_3[0xb20]; //001f6600
    undefined SCENE_CTitleLoop___init_8_1[0x1670]; //001f7120
    undefined SCENE_CTitleLoop___set_next_scene[0x190]; //001f8790
    undefined SCENE_CTitleLoop___update_scene[0xe0]; //001f8920
    undefined SCENE_CTitleLoop___init_8_2[0xd0]; //001f8a00
    undefined SCENE_CTitleLoop___init_7[0x850]; //001f8ad0
    undefined SCENE_CTitleLoop___init_3[0x7ce]; //001f9320
    undefined inject_GetGameStateTitleScreen[0x2462]; //001f9aee
    undefined AA_CFiler___return_0[0x10]; //001fbf50
    undefined BB_CFontFactory_Group___create[0x4dc0]; //001fbf60
    undefined BB_CFontFactory_CharSelect___create[0x7ff0]; //00200d20
    undefined CharSelect__ctor[0xdd0]; //00208d10
    undefined CharSelect___check_button_down[0x170]; //00209ae0
    undefined CharSelect___reset[0x770]; //00209c50
    undefined _get_DAT_00e3eb40[0x30]; //0020a3c0
    undefined CharSelect___init[0x12e0]; //0020a3f0
    undefined _ctor_for_stage_select_[0xa]; //0020b6d0
    undefined inject_GetStageSelectAddr[0xb6]; //0020b6da
    undefined CharSelect___set_stage_cursor[0xe0]; //0020b790
    undefined CharSelect___update_music_select[0x460]; //0020b870
    undefined CharSelect___update_char_select[0x1560]; //0020bcd0
    undefined CharSelect___update_stage_select[0x1730]; //0020d230
    undefined CharSelect___update[0x35b0]; //0020e960
    undefined SCENE_CCharSelect___init_8_1[0x1aa]; //00211f10
    undefined inject_OverwriteStagesList[0x476]; //002120ba
    undefined SCENE_CCharSelect___set_next_scene[0x780]; //00212530
    undefined SCENE_CCharSelect___update_scene[0x1b0]; //00212cb0
    undefined SCENE_CTitleLoop___init_2[0x30]; //00212e60
    undefined SCENE_CCharSelect___init_8_2[0x40]; //00212e90
    undefined SCENE_CCharSelect___init_7[0x280]; //00212ed0
    undefined SCENE_CCharSelect___init_3[0xa2]; //00213150
    undefined inject_GetGameStateCharacterSelect[0x113e]; //002131f2
    undefined SCENE_CStageInfo___init_8_1[0x1040]; //00214330
    undefined SCENE_CStageInfo___set_next_scene[0x60]; //00215370
    undefined SCENE_CStageInfo___update_scene[0xd0]; //002153d0
    undefined SCENE_CStageInfo___init_8_2[0xc0]; //002154a0
    undefined SCENE_CStageInfo___init_7[0x130]; //00215560
    undefined SCENE_CStageInfo___init_3[0x3d30]; //00215690
    undefined BB_CFontFactory_StageInfo___create[0xd00]; //002193c0
    undefined SCENE_CVSInfo__ctor[0x160]; //0021a0c0
    undefined SCENE_CVSInfo__dtor[0x100]; //0021a220
    undefined SCENE_CVSInfo___dtor[0x30]; //0021a320
    undefined SCENE_CVSInfo___init_8_1[0x13d0]; //0021a350
    undefined SCENE_CVSInfo___set_next_scene[0xe0]; //0021b720
    undefined SCENE_CVSInfo___update_scene[0xc0]; //0021b800
    undefined SCENE_CVSInfo___init_8_2[0xc0]; //0021b8c0
    undefined SCENE_CVSInfo___init_7[0x230]; //0021b980
    undefined SCENE_CVSInfo__func_GSS_3[0x2e]; //0021bbb0
    undefined inject_GetGameStateVersusScreen[0x39a2]; //0021bbde
    undefined SCENE_CVSInfo___create_CLoadTask[0x5c0]; //0021f580
    undefined CharSelect_CBase___empty_func[0x71c0]; //0021fb40
    undefined SCENE_CWinner___init_8_1[0x1a60]; //00226d00
    undefined SCENE_CWinner___set_next_scene[0x450]; //00228760
    undefined SCENE_CWinner___update_scene[0x110]; //00228bb0
    undefined SCENE_CWinner___init_8_2[0x110]; //00228cc0
    undefined SCENE_CWinner___init_7[0x1a0]; //00228dd0
    undefined SCENE_CWinner___init_3[0x39]; //00228f70
    undefined inject_GetGameStateVictoryScreen[0xcdd7]; //00228fa9
    undefined SCENE_CEnding___init_8_1[0x4a0]; //00235d80
    undefined SCENE_CEnding___set_next_scene[0x140]; //00236220
    undefined SCENE_CEnding___update_scene[0x80]; //00236360
    undefined SCENE_CEnding___init_8_2[0x2a0]; //002363e0
    undefined SCENE_CEnding___init_7[0x190]; //00236680
    undefined SCENE_CEnding___init_5[0xc10]; //00236810
    undefined SCENE_CEnding___init_4[0x20]; //00237420
    undefined SCENE_CEnding___init_3[0x970]; //00237440
    undefined GAME_CETCManager___calls_FUN_002425d0_on_etc_object_field_0x4[0x87f0]; //00237db0
    undefined GAME_CETCManager__static_init[0x340]; //002405a0
    undefined GAME_CETCManager__ctor[0x910]; //002408e0
    undefined Cockpit_CBase__dtor[0x310]; //002411f0
    undefined GAME_CETCManager__dtor[0x300]; //00241500
    undefined MainMenu_CBase__dtor[0x560]; //00241800
    undefined Cockpit_CBase__dtor_1[0x270]; //00241d60
    undefined MainMenu_CBase__dtor_1[0x600]; //00241fd0
    undefined GAME_CETCManager___CreateObjectByID_[0x310]; //002425d0
    undefined GAME_CETCManager___calls_CETCObject_vftable_3_[0x1260]; //002428e0
    undefined get_GAME_CETCManager[0xa0]; //00243b40
    undefined GAME_CETCManager___get_CETCObject_by_game_scene[0x820]; //00243be0
    undefined GAME_CETCObjParam___init[0x23950]; //00244400
    undefined GAME_CBB4VersionInfo1_01_00___get_switchdataD_00402774[0x80]; //00267d50
    undefined _get_GAME_CBB4VersionInfo[0x3b0]; //00267dd0
    undefined _return_0_3[0x10]; //00268180
    undefined GAMESTEAM_CDLCManager__maybe_log_game_state[0x2d30]; //00268190
    undefined BB_CFontFactory_Profile___create[0xc10]; //0026aec0
    undefined SCENE_CProfile___init_8_1[0x1240]; //0026bad0
    undefined SCENE_CProfile___set_next_scene[0xc0]; //0026cd10
    undefined SCENE_CProfile___update_scene[0x60]; //0026cdd0
    undefined SCENE_CProfile___init_7[0xe0]; //0026ce30
    undefined SCENE_CProfile___init_3[0xa90]; //0026cf10
    undefined SCENE_CExtra___init_8_1[0x1b0]; //0026d9a0
    undefined SCENE_CExtra___set_next_scene[0x60]; //0026db50
    undefined SCENE_CExtra___update_scene[0x190]; //0026dbb0
    undefined SCENE_CExtra___init_8_2[0x170]; //0026dd40
    undefined SCENE_CExtra___init_7[0xa0]; //0026deb0
    undefined SCENE_CExtra___init_3[0x660]; //0026df50
    undefined _get_DAT_00ef4ae8[0x4050]; //0026e5b0
    undefined SCENE_CAbyssUI___dtor[0x30]; //00272600
    undefined _get_DAT_00f6ba30[0x3d60]; //00272630
    undefined SCENE_CAbyssUI___init_8_1[0xb590]; //00276390
    undefined SCENE_CAbyssUI___set_next_scene[0x440]; //00281920
    undefined SCENE_CAbyssUI___update_scene[0x1cf0]; //00281d60
    undefined SCENE_CAbyssUI___init_8_2[0x100]; //00283a50
    undefined SCENE_CAbyssUI___init_7[0x1590]; //00283b50
    undefined SCENE_CAbyssUI___init_3[0xd130]; //002850e0
    undefined _play_menu_cancel_wav[0x150]; //00292210
    undefined _play_menu_cursor_a_wav[0x70]; //00292360
    undefined _play_menu_error_wav[0x70]; //002923d0
    undefined _play_menu_ok_wav[0x2a0]; //00292440
    undefined _handle_msg_input[0x3010]; //002926e0
    undefined _get_DAT_01100374_events_[0x10]; //002956f0
    undefined _get_DAT_01100374_events_1[0x130]; //00295700
    undefined _get_MenuMessage_n[0x1d0]; //00295830
    undefined _get_DAT_01100bd8_errors_[0xc0]; //00295a00
    undefined _get_DAT_00623674_palettes_1[0x10]; //00295ac0
    undefined _get_DAT_01015a28_item[0x20]; //00295ad0
    undefined _get__s_MsgWindow_006043e4_index[0x15e0]; //00295af0
    undefined _ctor_for_DAT_013b02f0_item[0x960]; //002970d0
    undefined MenuMessage___show_[0x160]; //00297a30
    undefined _DAT_01015a28_PushUIExclusiveName_[0x770]; //00297b90
    undefined _ctor_00298300_MSGW_OK[0x40]; //00298300
    undefined MenuMessage___init_yes_no_[0xa0]; //00298340
    undefined _show_error_[0x1410]; //002983e0
    undefined _DAT_01015a28_PushUIExclusiveName_1_[0x1190]; //002997f0
    undefined _get_DAT_00604950[0x11f0]; //0029a980
    undefined CBattleReplayDataManager__ctor[0x120]; //0029bb70
    undefined ReplayFile__pack_replay[0x1d0]; //0029bc90
    undefined CBattleReplayDataManager___pack_replay[0x830]; //0029be60
    undefined CBattleReplayDataManager___init_header_[0x2a0]; //0029c690
    undefined ReplayFile__unpack_replay[0xe0]; //0029c930
    undefined CBattleReplayDataManager___unpack_replay[0x8c0]; //0029ca10
    undefined CBattleReplayDataManager___get_replay[0x10]; //0029d2d0
    undefined get_CBattleReplayDataManager[0x60]; //0029d2e0
    undefined CBattleReplayDataManager___get_replay_buffer[0x30]; //0029d340
    undefined CBattleReplayDataManager___get_playback_speed[0x620]; //0029d370
    undefined NetworkStruct___get_0x270_1[0x60]; //0029d990
    undefined ReplayFileHeader___replay_version_lt7[0x50]; //0029d9f0
    undefined _mode_is_Versus_and_bsave_0x30_lt5[0x40]; //0029da40
    undefined CBattleReplayDataManager__get_playback_input_for_player[0x3f0]; //0029da80
    undefined CBattleReplayDataManager___init_GameVals_player_data[0xf80]; //0029de70
    undefined _get_DAT_011c47c0[0x10]; //0029edf0
    undefined _get_DAT_011cea10[0x10]; //0029ee00
    undefined _get_DAT_011cec00[0x5860]; //0029ee10
    undefined _get_CCommandListManager[0x34b0]; //002a4670
    undefined _get_DAT_0121bf80[0xb0]; //002a7b20
    undefined _get_DAT_0121bfa8[0x21d0]; //002a7bd0
    undefined _return_0_4[0x10]; //002a9da0
    undefined _empty_func_2[0x210]; //002a9db0
    undefined SCENE_CLibraryMode___dtor[0xa0]; //002a9fc0
    undefined SCENE_CLibraryMode___init_8_1[0x1cf0]; //002aa060
    undefined SCENE_CLibraryMode___set_next_scene[0x80]; //002abd50
    undefined SCENE_CLibraryMode___update_scene[0xd30]; //002abdd0
    undefined SCENE_CLibraryMode___init_8_2[0xc0]; //002acb00
    undefined SCENE_CLibraryMode___init_7[0x900]; //002acbc0
    undefined SCENE_CLibraryMode___init_3[0x1840]; //002ad4c0
    undefined AA_CFont_Tool___ctor_1[0x1b0]; //002aed00
    undefined AA_CFont_Tool___ctor_2[0x32b0]; //002aeeb0
    undefined _get_DAT_0121e450[0x3760]; //002b2160
    undefined SCENE_CMatchResult___init_8_1[0xd0]; //002b58c0
    undefined SCENE_CMatchResult___set_next_scene[0x90]; //002b5990
    undefined SCENE_CMatchResult___init_8_2[0x100]; //002b5a20
    undefined _set_flag[0xc1f0]; //002b5b20
    undefined CMenuItemManager___update_all[0x160]; //002c1d10
    undefined SCENE_CReplayTheater__dtor_1[0xa0]; //002c1e70
    undefined SCENE_CReplayTheater__dtor[0x30]; //002c1f10
    undefined SCENE_CReplayTheater___init_8_1[0xa20]; //002c1f40
    undefined SCENE_CReplayTheater__set_next_scene[0x150]; //002c2960
    undefined SCENE_CReplayTheater___update_menu[0x48]; //002c2ab0
    undefined inject_BeforeWriteReplayListDat[0x477]; //002c2af8
    undefined inject_DelNetworkReqWatchReplays[0x3b1]; //002c2f6f
    undefined SCENE_CReplayTheater__update_scene[0x70]; //002c3320
    undefined _ctor_for_replay_list_menu[0xc0]; //002c3390
    undefined SCENE_CReplayTheater___init_7[0xd0]; //002c3450
    undefined SCENE_CReplayTheater__init[0x30]; //002c3520
    undefined inject_GetGameStateReplayMenuScreen[0x540]; //002c3550
    undefined AutoSaveStruct__ctor[0x1f0]; //002c3a90
    undefined AutoSaveStruct___set_action_0x3a[0xc0]; //002c3c80
    undefined AutoSaveStruct___is_done[0x60]; //002c3d40
    undefined AutoSaveStruct___has_action[0x70]; //002c3da0
    undefined AutoSaveStruct___check_flags[0x90]; //002c3e10
    undefined AutoSaveStruct___set_save_flag_2_replay[0x80]; //002c3ea0
    undefined AutoSaveStruct___set_save_flag_4_replay_list[0x90]; //002c3f20
    undefined AutoSaveStruct___set_action_0x1d[0xc0]; //002c3fb0
    undefined AutoSaveStruct___set_action_0x2d[0xc0]; //002c4070
    undefined AutoSaveStruct___set_action_0x21_read_replay[0xc0]; //002c4130
    undefined AutoSaveStruct___set_action_0x28[0xd0]; //002c41f0
    undefined AutoSaveStruct___set_action_1_1_[0x70]; //002c42c0
    undefined AutoSaveStruct___set_save_flag_1_bbsave[0xa0]; //002c4330
    undefined AutoSaveStruct___set_action_1[0xe0]; //002c43d0
    undefined AutoSaveStruct___set_action_1_2_[0x70]; //002c44b0
    undefined AutoSaveStruct___return_1_[0x80]; //002c4520
    undefined AutoSaveStruct___set_action_0x44[0xd0]; //002c45a0
    undefined AutoSaveStruct___set_action_0x3b[0x210]; //002c4670
    undefined AutoSaveStruct___update_1[0xa0]; //002c4880
    undefined AutoSaveStruct___set_action[0x70]; //002c4920
    undefined AutoSaveStruct___update[0x24c0]; //002c4990
    undefined AutoSaveStruct___update_save_util[0x43a0]; //002c6e50
    undefined _get_training_playback_input[0x2a0]; //002cb1f0
    undefined _get_training_input_TRI_Posing_[0xb00]; //002cb490
    undefined _get_menu_item_values_by_name_or_default_[0x160]; //002cbf90
    undefined get_menu_item_values[0x310]; //002cc0f0
    undefined _input_is_for_other_player_[0x50]; //002cc400
    undefined _battle_is_paused_[0x50]; //002cc450
    undefined GameVals___mode_is_ReplayTheater[0x80]; //002cc4a0
    undefined GameVals___mode_is_Training[0xb30]; //002cc520
    undefined _update_replay_theater_menu_items_[0x570]; //002cd050
    undefined _set_menu_item_values_by_name[0x140]; //002cd5c0
    undefined set_menu_item_values[0x80]; //002cd700
    undefined _update_training_menu_items_[0x2d10]; //002cd780
    undefined _get_DAT_01392d10_pause_[0x3080]; //002d0490
    undefined _get_menu_item_configs_by_name_013ae3a8[0x1c0]; //002d3510
    undefined _maybe_get_training_menu_default_values_[0xa30]; //002d36d0
    undefined NetworkStruct___get_0x278__1_[0x1a90]; //002d4100
    undefined _get_DAT_013b02f0[0x1520]; //002d5b90
    undefined _get_menu_item_configs_by_name_013cb7f8[0x4810]; //002d70b0
    undefined _GameMode__Challenge[0x190]; //002db8c0
    undefined _GameMode__Tutorial[0x1890]; //002dba50
    undefined _get_DAT_013cb888[0x1140]; //002dd2e0
    undefined SCENE_CTutorialUI___init_8_1[0x560]; //002de420
    undefined SCENE_CTutorialUI___set_next_scene[0x280]; //002de980
    undefined SCENE_CTutorialUI___update_scene[0x1050]; //002dec00
    undefined SCENE_CTutorialUI___init_7[0x230]; //002dfc50
    undefined SCENE_CTutorialUI___init_3[0x250]; //002dfe80
    undefined _get_PTR_vftable_006070f0[0x300]; //002e00d0
    undefined _get_DAT_0141d9f8[0x2030]; //002e03d0
    undefined SCENE_CLegion___init_8_1[0x6190]; //002e2400
    undefined SCENE_CLegion___set_next_scene[0x250]; //002e8590
    undefined SCENE_CLegion___update_scene[0xb50]; //002e87e0
    undefined SCENE_CLegion___init_7[0x2240]; //002e9330
    undefined AAWin_CFileReader_Thread___vftable_0xe__returns_0[0x5440]; //002eb570
    undefined BB_CFontFactory_ModeSelect___create[0x4990]; //002f09b0
    undefined SCENE_CModeSelect___init_8_1[0x670]; //002f5340
    undefined SCENE_CModeSelect___set_next_scene[0x3e0]; //002f59b0
    undefined SCENE_CModeSelect___update_scene[0x90]; //002f5d90
    undefined SCENE_CProfile___init_8_2[0x40]; //002f5e20
    undefined SCENE_CModeSelect___init_7[0x170]; //002f5e60
    undefined SCENE_CModeSelect___init_3[0x5e0]; //002f5fd0
    undefined SCENE_CStaffRoll___init_8_1[0x540]; //002f65b0
    undefined SCENE_CStaffRoll___set_next_scene[0x150]; //002f6af0
    undefined SCENE_CStaffRoll___update_scene[0xb0]; //002f6c40
    undefined SCENE_CStaffRoll___init_8_2[0x190]; //002f6cf0
    undefined SCENE_CStaffRoll___init_7[0xf0]; //002f6e80
    undefined SCENE_CStaffRoll___init_3[0xa200]; //002f6f70
    undefined SCENE_CRanking___init_8_1[0x7c0]; //00301170
    undefined SCENE_CRanking___set_next_scene[0x60]; //00301930
    undefined SCENE_CRanking___update_scene[0x1e0]; //00301990
    undefined SCENE_CRanking___init_8_2[0x30]; //00301b70
    undefined SCENE_CRanking___init_7[0xc0]; //00301ba0
    undefined BB_CFontFactory_Ranking___create[0x170]; //00301c60
    undefined SCENE_CRanking___init_3[0x4ca0]; //00301dd0
    undefined SCENE_CResult___init_8_1[0xd00]; //00306a70
    undefined SCENE_TestRingCommand___set_next_scene[0xe0]; //00307770
    undefined SCENE_CResult___update_scene[0xf0]; //00307850
    undefined SCENE_CResult___init_8_2[0x90]; //00307940
    undefined SCENE_CResult___init_7[0x460]; //003079d0
    undefined SCENE_CResult___init_3[0x690]; //00307e30
    undefined SCENE_CWarning___init_8_1[0x420]; //003084c0
    undefined SCENE_CWarning___set_next_scene[0x60]; //003088e0
    undefined SCENE_CWarning___init_7[0x30]; //00308940
    undefined SCENE_CWarning___init_3[0x1a0]; //00308970
    undefined MainMenu__ctor[0x1b0]; //00308b10
    undefined SubMenu__ctor[0x33f0]; //00308cc0
    undefined _replay_list_view_go_down[0x60]; //0030c0b0
    undefined _set_DAT_0141e738_1[0x10]; //0030c110
    undefined MainMenu__update[0x1650]; //0030c120
    undefined _replay_list_view_reset_1[0x70]; //0030d770
    undefined _replay_list_view_reset[0x2920]; //0030d7e0
    undefined _replay_list_view_go_up[0x2480]; //00310100
    undefined MainMenu___update_controller_select[0x5810]; //00312580
    undefined MainMenu___update_lobby_list[0x2340]; //00317d90
    undefined MainMenu___update_sub_menu[0x4d0]; //0031a0d0
    undefined MainMenu__run_selected_menu_item[0x6f0]; //0031a5a0
    undefined MainMenu___update_main_menu[0x51a0]; //0031ac90
    undefined BB_CFontFactory_MainMenu___create[0x9100]; //0031fe30
    undefined SCENE_CMainMenuEX__ctor[0x100]; //00328f30
    undefined SCENE_CMainMenuEX__dtor[0x6b0]; //00329030
    undefined SCENE_CMainMenuEX___init_8_1[0xcd0]; //003296e0
    undefined SCENE_CMainMenuEX__SetNextGameScene[0x690]; //0032a3b0
    undefined SCENE_CMainMenuEX__update_scene[0x1fe0]; //0032aa40
    undefined GameVals___reset_for_Training[0x4d0]; //0032ca20
    undefined SCENE_CMainMenuEX___init_8_2[0xc0]; //0032cef0
    undefined SCENE_CMainMenuEX___init_7[0xb0]; //0032cfb0
    undefined GameVals___set_controller_indices[0x3b]; //0032d060
    undefined inject_GetIsP1CPU[0xc75]; //0032d09b
    undefined SCENE_CMainMenuEX__load_[0x23]; //0032dd10
    undefined inject_GetGameStateMenuScreen[0x256d]; //0032dd33
    undefined SCENE_CStorySelect___init_8_1[0x620]; //003302a0
    undefined SCENE_CStorySelect___set_next_scene[0x310]; //003308c0
    undefined SCENE_CStorySelect___update_scene[0xf0]; //00330bd0
    undefined SCENE_CStorySelect___init_8_2[0xc0]; //00330cc0
    undefined SCENE_CStorySelect___init_7[0xa0]; //00330d80
    undefined SCENE_CStorySelect___init_3[0x5f80]; //00330e20
    undefined BB_CFontFactory_StorySelect___create[0x128e0]; //00336da0
    undefined SCENE_CLobby___init_8_1[0x1d10]; //00349680
    undefined SCENE_CLobby__SetNextGameScene[0x70]; //0034b390
    undefined SCENE_CLobby___update_scene[0x110]; //0034b400
    undefined SCENE_CLobby___init_8_2[0xc0]; //0034b510
    undefined SCENE_CLobby___init_7[0x220]; //0034b5d0
    undefined SCENE_CLobby___init_3[0x21]; //0034b7f0
    undefined inject_GetGameStateLobby[0x107f]; //0034b811
    undefined GAME_CEventManager__ctor[0x250]; //0034c890
    undefined get_GAME_CEventManager[0x160]; //0034cae0
    undefined GAME_CEventControlTask__dtor[0x60]; //0034cc40
    undefined GAME_CEventControlTask__update_task[0x47c0]; //0034cca0
    undefined _empty_func_3[0x1f40]; //00351460
    undefined CAdvCtrTask___dtor[0xa50]; //003533a0
    undefined CAdvCtrTask___update_task[0x4930]; //00353df0
    undefined _get_DAT_0142a510[0x1250]; //00358720
    undefined _FUN_00359980_1[0x820]; //00359970
    undefined _get_DAT_0142a560_adv_ctrl_[0x1eb70]; //0035a190
    undefined _CryptHashData_[0x100]; //00378d00
    undefined _CryptDecrypt_[0x590]; //00378e00
    undefined _get_DAT_0142a764[0x6c0]; //00379390
    undefined AudioDeviceControlManager___return_0[0x2cd0]; //00379a50
    undefined SteamPeer2PeerBackend__GoesToSetDisconnectNotifyStart[0x80]; //0037c720
    undefined maybe_ggpo_start_session_with_SteamPeer2PeerBackend[0x80]; //0037c7a0
    undefined SteamSpectatorBackend__maybe_CreateSteamSpectatorBackend[0x100]; //0037c820
    undefined SteamPeer2PeerBackend__maybe_init[0x420]; //0037c920
    undefined SteamPeer2PeerBackend__AddLocalInput[0x390]; //0037cd40
    undefined SteamPeer2PeerBackend__DisconnectPlayer[0xe0]; //0037d0d0
    undefined SteamPeer2PeerBackend__DisconnectPlayerQueue[0xd0]; //0037d1b0
    undefined SteamPeer2PeerBackend__DoPoll[0xae0]; //0037d280
    undefined SteamPeer2PeerBackend__SetDisconnectTimeout[0x1020]; //0037dd60
    undefined GameInput__init[0x110]; //0037ed80
    undefined InputQueue__AddDelayedInputToQueue[0x1e0]; //0037ee90
    undefined InputQueue__AddInput[0xb0]; //0037f070
    undefined InputQueue__AdvanceQueueHead[0x600]; //0037f120
    undefined SteamUdp__maybe_init[0x2b0]; //0037f720
    undefined SteamUdpProtocol__maybe_init[0x360]; //0037f9d0
    undefined SteamUdpProtocol__maybe_init_1[0xa50]; //0037fd30
    undefined UdpMsg__PacketSize[0xac0]; //00380780
    undefined SteamUdpProtocol__maybe_Synchronize[0x650]; //00381240
    undefined _assert_fail_msg_ggpo[0xe0]; //00381890
    undefined Poll__maybe_init[0x1060]; //00381970
    undefined Sync__maybe_ChangeSomethingInput[0x410]; //003829d0
    undefined TimeSync__maybe_init[0x80]; //00382de0
    undefined maybe_TimeSync__advance_frame[0x1a0]; //00382e60
    undefined GGPOPerformanceMonitor__maybe_init[0x360]; //00383000
    undefined BackendStaticStruct__ctor[0x40]; //00383360
    undefined BackendStaticStruct__get_probable_struct_ptr[0x90]; //003833a0
    undefined maybe_BackendStaticStruct_idk[0x180]; //00383430
    undefined BackendStaticStruct___init[0x1a0]; //003835b0
    undefined BackendStaticStruct__maybe_session_init_idk[0x1f0]; //00383750
    undefined PerformanceMonitor__maybe_initialize_spectate_session[0xc0]; //00383940
    undefined BackendStaticStruct__maybe_init[0x3d0]; //00383a00
    undefined initialize_ggpo_callbacks_struct[0xe0]; //00383dd0
    undefined maybe_advance_frame[0x50]; //00383eb0
    undefined maybe_free_buffer[0x20]; //00383f00
    undefined load_game_state[0x390]; //00383f20
    undefined maybe_save_game_state[0x120]; //003842b0
    undefined important_to_initialize_statics_for_session[0x1220]; //003843d0
    undefined maybe_manager_static_part_of_init_FUN_010055f0[0x310]; //003855f0
    undefined manager_static_add_manager_1_FUN_01005900[0x20]; //00385900
    undefined manager_static_add_to_ptr_manager_list_1_FUN_01005920[0xa0]; //00385920
    undefined manager_static_add_manager_2_FUN_010059c0[0x20]; //003859c0
    undefined _dtor_for_DAT_00612718_[0x640]; //003859e0
    undefined SnapshotManager__maybe_init[0x1c0]; //00386020
    undefined _memcpy_2[0x290]; //003861e0
    undefined maybe_get_size_saved_buf[0x50]; //00386470
    undefined savestates_snapshot_allocate_memory[0x7e0]; //003864c0
    undefined large_memcpy_FUN_01006ca0[0x30]; //00386ca0
    undefined maybe_copies_state[0x4f0]; //00386cd0
    undefined _assert_fail_and_exit[0x2a9]; //003871c0
    undefined _operator_new_1[0x16c6c]; //00387469
    undefined _free_1_1[0x78f]; //0039e0d5
    undefined _free_1_2[0x45c]; //0039e864
    undefined _memcpy_1[0xa00]; //0039ecc0
    undefined _float_round_[0x180]; //0039f6c0
    undefined _memset_[0x1fc]; //0039f840
    undefined _atol_1[0x15f9]; //0039fa3c
    undefined _free_1_3[0x1ea5]; //003a1035
    undefined maybe_rand__[0x28b]; //003a2eda
    undefined _HeapCompact[0x1389]; //003a3165
    undefined _get_PTR_DAT_0060e180[0x12a1]; //003a44ee
    undefined _empty_func_[0xa97]; //003a578f
    undefined _DeleteCriticalSection_1[0x224]; //003a6226
    undefined _EnterCriticalSection_1[0x150]; //003a644a
    undefined _LeaveCriticalSection_1_1[0xb302]; //003a659a
    undefined _get_DAT_0060ea04[6]; //003b189c
    undefined _get_DAT_0060ea08[6]; //003b18a2
    undefined _get_DAT_0060ea00[6]; //003b18a8
    undefined _get_PTR_DAT_0060ea90[0xa966]; //003b18ae
    undefined _TlsGetValue_01ccb818[0x13aa]; //003bc214
    undefined _TlsGetValue_01ccb818_1[0x110d]; //003bd5be
    undefined _empty_func_4[0x17b51]; //003be6cb
    undefined _DebugMallocator_ltclass_std___Ref_count_del_alloc_ltclass___ExceptionPtr_void____cdecl___class___ExceptionPtr___class__DebugMallocator_ltint_gt__gt__gt___allocate[0x3e884]; //003d621c
    undefined AAWin_CFileReader_Thread___uncompress_pac_[0x7760]; //00414aa0
    undefined _get_DAT_00611e94[0x419e]; //0041c200
    undefined _get_PTR_s_No_error_005a4de8[6]; //0042039e
    undefined _get_DAT_00611ea0[0x2321c]; //004203a4
    undefined static___init_battle_pause_menu_items[0x3330]; //004435c0
    undefined static___init_DAT_013b02f0[0x13e0]; //004468f0
    undefined _empty_func_5[0x1a0]; //00447cd0
    undefined AA_CRandomManager__dtor[0x210]; //00447e70
    undefined _empty_func_6[0x30]; //00448080
    undefined AASTEAM_CNetworker__dtor_1[0x230]; //004480b0
    undefined _empty_func_7[0x190]; //004482e0
    undefined _empty_func_8[0x10]; //00448470
    undefined _empty_func_9[0x1a0]; //00448480
    undefined _empty_func_10[0x30]; //00448620
    undefined _empty_func_11[0x60]; //00448650
    undefined _empty_func_12[0x40]; //004486b0
    undefined _empty_func_13[0x1b0]; //004486f0
    undefined _empty_func_14[0x20]; //004488a0
    undefined _empty_func_15[0xf0]; //004488c0
    undefined _empty_func_16[0x50]; //004489b0
    undefined _empty_func_17[0xe0]; //00448a00
    undefined _empty_func_18[0x10]; //00448ae0
    undefined game_Stat_PCoinManager___init_1[0x10]; //00448af0
    undefined _empty_func_19[0x10]; //00448b00
    undefined _empty_func_20[0x1a0]; //00448b10
    undefined _empty_func_21[0x10]; //00448cb0
    undefined _empty_func_22[0x320]; //00448cc0
    undefined _empty_func_23[0x40]; //00448fe0
    undefined _empty_func_24[0x1e0]; //00449020
    undefined _empty_func_25[0xe0]; //00449200
    undefined _empty_func_26[0x1380]; //004492e0
    undefined4 _static_initializer_table; //0044a660
    undefined __44a664[0x45e0];
    pointer s_empty_; //0044ec44
    undefined __44ec48[0x103140];
    int _static_bbscript_cmd_lengths[0x41a][2]; //00551d88
    undefined __553e58[0x7f3c0];
    struct SteamInterfaces static_SteamInterfaces; //005d3218
    undefined __5d326c[0xd9ec];
    struct SaveUtil static_SaveUtil; //005e0c58
    undefined __5e0da8[0x31afc];
    undefined _static_AA_CCameraManager[0x10]; //006128a4
    undefined __6128b4[8];
    struct FileReaderStruct _static_FileReaderStruct; //006128bc
    undefined __6128d8[0x44];
    struct CInputList _static_CInputList; //0061291c
    undefined __61292c[0x20];
    undefined _static_AA_CModelInstanceManager[0x24]; //0061294c
    undefined __612970[0x90];
    undefined _static_AA_CParticleManager[0x60]; //00612a00
    undefined __612a60[0xb60];
    undefined _static_AA_CRandomManager[0x10184]; //006135c0
    undefined4 _static_AA_CFriendList_ptr; //00623744
    undefined __623748[0x1e6c];
    undefined _static_DInput8Struct[0xc]; //006255b4
    undefined __6255c0[0x1c8];
    struct AASTEAM_CNetworker static_AASTEAM_CNetworker; //00625788
    undefined __6291d8[8];
    undefined _lobby_search_results[0x40][0x30]; //006291e0
    int _lobby_search_results_last; //00629de0
    undefined __629de4[0x1c];
    undefined4 _static_AASTEAM_CRankingReader; //00629e00
    undefined __629e04[0x1c];
    undefined4 _static_AASTEAM_CReplayUploader_ptr; //00629e20
    undefined __629e24[0x18];
    undefined4 _static_AASTEAM_CVoiceChatManager_ptr; //00629e3c
    undefined __629e40[8];
    undefined _static_AASTEAM_CVoiceSound[0x18]; //00629e48
    undefined __629e60[0x306a0];
    struct AADX_CRenderer _static_AADX_CRenderer; //0065a500
    undefined __65b748[0x48];
    undefined _static_AADX_CRenderInserter[4]; //0065b790
    undefined __65b794[4];
    undefined _static_AADX_CRenderInserter_Less[4]; //0065b798
    undefined __65b79c[0x69c];
    undefined4 _static_AAWin_HQTimer; //0065be38
    undefined __65be3c[0x304];
    struct AAWin_CFileReader_Thread _static_AAWin_CFileReader_Thread; //0065c140
    undefined __65c198[0x310];
    undefined _static_AudioDeviceControlManager[0x1bc]; //0065c4a8
    undefined _static_GAME_SynthKeyControler[8]; //0065c664
    undefined __65c66c[4];
    undefined _static_GAME_SynthKeyControler_1[8]; //0065c670
    undefined _static_GAME_BattleSynthKeyControler[8]; //0065c678
    undefined __65c680[4];
    undefined _static_GAME_BattleSynthKeyControler_1[8]; //0065c684
    undefined __65c68c[0xa8c];
    undefined _static_GAMESTEAM_ImeEditBox[0x108]; //0065d118
    undefined __65d220[0x50];
    undefined _static_CSTEAMNetworkLobbyData[0x5270]; //0065d270
    undefined __6624e0[0x250];
    struct GAMESTEAM_CNetworkServer static_GAMESTEAM_CNetworkServer; //00662730
    undefined __664d20[0x98];
    struct GAMESTEAM_VirtualKeyboard _static_GAMESTEAM_VirtualKeyboard; //00664db8
    undefined __664fdc[0x4c];
    undefined _static_GAME_CEventFaceDataManager[0xbc]; //00665028
    undefined __6650e4[0x11fb4];
    undefined _static_GAME_CEff3DInstHndlManager[0x217430]; //00677098
    struct Buffer static_FPACBuffers[0x3d4]; //0088e4c8
    undefined __890368[0x48];
    struct GameVals static_GameVals; //008903b0
    undefined __8929c0[8];
    struct AASTEAM_SystemManager* AASTEAM_SystemManager_ptr; //008929c8
    undefined4 AAJVS_SystemManager_ptr; //008929cc
    undefined __8929d0[0x48];
    undefined4 _static_GAME_EventManager; //00892a18
    undefined __892a1c[0x524c];
    undefined4 _static_AchievementCountManager; //00897c68
    undefined __897c6c[0xdc];
    undefined _static_AchievementManager[0xec]; //00897d48
    undefined __897e34[8];
    undefined4 _static_GAMESTEAM_CClassGenerator_ptr; //00897e3c
    undefined __897e40[0x150e8];
    undefined4 _static_GAMESTEAM_CDLCManager_ptr; //008acf28
    undefined __8acf2c[8];
    undefined _static_GAME_CFadeTaskManager[0x3c]; //008acf34
    undefined _static_CInstallManager_NULL[4]; //008acf70
    undefined __8acf74[0x14c];
    struct NetUserData static_NetUserData; //008ad0c0
    undefined __8f7708[0xe8];
    undefined _static_StatBattleTmp[0x168]; //008f77f0
    struct NetworkStruct static_NetworkStruct; //008f7958
    undefined __8f7d98[0x840];
    struct CSaveDataManager static_CSaveDataManager; //008f85d8
    undefined __abfe18[0x2b4];
    undefined4 _static_FileInstaller; //00ac00cc
    undefined __ac00d0[0x10];
    undefined _static_GAME_CTextEffectObject[0x1498]; //00ac00e0
    undefined __ac1578[8];
    undefined4 _static_GAMESTEAM_CVoiceChatManager_ptr; //00ac1580
    undefined __ac1584[0x1a14];
    undefined _static_BG_EffectManager[0x60]; //00ac2f98
    undefined* _static_AllianceModeEnemyData[2]; //00ac2ff8
    undefined __ac3000[0x50];
    undefined _static_CBattle_AllianceMode[0x4a4]; //00ac3050
    undefined __ac34f4[0x1b054];
    struct SettingsMenuItem _menu_item_configs_00ade548[0x1e]; //00ade548
    int _menu_item_order_00adebd8[0x1e]; //00adebd8
    undefined __adec50[0x1354];
    undefined _static_BATTLE_CObjectManagerMemPoolSerializerDummy[4]; //00adffa4
    undefined __adffa8[8];
    undefined _static_game_Stat_PCoinManager[0x304]; //00adffb0
    undefined __ae02b4[0x2c381c];
    undefined _static_CPlayerRoomMember[0xb68]; //00da3ad0
    undefined _static_CPlayerBattleInformation[0x708]; //00da4638
    undefined __da4d40[0x1378];
    undefined _static_CNetworkLobbyWorld[0x2cc]; //00da60b8
    undefined __da6384[0x34];
    undefined _static_CNetworkUILobbyRegionSelect[0x1764]; //00da63b8
    undefined __da7b1c[0x124];
    struct CNetworkUIPlayerMatchCreate static_CNetworkUIPlayerMatchCreate; //00da7c40
    undefined __da9424[4];
    undefined _static_CNetworkUIPlayerMatchSearch[0x17e4]; //00da9428
    undefined __daac0c[0x4c];
    undefined _static_CNetworkUIPlayerMatchTopMenu[0x2d0]; //00daac58
    undefined __daaf28[0x48];
    undefined _static_CNetworkUIRankMatchTopMenu[0x2cc]; //00daaf70
    undefined __dab23c[4];
    undefined _static_CNetworkUIRankParam[0x1750]; //00dab240
    undefined __dac990[0x48];
    undefined _static_CNetworkUIRankMatchCharSele[0x1bb4]; //00dac9d8
    undefined __dae58c[0x1b4];
    undefined static_BATTLE_CBGManager[0x82c8]; //00dae740
    undefined __db6a08[0xd8];
    struct BATTLE_CObjectManager static_BATTLE_CObjectManager; //00db6ae0
    undefined __e3a940[0x98];
    undefined _static_BATTLE_CScreenManager[0x4500]; //00e3a9d8
    undefined _static_Cockpit_CBase[0xc]; //00e3eed8
    undefined _static_TitleLoop_CBase[8]; //00e3eee4
    undefined _static_CharSelect_CBase[8]; //00e3eeec
    undefined _static_Profile_CBase[8]; //00e3eef4
    undefined _static_VSInfo_CBase[8]; //00e3eefc
    undefined _static_Winner_CBase[8]; //00e3ef04
    undefined _static_StageInfo_CBase[8]; //00e3ef0c
    undefined _static_Ending_CBase[8]; //00e3ef14
    undefined _static_Ranking_CBase[8]; //00e3ef1c
    undefined _static_ModeSelect_CBase[8]; //00e3ef24
    undefined _static_StaffRoll_CBase[8]; //00e3ef2c
    undefined _static_Result_CBase[8]; //00e3ef34
    undefined _static_MainMenu_CBase[8]; //00e3ef3c
    undefined _static_StorySelect_CBase[8]; //00e3ef44
    undefined _static_Lobby_CBase[8]; //00e3ef4c
    undefined _static_MyRoomTest_CBase[8]; //00e3ef54
    undefined _static_DCC_CBase[8]; //00e3ef5c
    undefined _static_ETCObjectStatic_CBase[0xc]; //00e3ef64
    struct CharSelect static_CharSelect; //00e3ef70
    undefined static_TitleLoop[0x4a664]; //00e419e0
    struct MainMenu static_MainMenu; //00e8c044
    undefined __e942f0[0x5b6c];
    undefined _static_net_etc_[0x68]; //00e99e5c
    undefined __e99ec4[0x5800c];
    struct GAME_CETCManager static_GAME_CETCManager; //00ef1ed0
    undefined __ef4898[0xd8];
    undefined4 _static_GAME_VersionInfoManager; //00ef4970
    undefined __ef4974[8];
    undefined _static_GAME_CBB4VersionInfo0_00_00[4]; //00ef497c
    undefined __ef4980[0x20ba40];
    struct MenuMessage _static_MenuMessage_0; //011003c0
    undefined __1100660[0x18];
    struct MenuMessage _static_MenuMessage_1; //01100678
    undefined __1100918[0x10];
    struct MenuMessage _static_MenuMessage_2; //01100928
    undefined __1100bc8[0x5a8a8];
    struct CBattleReplayDataManager static_CBattleReplayDataManager; //0115b470
    undefined __11c06f8[0xe5d0];
    undefined _static_CCommandListManager[0x4614]; //011cecc8
    undefined __11d32dc[0x1316d8];
    undefined4 _array_of_CMenuItemManager_length; //013049b4
    undefined4 _array_of_CMenuItemManager; //013049b8
    undefined __13049bc[0x1d4];
    int _replay_list_menu_state[8]; //01304b90
    undefined __1304bb0[0x48];
    struct AutoSaveStruct _static_AutoSaveStruct; //01304bf8
    undefined __1304c50[0xa9758];
    struct SettingsMenuItem _menu_item_configs_013ae3a8[0x84]; //013ae3a8
    int _menu_item_order_013b0088[0x86]; //013b0088
    undefined __13b02a0[0x1b018];
    struct SettingsMenuItem _menu_item_configs_013cb2b8[0x18]; //013cb2b8
    int _menu_item_order_013cb7f8[0x18]; //013cb7f8
    undefined __13cb858[0x53248];
    undefined _static_GAME_CEventManager[0xb90c]; //0141eaa0
};
static_assert(sizeof(BBCF) == 21144492);

/*PlaceHolder Class Structure*/
struct BG_CChinatown {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CChinatown) == 760);

/*PlaceHolder Class Structure*/
struct BB_CFont_GagePoint15AF {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_GagePoint15AF) == 48);

/*PlaceHolder Class Structure*/
struct AA_CPalette_HIP {
    undefined __0000[0x3c];
};
static_assert(sizeof(AA_CPalette_HIP) == 60);

/*PlaceHolder Class Structure*/
struct BG_CTown {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CTown) == 760);

/*PlaceHolder Class Structure*/
struct SCENE_CMatchResult {
    struct SCENE_CBase base;
    undefined __0060[0x30];
};
static_assert(sizeof(SCENE_CMatchResult) == 144);

/*PlaceHolder Class Structure*/
struct GAMEDX_CFadeTaskAstralFinish {
    undefined __0000[0x18];
};
static_assert(sizeof(GAMEDX_CFadeTaskAstralFinish) == 24);

/*PlaceHolder Class Structure*/
struct AA_CNetworkReceiver {
    undefined** vftable;
    undefined __0004[0x18];
};
static_assert(sizeof(AA_CNetworkReceiver) == 28);

/*PlaceHolder Class Structure*/
struct AASTEAM_CAsyncTaskThread {
    undefined __0000[0x3c];
};
static_assert(sizeof(AASTEAM_CAsyncTaskThread) == 60);

/*PlaceHolder Class Structure*/
struct BG_CMonolis {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CMonolis) == 760);

/* /ClassDataTypes/CCallbackImpl<16> */
struct CCallbackImpl_lt16_gt_placeholder { undefined __0000[0x14]; };

/*real size is 0x2248*/
struct OBJ_CBase {
    struct OBJ_CCharBase ch;
};
static_assert(sizeof(OBJ_CBase) == 149880);

/*PlaceHolder Class Structure*/
struct BB_CFont_CharSel_CntDown {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_CharSel_CntDown) == 48);

/*PlaceHolder Class Structure*/
struct BG_CHalloween_2 {
    undefined __0000[0x33c];
};
static_assert(sizeof(BG_CHalloween_2) == 828);

/*PlaceHolder Class Structure*/
struct SCENE_CAbyssUI {
    struct SCENE_CBase base;
    undefined __0060[0x14];
};
static_assert(sizeof(SCENE_CAbyssUI) == 116);

/*PlaceHolder Class Structure*/
struct BG_CMain_Night {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CMain_Night) == 760);

/*PlaceHolder Class Structure*/
struct BB_CFont_StageInfo_Num {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_StageInfo_Num) == 48);

/*PlaceHolder Class Structure*/
struct BG_CBascule {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CBascule) == 760);

/*PlaceHolder Class Structure*/
struct BB_CFont_StageInfo_Name {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_StageInfo_Name) == 48);

/*PlaceHolder Class Structure*/
struct CAdvCtrTask {
    struct AA_TaskNode base;
    undefined __0018[0x21960];
};
static_assert(sizeof(CAdvCtrTask) == 137592);

/*PlaceHolder Class Structure*/
struct SCENE_PreTeamBattle {
    struct SCENE_CBase base;
    undefined __0060[0x1fa0];
};
static_assert(sizeof(SCENE_PreTeamBattle) == 8192);

/*PlaceHolder Class Structure*/
struct AA_CPalette_Custom {
    undefined __0000[0x1028];
};
static_assert(sizeof(AA_CPalette_Custom) == 4136);

/*PlaceHolder Class Structure*/
struct GAME_CEventControlTask {
    struct AA_TaskNode base;
};
static_assert(sizeof(GAME_CEventControlTask) == 24);

/*PlaceHolder Class Structure*/
struct GAMEDX_CFadeTaskSpiral {
    undefined __0000[0x3c];
};
static_assert(sizeof(GAMEDX_CFadeTaskSpiral) == 60);

/*PlaceHolder Class Structure*/
struct SCENE_CReplayTheater {
    struct SCENE_CBase base;
    undefined __0060[0x28];
};
static_assert(sizeof(SCENE_CReplayTheater) == 136);

/*PlaceHolder Class Structure*/
struct BB_CFont_CharSel_BgmSel {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_CharSel_BgmSel) == 48);

/*PlaceHolder Class Structure*/
struct GAMESTEAM_CVoiceChatManager {
    undefined __0000;
    undefined __0001;
    undefined __0002;
    undefined __0003;
};
static_assert(sizeof(GAMESTEAM_CVoiceChatManager) == 4);

/*PlaceHolder Class Structure*/
struct BG_CGate_Open {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CGate_Open) == 760);

/*PlaceHolder Class Structure*/
struct SCENE_CTestMyRoom {
    struct SCENE_CBase base;
    undefined __0060[0xac];
};
static_assert(sizeof(SCENE_CTestMyRoom) == 268);

/*PlaceHolder Class Structure*/
struct SCENE_CDcc {
    struct SCENE_CBase base;
    undefined __0060[0xac];
};
static_assert(sizeof(SCENE_CDcc) == 268);

/*PlaceHolder Class Structure*/
struct BG_CUltimate {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CUltimate) == 760);

/*PlaceHolder Class Structure*/
struct BG_CCircus {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CCircus) == 760);

/*PlaceHolder Class Structure*/
struct CEnumMediaTypes {
    undefined __0000[0x14];
};
static_assert(sizeof(CEnumMediaTypes) == 20);

/*PlaceHolder Class Structure*/
struct BB_CFont_ModeSel_CntDown {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_ModeSel_CntDown) == 48);

/*PlaceHolder Class Structure*/
struct BG_CMain_Rain {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CMain_Rain) == 760);

/*PlaceHolder Class Structure*/
struct AASTEAM_CReplayUploadTask {
    struct AA_TaskNode base;
    undefined __0018[0x2740];
};
static_assert(sizeof(AASTEAM_CReplayUploadTask) == 10072);

/*PlaceHolder Class Structure*/
struct BG_CVillage {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CVillage) == 760);

/*PlaceHolder Class Structure*/
struct BB_CFont_Profile_PersonalData {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_Profile_PersonalData) == 48);

/*PlaceHolder Class Structure*/
struct SCENE_CLibraryMode {
    struct SCENE_CBase base;
    undefined __0060[0x10];
};
static_assert(sizeof(SCENE_CLibraryMode) == 112);

/*PlaceHolder Class Structure*/
struct BG_CBase {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CBase) == 760);

/*PlaceHolder Class Structure*/
struct CSceneController {
    struct AA_TaskNode base;
    int state;
    undefined4 __001c;
    undefined4 __0020;
};
static_assert(sizeof(CSceneController) == 36);

struct GAMESTEAM_SearchResultNode {
    struct AA_ListNode base;
    undefined4 valid;
    wchar_t room_name[0x10];
    undefined __0030[0x2a];
    short rank_host_level;
    char rank_myarea[2];
    short rank_area_filter;
    undefined __0060[4];
    int player_session_flag;
    int player_session_value;
    byte player_member_max;
    byte num_members;
    byte player_private_max;
    byte player_private_num;
    undefined __0070[4];
    byte host_netcolor;
    undefined __0075[3];
    int ping;
    undefined __007c[0x8e];
    short rank_rtt_filter;
    undefined __010c[8];
    struct AASTEAM_SearchResult* source;
};
static_assert(sizeof(GAMESTEAM_SearchResultNode) == 280);

/*PlaceHolder Class Structure*/
struct SCENE_CCharSelect {
    struct SCENE_CBase base;
    undefined __0060[0x274];
};
static_assert(sizeof(SCENE_CCharSelect) == 724);

/*PlaceHolder Class Structure*/
struct AA_CFont_Tool {
    undefined __0000[0x8c];
};
static_assert(sizeof(AA_CFont_Tool) == 140);

/*PlaceHolder Class Structure*/
struct SCENE_CStaffRoll {
    struct SCENE_CBase base;
    undefined __0060[0x28];
};
static_assert(sizeof(SCENE_CStaffRoll) == 136);

/*PlaceHolder Class Structure*/
struct SCENE_CStorySelect {
    struct SCENE_CBase base;
    undefined __0060[0x28];
};
static_assert(sizeof(SCENE_CStorySelect) == 136);

/*PlaceHolder Class Structure*/
struct SCENE_CExtra {
    struct SCENE_CBase base;
    undefined __0060[0x28];
};
static_assert(sizeof(SCENE_CExtra) == 136);

/*PlaceHolder Class Structure*/
struct BB_CFont_SparringTime {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_SparringTime) == 48);

/*PlaceHolder Class Structure*/
struct BG_CMain_Daylight {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CMain_Daylight) == 760);

/*PlaceHolder Class Structure*/
struct SCENE_CWinner {
    struct SCENE_CBase base;
    undefined __0060[0x5c];
};
static_assert(sizeof(SCENE_CWinner) == 188);

/*PlaceHolder Class Structure*/
struct AA_CRenderController {
    undefined** vftable;
    undefined __0004[0x14];
    struct AADX_CRenderer* renderer;
};
static_assert(sizeof(AA_CRenderController) == 28);

/*PlaceHolder Class Structure*/
struct BB_CFont_Score {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_Score) == 48);

/*PlaceHolder Class Structure*/
struct AA_CParticleGroupEdit {
    undefined __0000[0x88];
};
static_assert(sizeof(AA_CParticleGroupEdit) == 136);

/*PlaceHolder Class Structure*/
struct BG_CBridge {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CBridge) == 760);

/*PlaceHolder Class Structure*/
struct AASTEAM_CInputXInputPad {
    undefined __0000[0x3c];
};
static_assert(sizeof(AASTEAM_CInputXInputPad) == 60);

/*PlaceHolder Class Structure*/
struct SCENE_CStageInfo {
    struct SCENE_CBase base;
    undefined __0060[0x4c];
};
static_assert(sizeof(SCENE_CStageInfo) == 172);

/*PlaceHolder Class Structure*/
struct AA_CTextureList {
    undefined __0000[0x48];
};
static_assert(sizeof(AA_CTextureList) == 72);

/*PlaceHolder Class Structure*/
struct BG_CGarden {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CGarden) == 760);

/*PlaceHolder Class Structure*/
struct AAJVS_CInputPad {
    undefined __0000[0x58];
};
static_assert(sizeof(AAJVS_CInputPad) == 88);

/*PlaceHolder Class Structure*/
struct SCENE_CGallery {
    struct SCENE_CBase base;
    undefined __0060[0x4c];
};
static_assert(sizeof(SCENE_CGallery) == 172);

/*PlaceHolder Class Structure*/
struct GAMEDX_CFadeTaskBGCrossFade {
    undefined __0000[0x20];
};
static_assert(sizeof(GAMEDX_CFadeTaskBGCrossFade) == 32);

/*PlaceHolder Class Structure*/
struct AA_CBoneMotion {
    undefined __0000[0xc];
};
static_assert(sizeof(AA_CBoneMotion) == 12);

/*PlaceHolder Class Structure*/
struct GAMEJVS_CCreditTask {
    struct AA_TaskNode base;
    undefined __0018[0x50];
};
static_assert(sizeof(GAMEJVS_CCreditTask) == 104);

/*PlaceHolder Class Structure*/
struct BB_CFont_GagePoint15 {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_GagePoint15) == 48);

/*PlaceHolder Class Structure*/
struct GAMESTEAM_CDLCManager {
    undefined __0000[0x90];
};
static_assert(sizeof(GAMESTEAM_CDLCManager) == 144);

/*PlaceHolder Class Structure*/
struct GAMESTEAM_COnlineStorageTransfer {
    undefined __0000[0x1c];
};
static_assert(sizeof(GAMESTEAM_COnlineStorageTransfer) == 28);

/*PlaceHolder Class Structure*/
struct BB_CFont_GagePoint {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_GagePoint) == 48);

/*PlaceHolder Class Structure*/
struct SCENE_CTutorialUI {
    struct SCENE_CBase base;
    undefined __0060[0x10];
};
static_assert(sizeof(SCENE_CTutorialUI) == 112);

/*PlaceHolder Class Structure*/
struct AASTEAM_CUMSTask {
    struct AA_TaskNode base;
    undefined __0018[0xf8];
};
static_assert(sizeof(AASTEAM_CUMSTask) == 272);

/*PlaceHolder Class Structure*/
struct SCENE_TestRingCommand {
    struct SCENE_CBase base;
    undefined __0060[8];
};
static_assert(sizeof(SCENE_TestRingCommand) == 104);

/*PlaceHolder Class Structure*/
struct GAMEDX_CFadeTaskBGChange {
    undefined __0000[0x3fc54];
};
static_assert(sizeof(GAMEDX_CFadeTaskBGChange) == 261204);

/*PlaceHolder Class Structure*/
struct AA_CMaterialMotion {
    undefined __0000[0xc];
};
static_assert(sizeof(AA_CMaterialMotion) == 12);

/*PlaceHolder Class Structure*/
struct GAMEDX_CFadeTaskBreak {
    undefined __0000[0x3fc38];
};
static_assert(sizeof(GAMEDX_CFadeTaskBreak) == 261176);

/*PlaceHolder Class Structure*/
struct AASTEAM_CInputPad {
    undefined __0000[0x294];
};
static_assert(sizeof(AASTEAM_CInputPad) == 660);

/*PlaceHolder Class Structure*/
struct BB_CFont_OverDriveCountDwon {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_OverDriveCountDwon) == 48);

/*PlaceHolder Class Structure*/
struct BG_CGate_1 {
    undefined __0000[0x33c];
};
static_assert(sizeof(BG_CGate_1) == 828);

/*PlaceHolder Class Structure*/
struct AASTEAM_CVoiceChatManager {
    undefined __0000[0x5648];
};
static_assert(sizeof(AASTEAM_CVoiceChatManager) == 22088);

/*PlaceHolder Class Structure*/
struct BG_CGate_2 {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CGate_2) == 760);

/*PlaceHolder Class Structure*/
struct BG_CGate_3 {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CGate_3) == 760);

/*PlaceHolder Class Structure*/
struct AASTEAM_CRankingThreadManager {
    undefined __0000[0xc];
};
static_assert(sizeof(AASTEAM_CRankingThreadManager) == 12);

/*PlaceHolder Class Structure*/
struct SCENE_CLegion {
    struct SCENE_CBase base;
    undefined __0060[0x144];
};
static_assert(sizeof(SCENE_CLegion) == 420);

/*PlaceHolder Class Structure*/
struct AA_Timer {
    undefined __0000[0x20];
};
static_assert(sizeof(AA_Timer) == 32);

/*PlaceHolder Class Structure*/
struct BB_CFont_RankingFont0 {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_RankingFont0) == 48);

/*PlaceHolder Class Structure*/
struct GAMEDX_CFadeTaskGizaGiza {
    undefined __0000[0x1c];
};
static_assert(sizeof(GAMEDX_CFadeTaskGizaGiza) == 28);

/*PlaceHolder Class Structure*/
struct GAME_CFadeTask {
    struct AA_TaskNode base;
    undefined __0018;
    undefined __0019;
    undefined __001a;
    undefined __001b;
};
static_assert(sizeof(GAME_CFadeTask) == 28);

/*PlaceHolder Class Structure*/
struct BG_CPipe {
    undefined __0000[0x338];
};
static_assert(sizeof(BG_CPipe) == 824);

/*PlaceHolder Class Structure*/
struct AA_CCollision_JON {
    undefined __0000[0xc8];
};
static_assert(sizeof(AA_CCollision_JON) == 200);

/*PlaceHolder Class Structure*/
struct SCENE_CResult {
    struct SCENE_CBase base;
    undefined __0060[0x18];
};
static_assert(sizeof(SCENE_CResult) == 120);

/*PlaceHolder Class Structure*/
struct BB_CFont_BEAT {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_BEAT) == 48);

/*PlaceHolder Class Structure*/
struct BG_EffectMikadoTokitome {
    undefined __0000[0x5c];
};
static_assert(sizeof(BG_EffectMikadoTokitome) == 92);

/*PlaceHolder Class Structure*/
struct SCENE_CModeSelect {
    struct SCENE_CBase base;
    undefined __0060[0x64];
};
static_assert(sizeof(SCENE_CModeSelect) == 196);

/*PlaceHolder Class Structure*/
struct CEnumPins {
    undefined __0000[0x30];
};
static_assert(sizeof(CEnumPins) == 48);

/*PlaceHolder Class Structure*/
struct GAMEDX_CFadeTaskBG {
    undefined __0000[0x1c];
};
static_assert(sizeof(GAMEDX_CFadeTaskBG) == 28);

/*PlaceHolder Class Structure*/
struct SCENE_CMainMenuEX {
    struct SCENE_CBase base;
    undefined __0060[0x9940];
};
static_assert(sizeof(SCENE_CMainMenuEX) == 39328);

/*PlaceHolder Class Structure*/
struct GAME_CParticleTask {
    struct AA_TaskNode base;
};
static_assert(sizeof(GAME_CParticleTask) == 24);

/*PlaceHolder Class Structure*/
struct SCENE_CLobby {
    struct SCENE_CBase base;
    undefined __0060[0xf0];
};
static_assert(sizeof(SCENE_CLobby) == 336);

/*PlaceHolder Class Structure*/
struct BG_CBridge_Night {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CBridge_Night) == 760);

/*PlaceHolder Class Structure*/
struct BB_CFont_Combo {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_Combo) == 48);

/*PlaceHolder Class Structure*/
struct BB_CFont_RankingFont2 {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_RankingFont2) == 48);

/*PlaceHolder Class Structure*/
struct GAMEDX_CFadeTaskUltimateFinish {
    undefined __0000[0x18];
};
static_assert(sizeof(GAMEDX_CFadeTaskUltimateFinish) == 24);

/*PlaceHolder Class Structure*/
struct BG_CHalloween {
    undefined __0000[0x33c];
};
static_assert(sizeof(BG_CHalloween) == 828);

/*PlaceHolder Class Structure*/
struct BG_CChurch {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CChurch) == 760);

/*PlaceHolder Class Structure*/
struct SCENE_CTitleLoop {
    struct SCENE_CBase base;
    undefined __0060[0x38];
};
static_assert(sizeof(SCENE_CTitleLoop) == 152);

/*PlaceHolder Class Structure*/
struct BB_CFont_Time {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_Time) == 48);

/*PlaceHolder Class Structure*/
struct BB_CEvent {
    undefined __0000[0x40];
};
static_assert(sizeof(BB_CEvent) == 64);

/*PlaceHolder Class Structure*/
struct AASTEAM_CRankingWriter {
    undefined __0000[0x274d0];
};
static_assert(sizeof(AASTEAM_CRankingWriter) == 160976);

/*PlaceHolder Class Structure*/
struct SCENE_CRanking {
    struct SCENE_CBase base;
    undefined __0060[0x20];
};
static_assert(sizeof(SCENE_CRanking) == 128);

/*PlaceHolder Class Structure*/
struct BB_CFont_RankingFont4 {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_RankingFont4) == 48);

/*PlaceHolder Class Structure*/
struct AA_CModelInstance {
    undefined __0000[0x24];
};
static_assert(sizeof(AA_CModelInstance) == 36);

/*PlaceHolder Class Structure*/
struct SCENE_CWarning {
    struct SCENE_CBase base;
    undefined __0060[0x10];
};
static_assert(sizeof(SCENE_CWarning) == 112);

/*PlaceHolder Class Structure*/
struct BB_CFont_MainMenu_Cmn {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_MainMenu_Cmn) == 48);

/*PlaceHolder Class Structure*/
struct AA_CUserManagedStorage {
    undefined __0000[0x10];
};
static_assert(sizeof(AA_CUserManagedStorage) == 16);

/*PlaceHolder Class Structure*/
struct AA_CTexture_HIP {
    undefined __0000[0x60];
};
static_assert(sizeof(AA_CTexture_HIP) == 96);

/*PlaceHolder Class Structure*/
struct AASTEAM_CInputKeyBoard {
    undefined __0000[0x128];
};
static_assert(sizeof(AASTEAM_CInputKeyBoard) == 296);

struct GAMESTEAM_BattleKeyControler {
    struct GAME_KeyControler base;
};
static_assert(sizeof(GAMESTEAM_BattleKeyControler) == 88);

/*PlaceHolder Class Structure*/
struct BB_CFont_StorySelect_ChapterSelectData {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_StorySelect_ChapterSelectData) == 48);

/*PlaceHolder Class Structure*/
struct AA_CReplayDownloader {
    undefined __0000[8];
};
static_assert(sizeof(AA_CReplayDownloader) == 8);

/*PlaceHolder Class Structure*/
struct AADX_CDynamicTextureList {
    undefined __0000[0x5c];
};
static_assert(sizeof(AADX_CDynamicTextureList) == 92);

/*PlaceHolder Class Structure*/
struct AA_CRenderStreamNode {
    undefined __0000[0x330];
};
static_assert(sizeof(AA_CRenderStreamNode) == 816);

/*PlaceHolder Class Structure*/
struct BG_CChinatown_Night {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CChinatown_Night) == 760);

/*PlaceHolder Class Structure*/
struct AA_CCamera {
    undefined __0000[0x11c];
};
static_assert(sizeof(AA_CCamera) == 284);

/*PlaceHolder Class Structure*/
struct AA_CMotionFile_MMOT {
    undefined __0000[0x50];
};
static_assert(sizeof(AA_CMotionFile_MMOT) == 80);

/*PlaceHolder Class Structure*/
struct BB_CFont_TitleLoop_Group {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_TitleLoop_Group) == 48);

/*PlaceHolder Class Structure*/
struct BB_CFont_RankingFont7 {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_RankingFont7) == 48);

/*PlaceHolder Class Structure*/
struct AASTEAM_CReplayDownloadTask {
    struct AA_TaskNode base;
    undefined __0018[0x2728];
};
static_assert(sizeof(AASTEAM_CReplayDownloadTask) == 10048);

/*PlaceHolder Class Structure*/
struct SCENE_CEnding {
    struct SCENE_CBase base;
    undefined __0060[0x44];
};
static_assert(sizeof(SCENE_CEnding) == 164);

/*PlaceHolder Class Structure*/
struct GAMESTEAM_CRanking {
    undefined __0000[8];
};
static_assert(sizeof(GAMESTEAM_CRanking) == 8);

/*PlaceHolder Class Structure*/
struct BG_CAstralFinish {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CAstralFinish) == 760);

/*PlaceHolder Class Structure*/
struct AA_PreloadTask {
    struct AA_TaskNode base;
    int state;
    undefined __001c[0x48];
};
static_assert(sizeof(AA_PreloadTask) == 100);

/*PlaceHolder Class Structure*/
struct AASTEAM_CRankingReader {
    undefined __0000[0x2780];
};
static_assert(sizeof(AASTEAM_CRankingReader) == 10112);

/*PlaceHolder Class Structure*/
struct AA_CTexture_CmpHIP {
    undefined __0000[0x40];
};
static_assert(sizeof(AA_CTexture_CmpHIP) == 64);

/*PlaceHolder Class Structure*/
struct BG_CUltimateFinish {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CUltimateFinish) == 760);

/*PlaceHolder Class Structure*/
struct BG_CAstral {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CAstral) == 760);

/*PlaceHolder Class Structure*/
struct BB_CFont_RankingFont3 {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_RankingFont3) == 48);

/*PlaceHolder Class Structure*/
struct BB_CFont_RankingFont1 {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_RankingFont1) == 48);

/*PlaceHolder Class Structure*/
struct AA_CParticleEdit {
    undefined __0000[0x270];
};
static_assert(sizeof(AA_CParticleEdit) == 624);

/*PlaceHolder Class Structure*/
struct AADX_CMovieTexture {
    undefined __0000[0x2f8];
};
static_assert(sizeof(AADX_CMovieTexture) == 760);

/*PlaceHolder Class Structure*/
struct BB_CFont_StorySelect_Load {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_StorySelect_Load) == 48);

/*PlaceHolder Class Structure*/
struct GAMESTEAM_CClassGenerator {
    undefined** vftable;
    undefined** AADX_CDynamicTextureListFactory;
    undefined** AADX_CStaticTextureListFactory;
    undefined** AADX_CJointDynamicTextureListFactory;
    undefined** AADX_CMovieTextureFactory;
    undefined** AA_CTextureFactory_HIP;
    undefined** AA_CPaletteFactory_HIP;
    undefined** AA_CTextureFactory_DDS;
    undefined** AA_CTextureFactory_CmpHIP;
    undefined** BB_CEventFactory;
    undefined** BB_CEventInstanceFactory;
    undefined** AA_CCollisionFactory_JON;
    undefined** AA_CModelFactory_MUA;
};
static_assert(sizeof(GAMESTEAM_CClassGenerator) == 52);

/*PlaceHolder Class Structure*/
struct AA_CTexture_DDS {
    undefined __0000[0xa4];
};
static_assert(sizeof(AA_CTexture_DDS) == 164);

/*PlaceHolder Class Structure*/
struct SCENE_CProfile {
    struct SCENE_CBase base;
    undefined __0060[0x34];
};
static_assert(sizeof(SCENE_CProfile) == 148);

/*PlaceHolder Class Structure*/
struct AA_CModel_MUA {
    undefined __0000[0x84];
};
static_assert(sizeof(AA_CModel_MUA) == 132);

/*PlaceHolder Class Structure*/
struct BG_CPipe_2 {
    undefined __0000[0x2f8];
};
static_assert(sizeof(BG_CPipe_2) == 760);

/*PlaceHolder Class Structure*/
struct BB_CFont_OverDrive {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_OverDrive) == 48);

/*PlaceHolder Class Structure*/
struct AA_CReplayUploader {
    undefined __0000[8];
};
static_assert(sizeof(AA_CReplayUploader) == 8);

/*PlaceHolder Class Structure*/
struct BB_CFont_SystemFont {
    undefined __0000[0x30];
};
static_assert(sizeof(BB_CFont_SystemFont) == 48);


#pragma pack ( pop )
