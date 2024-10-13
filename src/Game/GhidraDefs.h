typedef unsigned char   undefined;

//typedef unsigned long long    GUID;
//typedef pointer32 ImageBaseOffset32;

//typedef unsigned char    bool;
typedef unsigned char    byte;
typedef unsigned int    dword;
//float10
//typedef long double    longdouble;
typedef long long    longlong;
typedef unsigned char    uchar;
typedef unsigned int    uint;
typedef unsigned long    ulong;
typedef unsigned long long    ulonglong;
typedef unsigned char    undefined1;
typedef unsigned short    undefined2;
typedef unsigned int    undefined4;
typedef unsigned long long    undefined5;
typedef unsigned long long    undefined6;
typedef unsigned long long    undefined8;
typedef unsigned short    ushort;
typedef unsigned short    wchar16;
//typedef short    wchar_t;
typedef unsigned short    word;
typedef struct SteamPeer2PeerBackend SteamPeer2PeerBackend, * PSteamPeer2PeerBackend;

typedef struct GGPOSessionCallbacks GGPOSessionCallbacks, * PGGPOSessionCallbacks;

typedef struct Poll Poll, * PPoll;

typedef struct Sync Sync, * PSync;

typedef struct Udp Udp, * PUdp;

typedef uint UINT_PTR;

typedef UINT_PTR SOCKET;

typedef struct SteamUdpProtocol SteamUdpProtocol, * PSteamUdpProtocol;

typedef int BOOL;

typedef struct UdpMsg__connect_status UdpMsg__connect_status, * PUdpMsg__connect_status;

typedef struct Sync__SavedState Sync__SavedState, * PSync__SavedState;

typedef struct Sync__Config Sync__Config, * PSync__Config;

typedef struct InputQueue InputQueue, * PInputQueue;

typedef struct sockaddr_in sockaddr_in, * Psockaddr_in;

typedef ushort UINT16;

typedef struct SteamUdpProtocol___oo_packet SteamUdpProtocol___oo_packet, * PSteamUdpProtocol___oo_packet;

typedef struct RingBuffer_QueueEntry_256_ RingBuffer_QueueEntry_256_ , * PRingBuffer_QueueEntry_256_;

typedef union SteamUdpProtocol___state SteamUdpProtocol___state, * PSteamUdpProtocol___state;

typedef struct GameInput GameInput, * PGameInput;

typedef ushort uint16_t;

typedef struct TimeSync TimeSync, * PTimeSync;

typedef struct Sync__SavedFrame Sync__SavedFrame, * PSync__SavedFrame;

typedef ushort u_short;

typedef struct in_addr * Pin_addr;

typedef struct UdpMsg UdpMsg, * PUdpMsg;

typedef struct SteamUdpProtocol__QueueEntry SteamUdpProtocol__QueueEntry, * PSteamUdpProtocol__QueueEntry;

typedef struct SteamUdpProtocol__Union_state_sync SteamUdpProtocol__Union_state_sync, * PSteamUdpProtocol__Union_state_sync;

typedef struct SteamUdpProtocol__Union_state_running SteamUdpProtocol__Union_state_running, * PSteamUdpProtocol__Union_state_running;

typedef union _union_1226 _union_1226, * P_union_1226;

typedef struct _struct_1227 _struct_1227, * P_struct_1227;

typedef struct _struct_1228 _struct_1228, * P_struct_1228;

typedef ulong ULONG;

typedef uchar UCHAR;

typedef ushort USHORT;


typedef struct SnapshotManager SnapshotManager, * PSnapshotManager;

//typedef struct 20bdc_sized_plaaceholder 20bdc_sized_plaaceholder, * P20bdc_sized_plaaceholder;

typedef struct SnapshotManager__struct SnapshotManager__struct, * PSnapshotManager__struct;

struct static_DAT_of_PTR_on_load_4{
    byte padding_0x0[12];
    undefined* ptr_OBJ_with_seemingly_list_managers_1;
    undefined padding_0x10[16];
    undefined* ptr_OBJ_with_seemingly_list_managers_2;
    undefined padding_0x24[8];
    uint size_or_counter_for_inline_managers_1;
    struct BATTLE_CObjectManager*ptr_BATTLE_CObjectManager_plus_0x4;
    undefined* ptr_BATTLE_CScreenManager;
    undefined* ptr_GAME_CEff3DInstHndlManager;
    undefined* ptr_AA_CRandomManager;
    undefined* ptr_battle_stat__StatBattleTemp;
    undefined* ptr_BATTLE_CBGManager;
    byte padding_0x48[104];
    int size_or_counter_for_inline_managers_2;
    undefined* ptr_BATTLE_CObjectManager_static;
    undefined* ptr_AA_CParticleManager;
    undefined* ptr_BATTLE_CScreenManager_differen;
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
    byte padding_0xf0[128];
    struct SnapshotManager* ptr_snapshot_manager_mine;
    void* field31_0x174;
    void* field32_0x178;
    void* field33_0x17c;
    void* field34_0x180;
    char field35_0x184;
    undefined field36_0x185[335];
};

struct SnapshotManager__struct {
    int _framecount;
    undefined* _ptr_buf_saved_frame;
    int field2_0x8;
    int field3_0xc;
    undefined* field4_0x10;
    undefined* _ptr_buf_save_frame_1_maybe;
    undefined* _ptr_buf_save_frame_1_plus_some_offset;
    undefined* _ptr_BB_CEventInstance_0;
    undefined field8_0x20[12];
    undefined* _ptr_BB_CEventInstance_1;
    undefined* _ptr_BB_CEventInstance_1_plus_some_offset;
    undefined field11_0x34[12];
    undefined* field12_0x40;
    int field13_0x44;
};

struct sized_20bdc_plaaceholder{
    undefined field0_0x0[134108];
};

struct BATTLE_CObjectManager{
    byte field0_0x0[696];
    struct sized_20bdc_plaaceholder field1_0x2b8[3];
    undefined field2_0x6264c[312];
    undefined* ptr_to_smth_with_entity_list_1;
    undefined* ptr_to_smth_with_entity_list_2;
    undefined* ptr_to_smth_with_entity_list_3;
    undefined* array_ptr_to_OBJ_CBase[250];
    undefined* ptr_to_p1_OBJ_CCharBase;
    undefined field8_0x62b7c[135880];
    undefined* field9_0x83e44;
};

struct SnapshotManager {
    struct SnapshotManager__struct _saved_states_related_struct[10];
    int _counter_of_some_sort;
    undefined* field2_0x2d4;
    undefined* _base_of_snapshot_mem;
};


struct _struct_1227 {
    UCHAR s_b1;
    UCHAR s_b2;
    UCHAR s_b3;
    UCHAR s_b4;
};

struct _struct_1228 {
    USHORT s_w1;
    USHORT s_w2;
};

union _union_1226 {
    struct _struct_1227 S_un_b;
    struct _struct_1228 S_un_w;
    ULONG S_addr;
};

//struct in_addr {
//    union _union_1226 S_un;
//};

struct GameInput { /* the bits estimate needs to be checked */
    int frame;
    int size;
    char bits[18];
};

struct TimeSync { /* must double check GameInput size, might be 2 bytes short */
    undefined* field0_0x0;
    int _local[40];
    int _remote[40];
    struct GameInput _last_inputs[10];
    undefined field4_0x248;
    undefined field5_0x249;
    undefined field6_0x24a;
    undefined field7_0x24b;
    undefined field8_0x24c;
    undefined field9_0x24d;
    undefined field10_0x24e;
    undefined field11_0x24f;
    undefined field12_0x250;
    undefined field13_0x251;
    undefined field14_0x252;
    undefined field15_0x253;
    undefined field16_0x254;
    undefined field17_0x255;
    undefined field18_0x256;
    undefined field19_0x257;
    undefined field20_0x258;
    undefined field21_0x259;
    undefined field22_0x25a;
    undefined field23_0x25b;
    int _next_prediction;
};

struct Sync__SavedFrame {
    byte* buf;
    int cbuf;
    int frame;
    int checksum;
};

struct Sync__SavedState {
    struct Sync__SavedFrame frames[10];
    int head;
};

struct GGPOSessionCallbacks {
    //undefined* begin_game;
    int (*begin_game)();//its an empty function for now, will point to the same as log
    //undefined* save_game_state;
    int (*save_game_state)(unsigned char**, int*, int*); //aside from param_1 being pbuf im not sure of the other parameters, param_2 is either counter_of_some_sort or size of buf and param_3 i think its checksum, can be left as an adress to 0
    //undefined* load_game_state;
    int (*load_game_state)(unsigned char*); // param_1 is the adress of the buf to load
    //undefined* maybe_log_game_state;
    int (*maybe_log_game_state)();//its an empty function for now, will point to the same as log
    //undefined* free_buffer;
    int (*free_buffer)(unsigned char*); // param_1 is the adress of the buf to free?
    //undefined* advance_frame;
    int (*advance_frame)();
    //undefined* on_event;
    int (*on_event)(unsigned char*); // idk what param_1 is
};

struct Sync__Config {
    struct GGPOSessionCallbacks callbacks;
    int maybe_num_prediction_frames;
    int maybe_num_players;
    int maybe_input_size;
};

struct Sync { /* prob need to look at Sync__SavedStated again to fix the offset hardcoded */
    void** vftable;
    struct GGPOSessionCallbacks _callbacks;
    struct Sync__SavedState _savedstate;
    struct Sync__Config _config;
    BOOL _rollingback;
    int _last_confirmed_frame;
    int _framecount;
    int _max_prediction_frames;
    struct InputQueue* _input_queues;
    undefined field9_0x100[1036];
    struct UdpMsg__connect_status* _local_connect_status;
};

struct SteamUdpProtocol__Union_state_running {
    uint last_quality_report_time;
    uint last_network_stats_interval;
    uint last_input_packet_recv_time;
};

//struct sockaddr_in {
//    short sin_family;
//    u_short sin_port;
//    struct in_addr sin_addr;
 //   char sin_zero[8];
//};

struct SteamUdpProtocol___oo_packet {
    int send_time;
    struct sockaddr_in dest_addr;
    struct UdpMsg* msg;
};

struct SteamUdpProtocol__Union_state_sync {
    uint roundtrips_remaining;
    uint random;
};

union SteamUdpProtocol___state {
    struct SteamUdpProtocol__Union_state_sync sync;
    struct SteamUdpProtocol__Union_state_running running;
};

struct SteamUdpProtocol__QueueEntry {
    int queue_time;
    struct sockaddr_in dest_addr;
    struct UdpMsg* msg;
};

struct RingBuffer_QueueEntry_256_ {
    struct SteamUdpProtocol__QueueEntry _elements[256];
    int _tail;
    int _size;
    int _head;
};

struct SteamUdpProtocol {
    void** vftable;
    struct Udp* _udp;
    struct sockaddr_in _peer_addr;
    UINT16 _magic_number;
    int _queue;
    UINT16 _remote_magic_number;
    BOOL _connected;
    int _send_latency;
    int _oop_percent;
    struct SteamUdpProtocol___oo_packet _oo_packet;
    struct RingBuffer_QueueEntry_256_ _send_queue;
    undefined* field11_0x1850;
    int _packets_sent;
    int _bytes_sent;
    int _kbps_sent;
    int _stats_start_time;
    undefined field16_0x1864[24];
    union SteamUdpProtocol___state _state;
    undefined field18_0x1888[7188];
    struct GameInput _last_received_input;
    undefined field20_0x34b6;
    undefined field21_0x34b7;
    struct GameInput _last_sent_input;
    undefined field23_0x34d2;
    undefined field24_0x34d3;
    struct GameInput _last_acked_input;
    undefined field26_0x34ee;
    undefined field27_0x34ef;
    uint _last_send_time;
    uint _last_recv_time;
    uint _shutdown_timeout;
    uint _diconnect_event_sent;
    uint _disconnect_timeout;
    uint _disconnect_notify_start;
    bool _disconnect_notify_sent;
    undefined field35_0x3509;
    undefined field36_0x350a;
    undefined field37_0x350b;
    uint16_t _next_send_seq;
    uint16_t _next_recv_seq;
    struct TimeSync _timesync;
    undefined field41_0x3770[8208];
};

struct UdpMsg__connect_status {
    int disconnected : 1;
    int last_frame : 31;
};

struct Udp {
    SOCKET(*SOCKET)(int, int, int);
    struct GGPOSessionCallbacks* _callbacks;
    struct Poll* _poll;
};

struct Poll {
    undefined field0_0x0[1296];
};

struct SteamPeer2PeerBackend { /* PlaceHolder Class Structure */
    void** vftable;
    undefined* field1_0x4;
    undefined* field2_0x8;
    struct GGPOSessionCallbacks _callbacks;
    undefined* field4_0x28;
    struct Poll _poll;
    struct Sync _sync;
    struct Udp _udp;
    struct SteamUdpProtocol* _endpoints;
    int idk_what;
    struct SteamUdpProtocol _spectators[6];
    int _num_spectators;
    int _input_size;
    BOOL _synchronizing;
    int _num_players;
    int _next_recommended_sleep;
    int _next_spectator_frame;
    int _disconnect_timeout;
    int _disconnect_notify_start;
    struct UdpMsg__connect_status _local_connect_status[2];
    undefined field20_0x21788;
    undefined field21_0x21789;
    undefined field22_0x2178a;
    undefined field23_0x2178b;
    undefined field24_0x2178c;
    undefined field25_0x2178d;
    undefined field26_0x2178e;
    undefined field27_0x2178f;
};

struct UdpMsg {
    undefined field0_0x0[4128];
};

struct InputQueue {
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
    struct GameInput _inputs[138];
    struct GameInput _prediction;
    undefined field12_0xe46;
    undefined field13_0xe47;
};

