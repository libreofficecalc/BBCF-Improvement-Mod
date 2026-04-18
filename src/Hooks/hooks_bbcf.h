#pragma once

bool placeHooks_bbcf();

void RankedProbeTickFrameState();
void RankedProbeNoteLobbyCaller();
void RankedProbeNoteBuilder();
void RankedProbeNoteCompose();
void RankedProbeNoteState3Enter();
void RankedProbeNotePackSelect();
void RankedProbeNotePhase3();
void RankedProbeNoteBit4Skip();
void RankedProbeNoteSourceTotal();
void RankedProbeNoteSourcePair();
void RankedProbeNoteWritePacked();
void RankedProbeNoteGameCall();
void RankedProbeNoteUpload();
void RankedProbeDumpSummary(const char* reason);
