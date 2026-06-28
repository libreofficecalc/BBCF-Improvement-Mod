#pragma once
#pragma once
#include "NetworkManager.h"
#include "RoomManager.h"

class ReplayUploadManager
{
public:
	ReplayUploadManager(RoomManager* pRoomManager);
	void OnMatchInit();
	void OnMatchEnd();
	void SendReplayUploadEnabledBroadcastPacket();
	void RecvReplayUploadEnabledBroadcastPacket(Packet* packet);
	
	RoomManager* m_pRoomManager;
};