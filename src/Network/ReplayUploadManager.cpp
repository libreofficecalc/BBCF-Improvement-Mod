#include "ReplayUploadManager.h"

#include "Core/logger.h"
#include "Core/interfaces.h"




ReplayUploadManager::ReplayUploadManager(RoomManager* pRoomManager) : m_pRoomManager(pRoomManager){}

void ReplayUploadManager::OnMatchInit() {
	uint16_t thisPlayerMatchPlayerIndex = g_interfaces.pRoomManager->GetThisPlayerMatchPlayerIndex();
	// Only should be done if you are playing the match and not spectating
	if (thisPlayerMatchPlayerIndex < 2)
	{
		SendReplayUploadEnabledBroadcastPacket();
	}
	
}
void ReplayUploadManager::OnMatchEnd() {
	// Resets the value to default "false" to wait for broadcast
	g_modVals.uploadReplayDataVeto = false;
}
void ReplayUploadManager::SendReplayUploadEnabledBroadcastPacket()
{
	//this should be run on match init
	LOG(2, "ReplayUploadManager::SendReplayUploadEnabledCheckPacket\n");

	uint16_t thisPlayerMatchPlayerIndex = m_pRoomManager->GetThisPlayerMatchPlayerIndex();
	void* data_src = &g_modVals.uploadReplayData;
	Packet packet = Packet(
		data_src,
		(uint16_t)sizeof(g_modVals.uploadReplayData),
		PacketType_UploadReplayEnabled_Broadcast,
		thisPlayerMatchPlayerIndex
	);
	m_pRoomManager->SendPacketToSameMatchIMPlayersNonSpectator(&packet);

}

void ReplayUploadManager::RecvReplayUploadEnabledBroadcastPacket(Packet* packet)
{
	/*if either p1 or p2 don't want uploaded, disable it */
	int allowUpload;
	memcpy(&allowUpload, packet->data, packet->dataSize);
	if (!allowUpload) {
		g_modVals.uploadReplayDataVeto = true;
	}
}
