#include "InputBufferWindow.h"
#include <string>
#include <vector>
#include "Core/interfaces.h"
#include "Game/Player.h"
#include "Overlay/fonts.h"

#define GREEN ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
void InputBufferWindow::draw_right_side_buffers(CharData* player_data) {
	//if (g_interfaces.player2.GetData()->buffer_R_236) {
	//	ImGui::TextColored(GREEN,"%x R_236 : %d", &(player_data->buffer_R_236), player_data->buffer_R_236);
	//}
	for (auto &pair: input_buffer_map_right ) {
		//removes the side prefix, only allow for side prefix when both are on at the same time
		auto fmt_str = pair.first.c_str();
		auto addr = pair.second;
		if (*addr) {
			ImGui::TextColored(GREEN, fmt_str+2,  *addr);
		}
		else {
			ImGui::TextDisabled(fmt_str+2,  *addr);
		}
	}
}

void InputBufferWindow::draw_left_side_buffers(CharData* player_data) {
	//if (g_interfaces.player2.GetData()->buffer_L_236) {
	//	ImGui::Text("%x L_236 : %d", &(player_data->buffer_L_236), player_data->buffer_L_236);
	//}
	for (auto& pair : input_buffer_map_left) {
		auto fmt_str = pair.first.c_str();
		auto addr = pair.second;
		if (*addr) {
			ImGui::TextColored(GREEN, fmt_str+2, *addr);
		}
		else {
			ImGui::TextDisabled(fmt_str+2, *addr);
		}
	}
}
void InputBufferWindow::draw_active_buffers(CharData* player_data) {
	if (player_data->facingLeft == 0) {
		draw_left_side_buffers(player_data);
	}
	else {
		draw_right_side_buffers(player_data);
		
	}
    return;
}

void InputBufferWindow::Draw()
{
	//ImGuiIO& io = ImGui::GetIO();
	//ImFontAtlas* fontAtlas = io.Fonts;

	//if (fontAtlas->Fonts.size() > 1) {
	//	ImGui::PushFont(fontAtlas->Fonts[0]);
	//}

	
	//ImGui::SetWindowFontScale(1.5f);
    if (g_interfaces.player2.IsCharDataNullPtr() || g_interfaces.player1.IsCharDataNullPtr()) {
		InputBufferWindow::input_buffer_map_left = {};
		InputBufferWindow::input_buffer_map_right = {};

        return;
   }
	CharData* player;
	if (InputBufferWindow::player_number == 1) {
		player = g_interfaces.player1.GetData();
	}
	else {
		player = g_interfaces.player2.GetData();
	}
	if (InputBufferWindow::input_buffer_map_left.empty() ||
		InputBufferWindow::input_buffer_map_right.empty()) {

		initialize_buffer_maps(player);
	}
	draw_active_buffers(player);
	if (ImGui::Button(" + ")) {
		ImGui::SetWindowFontScale(1.5f);
	}
	ImGui::SameLine();
	if (ImGui::Button(" - ")) {
		ImGui::SetWindowFontScale(1.0f);
	}
	//if (fontAtlas->Fonts.size() > 1) {
	//	ImGui::PopFont();
	//}
	//ImGui::PopFont();
	if (false) {
		ImGui::Text("%x buffer_L_236 : %d", &(g_interfaces.player2.GetData()->buffer_L_236), g_interfaces.player2.GetData()->buffer_L_236);//0x1e848, +ac
		ImGui::Text("%x buffer_L_623 : %d", &(g_interfaces.player2.GetData()->buffer_L_623), g_interfaces.player2.GetData()->buffer_L_623);//0x1e849, +ad
		ImGui::Text("%x buffer_L_214 : %d", &(g_interfaces.player2.GetData()->buffer_L_214), g_interfaces.player2.GetData()->buffer_L_214);//0x1e84A, +ae
		ImGui::Text("%x buffer_L_41236 : %d", &(g_interfaces.player2.GetData()->buffer_L_41236), g_interfaces.player2.GetData()->buffer_L_41236);//0x1e84B, +af
		ImGui::Text("%x buffer_L_421 : %d", &(g_interfaces.player2.GetData()->buffer_L_421), g_interfaces.player2.GetData()->buffer_L_421);//0x1e84C, + b0
		ImGui::Text("%x buffer_L_63214 : %d", &(g_interfaces.player2.GetData()->buffer_L_63214), g_interfaces.player2.GetData()->buffer_L_63214);//0x1e84D, + b1
		ImGui::Text("%x buffer_L_236236 : %d", &(g_interfaces.player2.GetData()->buffer_L_236236), g_interfaces.player2.GetData()->buffer_L_236236);//0x1e84E, + b2
		ImGui::Text("%x buffer_L_214214 : %d", &(g_interfaces.player2.GetData()->buffer_L_214214), g_interfaces.player2.GetData()->buffer_L_214214);//0x1e84F, + b3

		ImGui::Text("%x buffer_L_6321463214 : %d", &(g_interfaces.player2.GetData()->buffer_L_6321463214), g_interfaces.player2.GetData()->buffer_L_6321463214);//0x1e851, + b5 (Double HCB, jubei astral)
		ImGui::Text("%x buffer_L_632146_2 : %d", &(g_interfaces.player2.GetData()->buffer_L_632146_2), g_interfaces.player2.GetData()->buffer_L_632146_2);//0x1e852, + b6 (terumi 100 meter damage super)

		ImGui::Text("%x buffer_L_2141236 : %d", &(g_interfaces.player2.GetData()->buffer_L_2141236), g_interfaces.player2.GetData()->buffer_L_2141236);//0x1E854, + b8 susanoo astral
		ImGui::Text("%x buffer_L_2363214 : %d", &(g_interfaces.player2.GetData()->buffer_L_2363214), g_interfaces.player2.GetData()->buffer_L_2363214);//0x1E855, + b9 rachel astral
		ImGui::Text("%x buffer_L_22 : %d", &(g_interfaces.player2.GetData()->buffer_L_22), g_interfaces.player2.GetData()->buffer_L_22);//0x1E856, + ba
		ImGui::Text("%x buffer_L_46 : %d", &(g_interfaces.player2.GetData()->buffer_L_46), g_interfaces.player2.GetData()->buffer_L_46); //0x1E857, + bb

		ImGui::Text("%x buffer_L_2H8jinastral : %d", &(g_interfaces.player2.GetData()->buffer_L_2H8jinastral), g_interfaces.player2.GetData()->buffer_L_2H8jinastral); //0x1E859, + BD 2H6 -> hold 2 -> 6, charge(?)
		ImGui::Text("%x buffer_L_6428 : %d", &(g_interfaces.player2.GetData()->buffer_L_6428), g_interfaces.player2.GetData()->buffer_L_6428);//0x1E85A, +BE
		ImGui::Text("%x buffer_L_4H128 : %d", &(g_interfaces.player2.GetData()->buffer_L_4H128), g_interfaces.player2.GetData()->buffer_L_4H128);//0x1E85B, + BF taokaka astral
		ImGui::Text("%x buffer_L_64641236 : %d", &(g_interfaces.player2.GetData()->buffer_L_64641236), g_interfaces.player2.GetData()->buffer_L_64641236);//0x1E85C, + C0 carl astral
		ImGui::Text("%x buffer_L_412364123641236 : %d", &(g_interfaces.player2.GetData()->buffer_L_412364123641236), g_interfaces.player2.GetData()->buffer_L_412364123641236);//0x1E85D, + C1   Triple HCF (412364123641236) Litchi OD astral
		ImGui::Text("%x buffer_L_KS28_ : %d", &(g_interfaces.player2.GetData()->buffer_L_KS28_), g_interfaces.player2.GetData()->buffer_L_KS28_);//0x1E85E, + C2, kagura stance, maybe it includes all characters not just kagura as long as D is pressed?
		ImGui::Text("%x buffer_L_646 : %d", &(g_interfaces.player2.GetData()->buffer_L_646), g_interfaces.player2.GetData()->buffer_L_646);//0x1E85F, + C3

		ImGui::Text("%x buffer_L_236236LambdaSuper : %d", &(g_interfaces.player2.GetData()->buffer_L_236236LambdaSuper), g_interfaces.player2.GetData()->buffer_L_236236LambdaSuper);//0x1E862, + C6 specific to lambda-11 super it seems

		ImGui::Text("%x buffer_L_360 : %d", &(g_interfaces.player2.GetData()->buffer_L_360), g_interfaces.player2.GetData()->buffer_L_360); //0x1E867, + CA
		ImGui::Text("%x buffer_L_720 : %d", &(g_interfaces.player2.GetData()->buffer_L_720), g_interfaces.player2.GetData()->buffer_L_720); //0x1E868, + CB

		ImGui::Text("%x buffer_L_222 : %d", &(g_interfaces.player2.GetData()->buffer_L_222), g_interfaces.player2.GetData()->buffer_L_222);//0x1E86A, + CD
		ImGui::Text("%x padding_1E86B : %d", &(g_interfaces.player2.GetData()->padding_1E86B), g_interfaces.player2.GetData()->padding_1E86B);//0x1E86B
		ImGui::Text("%x buffer_L_CMASH : %d", &(g_interfaces.player2.GetData()->buffer_L_CMASH), g_interfaces.player2.GetData()->buffer_L_CMASH);//0x1E86C, + CF susanoo's c mash special
		ImGui::Text("%x padding_1E86D[8] : %d", &(g_interfaces.player2.GetData()->padding_1E86D[8]), g_interfaces.player2.GetData()->padding_1E86D[8]);//0x1E86D
		ImGui::Text("%x buffer_L_8izanamifloat : %d", &(g_interfaces.player2.GetData()->buffer_L_8izanamifloat), g_interfaces.player2.GetData()->buffer_L_8izanamifloat); //0x1E875, +D9 input for izanami float
		ImGui::Text("%x buffer_L_66 : %d", &(g_interfaces.player2.GetData()->buffer_L_66), g_interfaces.player2.GetData()->buffer_L_66); //0x1E876, +DA
		ImGui::Text("%x buffer_L_44 : %d", &(g_interfaces.player2.GetData()->buffer_L_44), g_interfaces.player2.GetData()->buffer_L_44); //0x1E877, +DB

		ImGui::Text("%x buffer_L_22jinsekkajin : %d", &(g_interfaces.player2.GetData()->buffer_L_22jinsekkajin), g_interfaces.player2.GetData()->buffer_L_22jinsekkajin); //0x1E87A, +DF specific to jins sekkajin

		ImGui::Text("%x buffer_L_44nineseconddash : %d", &(g_interfaces.player2.GetData()->buffer_L_44nineseconddash), g_interfaces.player2.GetData()->buffer_L_44nineseconddash); //0x1E87C, +E1 specific for nine's second dash
		ImGui::Text("%x buffer_L_66nineseconddash : %d", &(g_interfaces.player2.GetData()->buffer_L_66nineseconddash), g_interfaces.player2.GetData()->buffer_L_66nineseconddash); //0x1E87D, +E2 specific for nine's second dash

		ImGui::Text("%x buffer_L_88nineseconddash : %d", &(g_interfaces.player2.GetData()->buffer_L_88nineseconddash), g_interfaces.player2.GetData()->buffer_L_88nineseconddash); //0x1E87A, +E4 specific for nine's second dash

		ImGui::Text("%x buffer_L_1080 : %d", &(g_interfaces.player2.GetData()->buffer_L_1080), g_interfaces.player2.GetData()->buffer_L_1080); //0x1E893, +F6
		ImGui::Text("%x buffer_L_1632143 : %d", &(g_interfaces.player2.GetData()->buffer_L_1632143), g_interfaces.player2.GetData()->buffer_L_1632143); //0x1E894, +F7

		ImGui::Text("%x buffer_L_632146 : %d", &(g_interfaces.player2.GetData()->buffer_L_632146), g_interfaces.player2.GetData()->buffer_L_632146); //0x1E896, +F9

		ImGui::Text("%x buffer_L_takemikazuchi_shit : %d", &(g_interfaces.player2.GetData()->buffer_L_takemikazuchi_shit), g_interfaces.player2.GetData()->buffer_L_takemikazuchi_shit); //0x1E899, +FD some specific stuff for the game's onslaught
		ImGui::Text("%x buffer_L_4H6 : %d", &(g_interfaces.player2.GetData()->buffer_L_4H6), g_interfaces.player2.GetData()->buffer_L_4H6); //0x1E89A, +FE
		ImGui::Text("%x buffer_L_2H8 : %d", &(g_interfaces.player2.GetData()->buffer_L_2H8), g_interfaces.player2.GetData()->buffer_L_2H8); //0x1E89B, +FF

		ImGui::Text("%x buffer_L_ultimateshotkagura : %d", &(g_interfaces.player2.GetData()->buffer_L_4H1236), g_interfaces.player2.GetData()->buffer_L_4H1236); //0x1E89D, +101
		ImGui::Text("%x buffer_L_82 : %d", &(g_interfaces.player2.GetData()->buffer_L_82), g_interfaces.player2.GetData()->buffer_L_82); //0x1E89E, +102
		ImGui::Text("%x buffer_L_8H2 : %d", &(g_interfaces.player2.GetData()->buffer_L_8H2), g_interfaces.player2.GetData()->buffer_L_8H2); //0x1E89F, +103

		ImGui::Text("%x buffer_L_236_5f : %d", &(g_interfaces.player2.GetData()->buffer_L_236_5f), g_interfaces.player2.GetData()->buffer_L_236_5f); //0x1E8A4, +108 236 within 3 frames of input valid, for mai's enhanced thrust
		ImGui::Text("%x buffer_L_214_5f : %d", &(g_interfaces.player2.GetData()->buffer_L_214_5f), g_interfaces.player2.GetData()->buffer_L_214_5f); //0x1E8A5, +109 214 for mai backflip idk why its special //0x1E89F, +103
	}

}

void InputBufferWindow::initialize_buffer_maps(CharData* player_data) {
	InputBufferWindow::input_buffer_map_left = std::vector<std::pair< std::string, int8_t* >>{ 
		{std::string("L_66 : %d"), &(player_data->buffer_L_66)},
		{std::string("L_44 : %d"), &(player_data->buffer_L_44)},
		{std::string("L_236 : %d"), &(player_data->buffer_L_236)},
		{std::string("L_623 : %d"), &(player_data->buffer_L_623)},
		{std::string("L_214 : %d"), &(player_data->buffer_L_214)},
		{std::string("L_41236 : %d"), &(player_data->buffer_L_41236)},
		{std::string("L_63214 : %d"), &(player_data->buffer_L_63214)},
		{std::string("L_236236 : %d"), &(player_data->buffer_L_236236)},
		{std::string("L_214214 : %d"), &(player_data->buffer_L_214214)},
		{std::string("L_GIRADA : %d"), &(player_data->buffer_L_360)},
		{std::string("L_GIRADONA : %d"), &(player_data->buffer_L_720)},
		{std::string("L_GIRADASSA : %d"), &(player_data->buffer_L_1080)},

		{std::string("L_421 : %d"), &(player_data->buffer_L_421)},
		{std::string("L_6321463214 : %d"), &(player_data->buffer_L_6321463214)},
		{std::string("L_632146_2 : %d"), &(player_data->buffer_L_632146_2)},

		{std::string("L_2141236 : %d"), &(player_data->buffer_L_2141236)},
		{std::string("L_2363214 : %d"), &(player_data->buffer_L_2363214)},
		{std::string("L_22 : %d"), &(player_data->buffer_L_22)},
		{std::string("L_46 : %d"), &(player_data->buffer_L_46)},

		{std::string("L_2H8jinastral : %d"), &(player_data->buffer_L_2H8jinastral)},
		{std::string("L_6428 : %d"), &(player_data->buffer_L_6428)},
		{std::string("L_4H128 : %d"), &(player_data->buffer_L_4H128)},
		{std::string("L_64641236 : %d"), &(player_data->buffer_L_64641236)},
		{std::string("L_412364123641236 : %d"), &(player_data->buffer_L_412364123641236)},
		{std::string("L_KS28_ : %d"), &(player_data->buffer_L_KS28_)},
		{std::string("L_646 : %d"), &(player_data->buffer_L_646)},

		{std::string("L_236236LambdaSuper : %d"), &(player_data->buffer_L_236236LambdaSuper)},

		

		{std::string("L_222 : %d"), &(player_data->buffer_L_222)},
	
		{std::string("L_CMASH : %d"),& (player_data->buffer_L_CMASH)},

		{std::string("L_8izanamifloat : %d"),& (player_data->buffer_L_8izanamifloat)},
		

		{std::string("L_22jinsekkajin : %d"), &(player_data->buffer_L_22jinsekkajin)},

		{std::string("L_44nineseconddash : %d"), &(player_data->buffer_L_44nineseconddash)},
		{std::string("L_66nineseconddash : %d"), &(player_data->buffer_L_66nineseconddash)},

		{std::string("L_88nineseconddash : %d"), &(player_data->buffer_L_88nineseconddash)},

		
		{std::string("L_1632143 : %d"), &(player_data->buffer_L_1632143)},

		{std::string("L_632146 : %d"), &(player_data->buffer_L_632146)},

		{std::string("L_takemikazuchi_shit : %d"), &(player_data->buffer_L_takemikazuchi_shit)},
		{std::string("L_4H6 : %d"), &(player_data->buffer_L_4H6)},
		{std::string("L_2H8 : %d"), &(player_data->buffer_L_2H8)},

		{std::string("L_ultimateshotkagura : %d"), &(player_data->buffer_L_4H1236)},
		{std::string("L_82 : %d"), &(player_data->buffer_L_82)},
		{std::string("L_8H2 : %d"), &(player_data->buffer_L_8H2)},

		{std::string("L_236_5f : %d"), &(player_data->buffer_L_236_5f)},
		{std::string("L_214_5f : %d"), &(player_data->buffer_L_214_5f)},
	};
	InputBufferWindow::input_buffer_map_right= std::vector<std::pair< std::string, int8_t* >>{
		{std::string("R_66 : %d"), &(player_data->buffer_R_66)},
		{std::string("R_44 : %d"), &(player_data->buffer_R_44)},
		{std::string("R_236 : %d"), &(player_data->buffer_R_236)},
		{std::string("R_623 : %d"), &(player_data->buffer_R_623)},
		{std::string("R_214 : %d"), &(player_data->buffer_R_214)},
		{std::string("R_41236 : %d"), &(player_data->buffer_R_41236)},
		{std::string("R_63214 : %d"), &(player_data->buffer_R_63214)},
		{std::string("R_236236 : %d"), &(player_data->buffer_R_236236)},
		{std::string("R_214214 : %d"), &(player_data->buffer_R_214214)},
		{std::string("R_GIRADA : %d"), &(player_data->buffer_R_360)},
		{std::string("R_GIRADONA : %d"), &(player_data->buffer_R_720)},
		{std::string("R_GIRADASSA : %d"), &(player_data->buffer_R_1080)},

		{std::string("R_421 : %d"), &(player_data->buffer_R_421)},
		{std::string("R_6321463214 : %d"), &(player_data->buffer_R_6321463214)},
		{std::string("R_632146_2 : %d"), &(player_data->buffer_R_632146_2)},

		{std::string("R_2141236 : %d"), &(player_data->buffer_R_2141236)},
		{std::string("R_2363214 : %d"), &(player_data->buffer_R_2363214)},
		{std::string("R_22 : %d"), &(player_data->buffer_R_22)},
		{std::string("R_46 : %d"), &(player_data->buffer_R_46)},

		{std::string("R_2H8jinastral : %d"), &(player_data->buffer_R_2H8jinastral)},
		{std::string("R_6428 : %d"), &(player_data->buffer_R_6428)},
		{std::string("R_4H128 : %d"), &(player_data->buffer_R_4H128)},
		{std::string("R_64641236 : %d"), &(player_data->buffer_R_64641236)},
		{std::string("R_412364123641236 : %d"), &(player_data->buffer_R_412364123641236)},
		{std::string("R_KS28_ : %d"), &(player_data->buffer_R_KS28_)},
		{std::string("R_646 : %d"), &(player_data->buffer_R_646)},

		{std::string("R_236236LambdaSuper : %d"), &(player_data->buffer_R_236236LambdaSuper)},



		{std::string("R_222 : %d"), &(player_data->buffer_R_222)},

		{std::string("R_CMASH : %d"),&(player_data->buffer_R_CMASH)},

		{std::string("R_8izanamifloat : %d"),&(player_data->buffer_R_8izanamifloat)},
		

		{std::string("R_22jinsekkajin : %d"), &(player_data->buffer_R_22jinsekkajin)},

		{std::string("R_44nineseconddash : %d"), &(player_data->buffer_R_44nineseconddash)},
		{std::string("R_66nineseconddash : %d"), &(player_data->buffer_R_66nineseconddash)},

		{std::string("R_88nineseconddash : %d"), &(player_data->buffer_R_88nineseconddash)},


		{std::string("R_1632143 : %d"), &(player_data->buffer_R_1632143)},

		{std::string("R_632146 : %d"), &(player_data->buffer_R_632146)},

		{std::string("R_takemikazuchi_shit : %d"), &(player_data->buffer_R_takemikazuchi_shit)},
		{std::string("R_4H6 : %d"), &(player_data->buffer_R_4H6)},
		{std::string("R_2H8 : %d"), &(player_data->buffer_R_2H8)},

		{std::string("R_ultimateshotkagura : %d"), &(player_data->buffer_R_4H1236)},
		{std::string("R_82 : %d"), &(player_data->buffer_R_82)},
		{std::string("R_8H2 : %d"), &(player_data->buffer_R_8H2)},

		{std::string("R_236_5f : %d"), &(player_data->buffer_R_236_5f)},
		{std::string("R_214_5f : %d"), &(player_data->buffer_R_214_5f)},
	};
}
