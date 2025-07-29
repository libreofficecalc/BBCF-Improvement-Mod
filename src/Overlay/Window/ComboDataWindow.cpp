#include "ComboDataWindow.h"

#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Core/info.h"


void ComboDataWindow::Draw() {
	DrawMainSection();
}
struct HeatControl {
	HeatControl(CharData* playerObj, CharData* opponentObj)
		: player(playerObj),
		opponent(opponentObj),
		previous_heat(playerObj->heatMeter),
		heat_gained(0){}
	CharData* player = nullptr;
	CharData* opponent = nullptr;
	int previous_heat = 0;
	int heat_gained = 0;
	void HeatControl::set_char_objs(CharData* playerObj, CharData* opponentObj) {
		player = playerObj;
		opponent = opponentObj;
	};
	void HeatControl::calculate_heat_gain() {
		int currentHeat = player->heatMeter;
		int delta = currentHeat - previous_heat;
		if (delta > 0 && opponent->heatGeneratedForCombo > 0) {
			heat_gained += delta;
		}
		if (delta < 0 && opponent->heatGeneratedForCombo > 0) {
			previous_heat = currentHeat;
		}
		if (opponent->hitCount == 1) {
			//necessary when the combo starts with a move that spends and generates heat, like jin's ex moves
			//heatGeneratedForCombo does not take into account heat cd
			float heat_cd_mult = 1;
			if (player->heatGainCooldown > 0) {
				heat_cd_mult = 0.25f;
			}
			heat_gained = floor(opponent->heatGeneratedForCombo * heat_cd_mult);
		}

		if (*g_gameVals.pFrameCount == 1) {
			//resets all values when training mode is reset while the combo data window is open
			currentHeat = 0;
			previous_heat = 0;
			heat_gained = 0;
		}

		previous_heat = currentHeat;
	}
		
};

void ComboDataWindow::DrawMainSection() {
	if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
		CharData* p1 = g_interfaces.player1.GetData();
		CharData* p2 = g_interfaces.player2.GetData();
		auto starter_rating = ""; int histun_decay = 0;
		HeatControl* hc;

		static HeatControl heat_control_p1 = HeatControl(p1,p2);
		static HeatControl heat_control_p2 = HeatControl(p2,p1);
		static int player_radio = 0;
		static int current_heat = 0;
		static int previous_heat = 0;
		static int heat_gained = 0;
		ImGui::RadioButton("P1", &player_radio, 0);
		ImGui::SameLine();
		ImGui::RadioButton("P2", &player_radio, 1);
		//calculate heat gain for both players, regardless of which is currently chosen
		heat_control_p1.set_char_objs(p1, p2);
		heat_control_p1.calculate_heat_gain();
		heat_control_p2.set_char_objs(p2, p1);
		heat_control_p2.calculate_heat_gain();
		//make it clearer later
		if (player_radio == 0) {
			p1 = g_interfaces.player1.GetData();
			p2 = g_interfaces.player2.GetData();
			hc = &heat_control_p1;
		}
		else {
			p1 = g_interfaces.player2.GetData();
			p2 = g_interfaces.player1.GetData();
			hc = &heat_control_p2;
		}
		if (p2->starterRating == 1) {
			starter_rating = "short";
		}
		else if (p2->starterRating == 2) {
			starter_rating = "normal";
		}
		else if (p2->starterRating == 3) {
			starter_rating = "long";
		}
		else { starter_rating = ""; }
		if (p2->comboTime > 660) {
			histun_decay = -100000;
		}
		else if (p2->comboTime > 480) {
			histun_decay = -10;
		}
		else if (p2->comboTime > 300) {
			histun_decay = -5;
		}
		else if (p2->comboTime > 120) {
			histun_decay = -2;
		}
		ImGui::Text("Starter Rating: %s", starter_rating);
		ImGui::Text("Combo Time: %d", p2->comboTime);
		ImGui::Text("Hitstun: %d", p2->hitstun);
		ImGui::Text("Hitstun Decay: %dF", histun_decay);
		ImGui::Text("Combo Proration: %d", p2->comboProration);
		ImGui::Text("Heat Generated: %d", hc->heat_gained);
		ImGui::Text("Heat Cooldown: %d", p1->heatGainCooldown);
		if (ImGui::TreeNode("Same Move Proration Stack")) {
			char* smp_stack_location = p2->sameMoveProrationStack;
			for (int iter = 0; iter < 5; iter++) {
				
				ImGui::Text("%s", smp_stack_location);
				smp_stack_location += 32;
			}
			ImGui::TreePop();
		}
		if (ImGui::Button(" + ")) {
			ImGui::SetWindowFontScale(1.8f);
		}
		ImGui::SameLine();
		if (ImGui::Button(" - ")) {
			ImGui::SetWindowFontScale(1.0f);
		}
		//ImGui::Text("Heat from combo2: %d", p1->heatGeneratedForCombo);
		
	}
}