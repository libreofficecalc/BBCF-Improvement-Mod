#pragma once
#include "DebugWindow.h"

#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Overlay/NotificationBar/NotificationBar.h"
#include "Overlay/WindowManager.h"
#include "Overlay/Window/HitboxOverlay.h"
#include "Core/info.h"
//#include "Game/GhidraDefs.h"
#include "Game/SnapshotApparatus/SnapshotApparatus.h"
#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

//#include <d3d9.h>
//#include <imgui_impl_dx9.cpp>
//#include <d3dx9tex.h> // Include D3DX9Tex header for texture loading

//LPDIRECT3DTEXTURE9 LoadTextureFromFile(LPDIRECT3DDEVICE9 device, const char* filename) {
//	int width, height, numChannels;
//	unsigned char* imageData = stbi_load(filename, &width, &height, &numChannels, STBI_rgb_alpha);
//	if (!imageData) {
//		return nullptr;
//	}
//
//	LPDIRECT3DTEXTURE9 texture = nullptr;
//	HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(device, imageData, width * height * 4,
//		width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
//		D3DX_DEFAULT, D3DX_DEFAULT, 0xFF000000, nullptr, nullptr, &texture);
//
//	// Free the loaded image data
//	stbi_image_free(imageData);
//
//	if (FAILED(hr)) {
//		return nullptr;
//	}
//
//	return texture;
//}

//bool LoadTextureFromFile(const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height)
//{
//	// Load texture from disk
//	auto device = g_interfaces.pD3D9ExWrapper;
//	PDIRECT3DTEXTURE9 texture;
//	HRESULT hr = D3DXCreateTextureFromFileA(device, filename, &texture);
//	if (hr != S_OK)
//		return false;
//	// Retrieve description of the texture surface so we can access its size
//	D3DSURFACE_DESC my_image_desc;
//	texture->GetLevelDesc(0, &my_image_desc);
//	*out_texture = texture;
//	*out_width = (int)my_image_desc.Width;
//	*out_height = (int)my_image_desc.Height;
//	return true;
//}
//



void DebugWindow::Draw()
{	
	//g_pd3dDevice
	//auto device = g_interfaces.pD3D9ExWrapper;
	//std::string path = "jn203_06.png";
	//int my_image_width = 0;
	//int my_image_height = 0;
	//PDIRECT3DTEXTURE9 my_texture = NULL;
	//bool ret = LoadTextureFromFile("jn203_06.png", &my_texture, &my_image_width, &my_image_height);
	//if (ret) {
	//	// Handle loading failure
	//	// ...
	//	ImGui::Image((void*)my_texture, ImVec2(my_image_width, my_image_height));
	//}
	
	if (m_showDemoWindow)
	{
		ImGui::ShowDemoWindow(&m_showDemoWindow);
	}

	DrawImGuiSection();

	DrawGameValuesSection();

	DrawRoomSection();

	DrawSettingsSection();

	DrawNotificationSection();
}

void DebugWindow::DrawImGuiSection()
{
	if (!ImGui::CollapsingHeader("ImGui"))
		return;

	if (ImGui::TreeNode("Display"))
	{
		static ImVec2 newDisplaySize = ImVec2(Settings::settingsIni.renderwidth, Settings::settingsIni.renderheight);

		ImVec2 curDisplaySize = ImGui::GetIO().DisplaySize;

		ImGui::Text("Current display size: %.f %.f", curDisplaySize.x, curDisplaySize.y);

		ImGui::InputFloat2("Display size", (float*)&newDisplaySize, 0);

		static bool isNewDisplaySet = false;
		ImGui::Checkbox("Set value", &isNewDisplaySet);

		if (isNewDisplaySet)
			ImGui::GetIO().DisplaySize = newDisplaySize;

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Unicode text"))
	{
		ImGui::Text("Hiragana: \xe3\x81\x8b\xe3\x81\x8d\xe3\x81\x8f\xe3\x81\x91\xe3\x81\x93 (kakikukeko)");
		ImGui::Text("Kanjis: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (nihongo)");

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Frame data"))
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("DeltaTime: %f", ImGui::GetIO().DeltaTime);
		ImGui::Text("FrameCount: %d", ImGui::GetFrameCount());

		ImGui::TreePop();
	}

	if (ImGui::Button("Demo"))
	{
		m_showDemoWindow = !m_showDemoWindow;
	}
}

void DebugWindow::DrawGameValuesSection()
{
	//bbcf.exe+0x612888
	//static unsigned char savedstate_mine[10][0xa10000] = {};
	char* base_addr = GetBbcfBaseAdress();
	if (ImGui::TreeNode("replay rewind testing")) {
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("snapshot testing")) {


		void* DAT_01292888 = base_addr + 0x612888;
		static SnapshotApparatus* snap_apparatus_debug = nullptr;
		static int snapshot_position_counter = 0;
		if (snap_apparatus_debug == nullptr) {
			snap_apparatus_debug = new SnapshotApparatus();
		}
		if (ImGui::Button("Save snapshot")) {
			Snapshot* pbuf_mine = new Snapshot();
			//snap_apparatus_debug->save_snapshot(0);
			snap_apparatus_debug->save_snapshot(&pbuf_mine);



		}
		if (ImGui::Button("Load snapshot")) {
			snap_apparatus_debug->load_snapshot(0);

		}
		//!!!!UNCOMMENT LATER WHEN STATIC EXISTS
		//if (ImGui::Button("Save snapshot replay mine struct")) {
		//	Snapshot* buf = &snapshot_replay_pre_allocated[snapshot_position_counter %SNAPSHOT_PREALLOC_SIZE];
		//	snap_apparatus_debug->save_snapshot(&buf);
		//	//snap_apparatus_debug->save_snapshot((Snapshot**)savedstate_mine);
		//	snapshot_position_counter += 1;

		//}
		//if (ImGui::Button("Load snapshot replay mine struct 0 ")) {
		//	auto pos = &snapshot_replay_pre_allocated[0];
		//	snap_apparatus_debug->load_snapshot(&snapshot_replay_pre_allocated[0]);

		//}
		//if (ImGui::Button("Load snapshot replay mine struct")) {
		//	auto pos2 = &snapshot_replay_pre_allocated[(snapshot_position_counter-1) % SNAPSHOT_PREALLOC_SIZE];
		//	snap_apparatus_debug->load_snapshot(&snapshot_replay_pre_allocated[(snapshot_position_counter-1)% SNAPSHOT_PREALLOC_SIZE]);

		//}
		if (ImGui::TreeNode("Netcode stuff")) {

			char* base_addr = GetBbcfBaseAdress();
			void* addr = base_addr + 0x65bd08;
			SteamPeer2PeerBackend* bckend = *(SteamPeer2PeerBackend**)addr;
			static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(base_addr + 0x612718);
			SnapshotManager* snap_manager = 0;
			if (DAT_on_load_4_addr) {
				snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
			}
			void* initialize_ggpo_callbacks_struct_ptr = base_addr + 0x383dd0;
			typedef void (*initialize_ggpo_callbacks_struct_decl)(GGPOSessionCallbacks*);
			initialize_ggpo_callbacks_struct_decl initialize_ggpo_callbacks_struct = reinterpret_cast<initialize_ggpo_callbacks_struct_decl>(initialize_ggpo_callbacks_struct_ptr);
			GGPOSessionCallbacks* callbacks_ptr = new GGPOSessionCallbacks;
			initialize_ggpo_callbacks_struct(callbacks_ptr);

			if (*(int*)DAT_01292888 == 0) {
				//return;

				//those are the changes to code directly to initializze maybe_network_stuff_init
				// 
				// 
				// 
				///PRELUDE
				auto mem_offset_1 = 0xe5701;//2 bytes
				auto mem_offset_2 = 0xe577d;//2 bytes
				auto mem_offset_3 = 0xe5a9c;//6 bytes
				char nops[] = "\x90\x90\x90\x90\x90\x90\x90\x90";
				char oldmem_1[2];// = "\x90\x90\";
				char oldmem_2[2];// = "\x90\x90\";
				char oldmem_3[6];// = "\x90\x90\x90\x90\x90\x90\x90\x90";
				void* ptr_oldmem_1 = base_addr + mem_offset_1;
				void* ptr_oldmem_2 = base_addr + mem_offset_2;
				void* ptr_oldmem_3 = base_addr + mem_offset_3;
				memcpy(oldmem_1, ptr_oldmem_1, 2);
				WriteToProtectedMemory((uintptr_t)ptr_oldmem_1, nops, 2);
				memcpy(oldmem_2, ptr_oldmem_2, 2);
				WriteToProtectedMemory((uintptr_t)ptr_oldmem_2, nops, 2);
				memcpy(oldmem_3, ptr_oldmem_3, 6);
				WriteToProtectedMemory((uintptr_t)ptr_oldmem_3, nops, 6);
				//memcpy(ptr_oldmem_1, nops, 2);
				///PRELUDE_END

				//this is copied from "maybe_network_stuff_init
				void** SCENE_CBattle_static_ptr = (void**)(base_addr + 0x8929B4);
				void* maybe_network_stuff_init_ptr = base_addr + 0xe56f0;
				typedef void (*maybe_network_stuff_init_decl)(void*);
				maybe_network_stuff_init_decl maybe_network_stuff_init = reinterpret_cast<maybe_network_stuff_init_decl>(maybe_network_stuff_init_ptr);
				maybe_network_stuff_init(*SCENE_CBattle_static_ptr);
				WriteToProtectedMemory((uintptr_t)ptr_oldmem_1, oldmem_1, 2);
				WriteToProtectedMemory((uintptr_t)ptr_oldmem_2, oldmem_2, 2);
				WriteToProtectedMemory((uintptr_t)ptr_oldmem_3, oldmem_3, 6);
				//memcpy(ptr_oldmem_1, oldmem_1, 2);
			}


			if (ImGui::Button("maybe_ggpo_start_session_with_SteamPeer2PeerBackend")) {
				void* ggpo_start_session_ptr = base_addr + 0x37c7a0;
				void* initialize_ggpo_callbacks_struct_ptr = base_addr + 0x383dd0;
				void* maybe_session_init_idk_ptr = base_addr + 0x383750;
				typedef int (*ggpo_start_session_decl)(unsigned char**, GGPOSessionCallbacks*, char*, int, int);
				typedef void (*initialize_ggpo_callbacks_struct_decl)(GGPOSessionCallbacks*);
				typedef void (*maybe_session_init_idk_decl)(void*);
				ggpo_start_session_decl ggpo_start_session = reinterpret_cast<ggpo_start_session_decl>(ggpo_start_session_ptr);
				initialize_ggpo_callbacks_struct_decl initialize_ggpo_callbacks_struct = reinterpret_cast<initialize_ggpo_callbacks_struct_decl>(initialize_ggpo_callbacks_struct_ptr);
				maybe_session_init_idk_decl maybe_session_init_idk = reinterpret_cast<maybe_session_init_idk_decl>(maybe_session_init_idk_ptr);

				GGPOSessionCallbacks* callbacks_ptr = new GGPOSessionCallbacks;
				initialize_ggpo_callbacks_struct(callbacks_ptr);
				//needs to be initialized with something else as the first argument, need to try and run the "maybe_session_init_idk"(base+383750) function instead of this one raw
				ggpo_start_session((unsigned char**)addr, callbacks_ptr, "BBCF", 2, 2); // if run by itself the game freezes, doesnt crash, so prob is just waiting for the rest to be defined?
				//ggpo_start_session((unsigned char**)addr, callbacks_ptr, "BBCF", 0, 2);
				//maybe_session_init_idk((void*)(base_addr + 0x65bd08));
				//need another one
			}
			if (ImGui::Button("maybe_network_stuff_init")) {
				void** SCENE_CBattle_static_ptr = (void**)(base_addr + 0x8929B4);
				void* maybe_network_stuff_init_ptr = base_addr + 0xe56f0;
				typedef void (*maybe_network_stuff_init_decl)(void*);
				maybe_network_stuff_init_decl maybe_network_stuff_init = reinterpret_cast<maybe_network_stuff_init_decl>(maybe_network_stuff_init_ptr);
				maybe_network_stuff_init(*SCENE_CBattle_static_ptr);
			}

			//static unsigned char savedstate_mine[10][0xa10000];// = new unsigned char[0x8536f8];
			static int* temp_savestate_loc = 0;
			//static int sizeofstate = 0x8536f8;
			static int sizeofstate = 0xa10000;
			static int checksum = 0;
			static int savegame_index = 0;
			static int savegame_count = 0;
			//ImGui::Text("static savedstate_mine addr: %x", &savedstate_mine);
			//static unsigned char* ptr_savedstate_mine = (unsigned char*)&savedstate_mine;
			//void* snapshot_manager
			if (snap_manager) {
				ImGui::Text("savegame_count: %d", savegame_count);
				ImGui::Text("_counter_of_some_sort: %d", snap_manager->_counter_of_some_sort);
				if (ImGui::Button("save_state_no_backend")) {


					//callbacks_ptr->save_game_state((unsigned char**)&savedstate_mine[savegame_index], &sizeofstate, &checksum);

					sizeofstate = 1;
					unsigned char** pbuf = (unsigned char**)&snap_manager->_saved_states_related_struct[savegame_count % 10]._ptr_buf_saved_frame;
					//callbacks_ptr->save_game_state(pbuf, 
					//								&snap_manager->_counter_of_some_sort, 
					//								&checksum);
					callbacks_ptr->free_buffer(*pbuf);
					callbacks_ptr->save_game_state(pbuf,
						&sizeofstate,
						&checksum);
					savegame_index = (savegame_index + 1) % 10;
					savegame_count += 1;


				}
				if (savegame_count == 0) {

				}
				if (ImGui::Button("load_state_no_backend")) {

					//memcpy(&temp_savestate_loc, savedstate_mine[savegame_index], 4);
					//callbacks_ptr->load_game_state((unsigned char*)temp_savestate_loc);
					///PRELUDE
					auto mem_offset_1 = 0x383f63;//3 bytes
					void* ptr_oldmem_load = base_addr + mem_offset_1;
					char nops[] = "\x90\x90\x90\x90\x90\x90\x90\x90";
					char jmp_short_23[] = "\xEB\x23";
					char oldmem_load[3];
					memcpy(oldmem_load, ptr_oldmem_load, 3);
					WriteToProtectedMemory((uintptr_t)ptr_oldmem_load, jmp_short_23, 2);
					WriteToProtectedMemory((uintptr_t)ptr_oldmem_load + 2, nops, 1);
					/// PRELUDE_END

					unsigned char* buf = (unsigned char*)snap_manager->_saved_states_related_struct[(savegame_count - 1) % 10]._ptr_buf_saved_frame;
					callbacks_ptr->load_game_state(buf);

					///CLEANUP
					WriteToProtectedMemory((uintptr_t)ptr_oldmem_load, oldmem_load, 3);
					///CLEANUP_END


				}
			}
			if (bckend) {
				ImGui::Text("SteamPeer2PeerBackend addr: %x", bckend);
				ImGui::Text("Sync addr: %x", &bckend->_sync);
				if (ImGui::Button("ggpo_advance_frame")) {
					bckend->_callbacks.advance_frame();
				}

				if (ImGui::TreeNode("Netcode details")) {
					//btw chardata resides on buf + 0x623E10 for P1 and for p2 buf + 0x623E10 + 0x24978
					if (&bckend->_sync) {
						static bool save_continuously = false;
						Sync__SavedState* saved_states = &bckend->_sync._savedstate;
						if (ImGui::Button("save_frame_state")) {
							//static byte** buf = &saved_states->frames[0].buf;
							byte** buf = &saved_states->frames[0].buf;
							saved_states->frames[0].cbuf = 0x8536f8;
							//static int save_frame_state_cbuf_static = 0xa1000;
							//static int save_frame_state_checksum_static = 0;
							saved_states->frames[0].checksum = 0;
							auto tst = bckend->_callbacks.save_game_state(buf, &saved_states->frames[0].cbuf, &saved_states->frames[0].checksum);
							savegame_count += 1;
						}

						if (ImGui::Button("Load frames in rollback 2")) {
							byte* buf = saved_states->frames[0].buf;
							typedef int (*AssemblyFunction)(unsigned char*);
							void* asmFunctionPtr = bckend->_callbacks.load_game_state;
							auto tst = bckend->_callbacks.load_game_state(buf);
							//AssemblyFunction asmFunction = reinterpret_cast<AssemblyFunction>(asmFunctionPtr);
							//int (*asmFunction)(unsigned char*) = (int (*)(unsigned char*))(asmFunctionPtr);
							//auto tst = asmFunction(buf);
							//int(*fptr1)(unsigned char*) = bckend->_callbacks.load_game_state;
							//int (*fptr1)(unsigned char*) bckend->_callbacks.load_game_state(buf);
						}
						for (int i = 0; i < 10; i++) {
							Sync__SavedFrame state_save = saved_states->frames[i];
							ImGui::Text("-------------------------");
							ImGui::Text("buf addr: %x", state_save.buf);
							ImGui::Text("cbuf: %d", state_save.cbuf);
							ImGui::Text("frame: %d", state_save.frame);
							ImGui::Text("checksum: %d", state_save.checksum);
							ImGui::Text("-------------------------");
						}
					}
					else {
						ImGui::Text("must have bckend->_sync initialized");
					}
					ImGui::TreePop();
				}
			}
			else {
				ImGui::Text("must have SteamPeer2PeerBackend initialized");
			}

		

			ImGui::TreePop();
		
		}
		ImGui::TreePop();
}
	if (!ImGui::CollapsingHeader("Game values"))
		return;

	if (ImGui::TreeNode("Character data"))
	{
		if (!g_interfaces.player1.IsCharDataNullPtr())
			ImGui::Text("pP1CharData 0x%p", g_interfaces.player1.GetData());
		if (!g_interfaces.player1.IsCharDataNullPtr())
			ImGui::Text("pP1CurrentScriptActionLocationInMemory 0x%p", g_interfaces.player1.GetData()->nextScriptLineLocationInMemory);

		if (!g_interfaces.player2.IsCharDataNullPtr())
			ImGui::Text("pP2CharData 0x%p", g_interfaces.player2.GetData());
		
		if (!g_interfaces.player2.IsCharDataNullPtr())
			ImGui::Text("pP2CurrentScriptActionLocationInMemory 0x%p", g_interfaces.player2.GetData()->nextScriptLineLocationInMemory);

		if (!g_interfaces.player2.IsCharDataNullPtr())
			ImGui::Text("pP2slot2_or_slot4 0x%x", g_interfaces.player2.GetData()->slot2_or_slot4);

		if (!g_interfaces.player1.IsCharDataNullPtr())
			ImGui::Text("P1CharIndex %d", g_interfaces.player1.GetData()->charIndex);

		if (!g_interfaces.player2.IsCharDataNullPtr())
			ImGui::Text("P2CharIndex %d", g_interfaces.player2.GetData()->charIndex);

		
			ImGui::Text("g_gameVals.isP1CPU: %d", g_gameVals.isP1CPU);



			//if (!g_interfaces.player2.IsCharDataNullPtr()) {
				//static bool lock = false;
				//static int offset = 0;
				//ImGui::Checkbox("Lock offset from value", &lock);
				//ImGui::InputInt("offset_to_set", &offset);
				//if (lock){
				//	char * start_buffer= g_interfaces.player2.GetData()->pad_1E79D;
				//	start_buffer[offset] = 0x01;
				//	start_buffer[offset+1] = 0x01;
				//	start_buffer[offset + 2] = 0x01;
				//	start_buffer[offset + 3] = 0x01;
				//	start_buffer[offset + 4] = 0x01;
				//}
				//ImGui::Text("g_interfaces.player2.GetData()->facingLeft: %d", g_interfaces.player2.GetData()->facingLeft);
				/*input buffers*/
			
				/*this will skip all the non motion inputs for now*/
		
				/*motion buffers Left side, size = 10A, the offsets listed are the offsets from input_buffer_flag_base*/
				//ImGui::Text("%x buffer_L_236 : %d", &(g_interfaces.player2.GetData()->buffer_R_236), g_interfaces.player2.GetData()->buffer_R_236);//0x1e848, +ac
				//ImGui::Text("%x buffer_L_623 : %d", &(g_interfaces.player2.GetData()->buffer_L_623), g_interfaces.player2.GetData()->buffer_L_623);//0x1e849, +ad
				//ImGui::Text("%x buffer_L_214 : %d", &(g_interfaces.player2.GetData()->buffer_L_214), g_interfaces.player2.GetData()->buffer_L_214);//0x1e84A, +ae
				//ImGui::Text("%x buffer_L_41236 : %d", &(g_interfaces.player2.GetData()->buffer_L_41236), g_interfaces.player2.GetData()->buffer_L_41236);//0x1e84B, +af
				//ImGui::Text("%x buffer_L_421 : %d", &(g_interfaces.player2.GetData()->buffer_L_421), g_interfaces.player2.GetData()->buffer_L_421);//0x1e84C, + b0
				//ImGui::Text("%x buffer_L_63214 : %d", &(g_interfaces.player2.GetData()->buffer_L_63214), g_interfaces.player2.GetData()->buffer_L_63214);//0x1e84D, + b1
				//ImGui::Text("%x buffer_L_236236 : %d", &(g_interfaces.player2.GetData()->buffer_L_236236), g_interfaces.player2.GetData()->buffer_L_236236);//0x1e84E, + b2
				//ImGui::Text("%x buffer_L_214214 : %d", &(g_interfaces.player2.GetData()->buffer_L_214214), g_interfaces.player2.GetData()->buffer_L_214214);//0x1e84F, + b3
			
				//ImGui::Text("%x buffer_L_6321463214 : %d", &(g_interfaces.player2.GetData()->buffer_L_6321463214), g_interfaces.player2.GetData()->buffer_L_6321463214);//0x1e851, + b5 (Double HCB, jubei astral)
				//ImGui::Text("%x buffer_L_632146_2 : %d", &(g_interfaces.player2.GetData()->buffer_L_632146_2), g_interfaces.player2.GetData()->buffer_L_632146_2);//0x1e852, + b6 (terumi 100 meter damage super)
			
				//ImGui::Text("%x buffer_L_2141236 : %d", &(g_interfaces.player2.GetData()->buffer_L_2141236), g_interfaces.player2.GetData()->buffer_L_2141236);//0x1E854, + b8 susanoo astral
				//ImGui::Text("%x buffer_L_2363214 : %d", &(g_interfaces.player2.GetData()->buffer_L_2363214), g_interfaces.player2.GetData()->buffer_L_2363214);//0x1E855, + b9 rachel astral
				//ImGui::Text("%x buffer_L_22 : %d", &(g_interfaces.player2.GetData()->buffer_L_22), g_interfaces.player2.GetData()->buffer_L_22);//0x1E856, + ba
				//ImGui::Text("%x buffer_L_46 : %d", &(g_interfaces.player2.GetData()->buffer_L_46), g_interfaces.player2.GetData()->buffer_L_46); //0x1E857, + bb
			
				//ImGui::Text("%x buffer_L_2H6 : %d", &(g_interfaces.player2.GetData()->buffer_L_2H6), g_interfaces.player2.GetData()->buffer_L_2H6); //0x1E859, + BD 2H6 -> hold 2 -> 6, charge(?)
				//ImGui::Text("%x buffer_L_6428 : %d", &(g_interfaces.player2.GetData()->buffer_L_6428), g_interfaces.player2.GetData()->buffer_L_6428);//0x1E85A, +BE
				//ImGui::Text("%x buffer_L_4H128 : %d", &(g_interfaces.player2.GetData()->buffer_L_4H128), g_interfaces.player2.GetData()->buffer_L_4H128);//0x1E85B, + BF taokaka astral
				//ImGui::Text("%x buffer_L_64641236 : %d", &(g_interfaces.player2.GetData()->buffer_L_64641236), g_interfaces.player2.GetData()->buffer_L_64641236);//0x1E85C, + C0 carl astral
				//ImGui::Text("%x buffer_L_412364123641236 : %d", &(g_interfaces.player2.GetData()->buffer_L_412364123641236), g_interfaces.player2.GetData()->buffer_L_412364123641236);//0x1E85D, + C1   Triple HCF (412364123641236) Litchi OD astral
				//ImGui::Text("%x buffer_L_KS28_ : %d", &(g_interfaces.player2.GetData()->buffer_L_KS28_), g_interfaces.player2.GetData()->buffer_L_KS28_);//0x1E85E, + C2, kagura stance, maybe it includes all characters not just kagura as long as D is pressed?
				//ImGui::Text("%x buffer_L_646 : %d", &(g_interfaces.player2.GetData()->buffer_L_646), g_interfaces.player2.GetData()->buffer_L_646);//0x1E85F, + C3
			
				//ImGui::Text("%x buffer_L_236236LambdaSuper : %d", &(g_interfaces.player2.GetData()->buffer_L_236236LambdaSuper), g_interfaces.player2.GetData()->buffer_L_236236LambdaSuper);//0x1E862, + C6 specific to lambda-11 super it seems
				//
				//ImGui::Text("%x buffer_L_360 : %d", &(g_interfaces.player2.GetData()->buffer_L_360), g_interfaces.player2.GetData()->buffer_L_360); //0x1E867, + CA
				//ImGui::Text("%x buffer_L_720 : %d", &(g_interfaces.player2.GetData()->buffer_L_720), g_interfaces.player2.GetData()->buffer_L_720); //0x1E868, + CB
		
				//ImGui::Text("%x buffer_L_222 : %d", &(g_interfaces.player2.GetData()->buffer_L_222), g_interfaces.player2.GetData()->buffer_L_222);//0x1E86A, + CD
				//ImGui::Text("%x padding_1E86B : %d", &(g_interfaces.player2.GetData()->padding_1E86B), g_interfaces.player2.GetData()->padding_1E86B);//0x1E86B
				//ImGui::Text("%x buffer_L_CMASH : %d", &(g_interfaces.player2.GetData()->buffer_L_CMASH), g_interfaces.player2.GetData()->buffer_L_CMASH);//0x1E86C, + CF susanoo's c mash special
				//ImGui::Text("%x padding_1E86D[8] : %d", &(g_interfaces.player2.GetData()->padding_1E86D[8]), g_interfaces.player2.GetData()->padding_1E86D[8]);//0x1E86D
				//ImGui::Text("%x buffer_L_8izanamifloat : %d", &(g_interfaces.player2.GetData()->buffer_L_8izanamifloat), g_interfaces.player2.GetData()->buffer_L_8izanamifloat); //0x1E875, +D9 input for izanami float
				//ImGui::Text("%x buffer_L_66 : %d", &(g_interfaces.player2.GetData()->buffer_L_66), g_interfaces.player2.GetData()->buffer_L_66); //0x1E876, +DA
				//ImGui::Text("%x buffer_L_44 : %d", &(g_interfaces.player2.GetData()->buffer_L_44), g_interfaces.player2.GetData()->buffer_L_44); //0x1E877, +DB
		
				//ImGui::Text("%x buffer_L_22jinsekkajin : %d", &(g_interfaces.player2.GetData()->buffer_L_22jinsekkajin), g_interfaces.player2.GetData()->buffer_L_22jinsekkajin); //0x1E87A, +DF specific to jins sekkajin
		
				//ImGui::Text("%x buffer_L_44nineseconddash : %d", &(g_interfaces.player2.GetData()->buffer_L_44nineseconddash), g_interfaces.player2.GetData()->buffer_L_44nineseconddash); //0x1E87C, +E1 specific for nine's second dash
				//ImGui::Text("%x buffer_L_66nineseconddash : %d", &(g_interfaces.player2.GetData()->buffer_L_66nineseconddash), g_interfaces.player2.GetData()->buffer_L_66nineseconddash); //0x1E87D, +E2 specific for nine's second dash
			
				//ImGui::Text("%x buffer_L_88nineseconddash : %d", &(g_interfaces.player2.GetData()->buffer_L_88nineseconddash), g_interfaces.player2.GetData()->buffer_L_88nineseconddash); //0x1E87A, +E4 specific for nine's second dash
			
				//ImGui::Text("%x buffer_L_1080 : %d", &(g_interfaces.player2.GetData()->buffer_L_1080), g_interfaces.player2.GetData()->buffer_L_1080); //0x1E893, +F6
				//ImGui::Text("%x buffer_L_1632143 : %d", &(g_interfaces.player2.GetData()->buffer_L_1632143), g_interfaces.player2.GetData()->buffer_L_1632143); //0x1E894, +F7
		
				//ImGui::Text("%x buffer_L_632146 : %d", &(g_interfaces.player2.GetData()->buffer_L_632146), g_interfaces.player2.GetData()->buffer_L_632146); //0x1E896, +F9
		
				//ImGui::Text("%x buffer_L_takemikazuchi_shit : %d", &(g_interfaces.player2.GetData()->buffer_L_takemikazuchi_shit), g_interfaces.player2.GetData()->buffer_L_takemikazuchi_shit); //0x1E899, +FD some specific stuff for the game's onslaught
				//ImGui::Text("%x buffer_L_4H6 : %d", &(g_interfaces.player2.GetData()->buffer_L_4H6), g_interfaces.player2.GetData()->buffer_L_4H6); //0x1E89A, +FE
				//ImGui::Text("%x buffer_L_2H8 : %d", &(g_interfaces.player2.GetData()->buffer_L_2H8), g_interfaces.player2.GetData()->buffer_L_2H8); //0x1E89B, +FF
		
				//ImGui::Text("%x buffer_L_ultimateshotkagura : %d", &(g_interfaces.player2.GetData()->buffer_L_4H1236), g_interfaces.player2.GetData()->buffer_L_4H1236); //0x1E89D, +101
				//ImGui::Text("%x buffer_L_82 : %d", &(g_interfaces.player2.GetData()->buffer_L_82), g_interfaces.player2.GetData()->buffer_L_82); //0x1E89E, +102
				//ImGui::Text("%x buffer_L_8H2 : %d", &(g_interfaces.player2.GetData()->buffer_L_8H2), g_interfaces.player2.GetData()->buffer_L_8H2); //0x1E89F, +103
			
				//ImGui::Text("%x buffer_L_236_3f : %d", &(g_interfaces.player2.GetData()->buffer_L_236_5f), g_interfaces.player2.GetData()->buffer_L_236_5f); //0x1E8A4, +108 236 within 3 frames of input valid, for mai's enhanced thrust
				//ImGui::Text("%x buffer_L_214_mai : %d", &(g_interfaces.player2.GetData()->buffer_L_214_5f), g_interfaces.player2.GetData()->buffer_L_214_5f); //0x1E8A5, +109 214 for mai backflip idk why its special //0x1E89F, +103
				//
				
		//}
		//	ImGui::Text("g_gameVals.P1InputJumpBackAddr: %d", g_gameVals.P1InputJumpBackAdress);

		ImGui::Separator();
		ImGui::Text("pP1PalIndex 0x%p", &(g_interfaces.player1.GetPalHandle().GetPalIndexRef()));
		ImGui::Text("pP2PalIndex 0x%p", &(g_interfaces.player2.GetPalHandle().GetPalIndexRef()));

		const int PAL_INDEX_MIN = 0;
		const int PAL_INDEX_MAX = 23;
		if (!g_interfaces.player1.GetPalHandle().IsNullPointerPalIndex())
			ImGui::SliderInt("P1PalIndex", &g_interfaces.player1.GetPalHandle().GetPalIndexRef(), PAL_INDEX_MIN, PAL_INDEX_MAX);

		if (!g_interfaces.player2.GetPalHandle().IsNullPointerPalIndex())
			ImGui::SliderInt("P2PalIndex", &g_interfaces.player2.GetPalHandle().GetPalIndexRef(), PAL_INDEX_MIN, PAL_INDEX_MAX);

		if (!g_interfaces.player1.GetPalHandle().IsNullPointerPalIndex())
			ImGui::Text("P1OrigPalIndex %d", g_interfaces.player1.GetPalHandle().GetOrigPalIndex());

		if (!g_interfaces.player2.GetPalHandle().IsNullPointerPalIndex())
			ImGui::Text("P2OrigPalIndex %d", g_interfaces.player2.GetPalHandle().GetOrigPalIndex());

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Game and Match variables"))
	{
		//!!!!UNCOMMENT LATER WHEN STATIC EXISTS
		//ImGui::Text("adress static snapshot: 0x%x ", &snapshot_replay_pre_allocated);
		ImGui::Text("sizeof chardata: 0x%d ", sizeof(CharData));
		ImGui::Text("sizeof snapshot: 0x%d ", sizeof(Snapshot));
		ImGui::Text("pGameState: 0x%p : %d", g_gameVals.pGameState, SafeDereferencePtr(g_gameVals.pGameState));
		ImGui::Text("pGameMode: 0x%p : %d", g_gameVals.pGameMode, SafeDereferencePtr(g_gameVals.pGameMode));
		ImGui::Text("pMatchState: 0x%p : %d", g_gameVals.pMatchState, SafeDereferencePtr(g_gameVals.pMatchState));
		ImGui::Text("pMatchTimer: 0x%p : %d", g_gameVals.pMatchTimer, SafeDereferencePtr(g_gameVals.pMatchTimer));
		ImGui::Text("pMatchRounds: 0x%p : %d", g_gameVals.pMatchRounds, SafeDereferencePtr(g_gameVals.pMatchRounds));
		ImGui::Text("pFrameCount: 0x%p : %d", g_gameVals.pFrameCount, SafeDereferencePtr((int*)g_gameVals.pFrameCount));
		ImGui::Text("pIsHUDHidden: 0x%p", g_gameVals.pIsHUDHidden);
		ImGui::Checkbox("pIsHUDHidden", (bool*)g_gameVals.pIsHUDHidden);
		ImGui::Text("entityCount: %d", g_gameVals.entityCount);
		ImGui::Text("pEntityList: 0x%p", g_gameVals.pEntityList);
		ImGui::Text("viewMatrix: 0x%p", g_gameVals.viewMatrix);
		ImGui::Text("projMatrix: 0x%p", g_gameVals.projMatrix);
		ImGui::Text("pRoom: 0x%p", g_gameVals.pRoom);
		ImGui::Text("enableForeignPalettes: %d", g_modVals.enableForeignPalettes);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Hitbox overlay"))
	{
		float& scale = WindowManager::GetInstance().GetWindowContainer()->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->GetScale();
		ImGui::SliderFloat("Hitbox overlay projection scale", &scale, 0.3f, 0.4f);

		ImGui::TreePop();
	}
}

void DebugWindow::DrawRoomSection()
{
	if (!ImGui::CollapsingHeader("Room"))
		return;
	ImGui::Text("g_modValsReplayUploadVeto: %d", g_modVals.uploadReplayDataVeto);
	if (!g_gameVals.pRoom || g_gameVals.pRoom->roomStatus == RoomStatus_Unavailable)
	{
		ImGui::TextUnformatted("Room is not available!");
		return;
	}

	if (ImGui::TreeNode("Room info"))
	{
		ImGui::Text("Status: %d", g_gameVals.pRoom->roomStatus);
		ImGui::Text("Name: %s", utf16_to_utf8(std::wstring(g_gameVals.pRoom->roomName)).c_str());
		ImGui::Text("Type: %d", g_gameVals.pRoom->roomType);
		ImGui::Text("Capacity: %d", g_gameVals.pRoom->capacity);
		ImGui::Text("Invitation: %d", g_gameVals.pRoom->invitation);
		ImGui::Text("Member count: %d", g_gameVals.pRoom->memberCount);
		ImGui::Text("Seconds elapsed: %d", g_gameVals.pRoom->secondsElapsed);
		ImGui::Text("Room Rematch: %x", g_gameVals.pRoom->roundsToWinPlusMatchLimitBitfield);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Room members"))
	{
		for (int i = 0; i < MAX_PLAYERS_IN_ROOM; i++)
		{
			const RoomMemberEntry* member = g_interfaces.pRoomManager->GetRoomMemberEntryByIndex(i);

			if (!member)
				continue;

			const char* name = g_interfaces.pRoomManager->GetPlayerSteamName(member->steamId);

			char buf[128];
			sprintf_s(buf, "%s##%d", name, i + 1);

			if (ImGui::TreeNode(buf))
			{
				ImGui::Text("MemberIndex: %d", member->memberIndex);
				ImGui::Text("MatchId: %d", member->matchId);
				ImGui::Text("MatchPlayerIndex: %d", member->matchPlayerIndex);
				ImGui::Text("SteamID: %llu", member->steamId);

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Room IM users"))
	{
		for (int i = 0; i < MAX_PLAYERS_IN_ROOM; i++)
		{
			const IMPlayer& player = g_interfaces.pRoomManager->m_imPlayers[i];
			const RoomMemberEntry* member = g_interfaces.pRoomManager->GetRoomMemberEntryByIndex(player.roomMemberIndex);

			if (!member || player.steamID.ConvertToUint64() == -1)
				continue;

			const char* name = g_interfaces.pRoomManager->GetPlayerSteamName(member->steamId);

			char buf[128];
			sprintf_s(buf, "%s##%d", player.steamName.c_str(), player.roomMemberIndex);

			if (ImGui::TreeNode(buf))
			{
				ImGui::Text("MemberIndex: %d", player.roomMemberIndex);
				ImGui::Text("SteamID: %llu", player.steamID.ConvertToUint64());

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("IM users in current match"))
	{
		if (g_interfaces.pRoomManager->IsThisPlayerInMatch())
		{
			ImGui::Text("MatchID: %d", g_interfaces.pRoomManager->GetThisPlayerRoomMemberEntry()->matchId);

			for (const IMPlayer& player : g_interfaces.pRoomManager->GetIMPlayersInCurrentMatch())
			{
				const RoomMemberEntry* member = g_interfaces.pRoomManager->GetRoomMemberEntryByIndex(player.roomMemberIndex);

				char buf[128];
				sprintf_s(buf, "%s##%d", player.steamName.c_str(), player.roomMemberIndex);

				if (ImGui::TreeNode(buf))
				{
					ImGui::Text("MemberIndex: %d", player.roomMemberIndex);
					ImGui::Text("MatchPlayerIndex: %d", member->matchPlayerIndex);
					ImGui::Text("SteamID: %llu", player.steamID.ConvertToUint64());

					ImGui::TreePop();
				}
			}
			ImGui::Text("---------------------------GetIMPlayersInCurrentMatchNonSpec------------------------------------------");
			ImGui::Text("MatchID: %d", g_interfaces.pRoomManager->GetThisPlayerRoomMemberEntry()->matchId);

			for (const IMPlayer& player : g_interfaces.pRoomManager->GetIMPlayersInCurrentMatchNonSpec())
			{
				const RoomMemberEntry* member = g_interfaces.pRoomManager->GetRoomMemberEntryByIndex(player.roomMemberIndex);

				char buf[128];
				sprintf_s(buf, "%s##%d", player.steamName.c_str(), player.roomMemberIndex);

				if (ImGui::TreeNode(buf))
				{
					ImGui::Text("MemberIndex: %d", player.roomMemberIndex);
					ImGui::Text("MatchPlayerIndex: %d", member->matchPlayerIndex);
					ImGui::Text("SteamID: %llu", player.steamID.ConvertToUint64());

					ImGui::TreePop();
				}
			}
		}
		else
		{
			ImGui::TextUnformatted("Not in match!");
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Custom gamemode selection"))
	{
		ImGui::Text("Player1 choice: %s",
			g_interfaces.pGameModeManager->GetGameModeName(g_interfaces.pOnlineGameModeManager->GetPlayer1GameModeChoice()).c_str());

		ImGui::Text("Player2 choice: %s",
			g_interfaces.pGameModeManager->GetGameModeName(g_interfaces.pOnlineGameModeManager->GetPlayer2GameModeChoice()).c_str());

		ImGui::Text("Activated gamemode: %s",
			g_interfaces.pGameModeManager->GetGameModeName(g_interfaces.pGameModeManager->GetActiveGameMode()).c_str());

		ImGui::TreePop();
	}

	if (ImGui::Button("Send announce"))
	{
		g_interfaces.pRoomManager->SendAnnounce();
	}

	if (ImGui::Button("Set rich presence"))
	{
		g_interfaces.pSteamFriendsWrapper->SetRichPresence("status", "In Match / Spectate Room");
		g_interfaces.pSteamFriendsWrapper->SetRichPresence("connect", "+connect_lobby 109775241036175527");

		// In case of lobby or ranked we might have no lobbyId. Go with room owner's steamId instead.
		g_interfaces.pSteamFriendsWrapper->SetRichPresence("steam_player_group", std::to_string(g_gameVals.pRoom->member1.steamId).c_str());
		g_interfaces.pSteamFriendsWrapper->SetRichPresence("steam_player_group_size", std::to_string(g_gameVals.pRoom->memberCount).c_str());
	}

	if (ImGui::Button("Clear rich presence"))
	{
		g_interfaces.pSteamFriendsWrapper->ClearRichPresence();
	}
}
void DebugWindow::DrawSettingsSection() {
	if (!ImGui::CollapsingHeader("Settings"))
		return;
	if (ImGui::Button("Disable UploadReplayData")) {
		Settings::changeSetting("UploadReplayData", std::to_string(0));
		Settings::loadSettingsFile();

	}
	
}
void DebugWindow::DrawNotificationSection()
{
	if (!ImGui::CollapsingHeader("Notification"))
		return;

	const int BUF_SIZE = 1024;

	char text[BUF_SIZE] = "Opponent BBCFIM detected - Ping: 130ms (Press F2 for more info)";

	ImGui::SliderFloat("Seconds to slide", &g_notificationBar->m_secondsToSlide, 0, 15, "%.1f");
	ImGui::InputText("##Notification", text, BUF_SIZE);

	if (ImGui::Button("Add notification"))
	{
		g_notificationBar->AddNotification(text);
	}

	if (ImGui::Button("Clear notifications"))
	{
		g_notificationBar->ClearNotifications();
	}
}
