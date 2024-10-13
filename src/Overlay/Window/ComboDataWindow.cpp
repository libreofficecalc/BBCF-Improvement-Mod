#include "ComboDataWindow.h"

#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Core/info.h"


void ComboDataWindow::Draw() {
	DrawMainSection();
}

void ComboDataWindow::DrawMainSection() {
	if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
		CharData* p1 = g_interfaces.player1.GetData();
		CharData* p2 = g_interfaces.player2.GetData();
		auto starter_rating = ""; int histun_decay = 0;
		static int player_radio = 0;
		ImGui::RadioButton("P1", &player_radio, 0);
		ImGui::SameLine();
		ImGui::RadioButton("P2", &player_radio, 1);
		//make it clearer later
		if (player_radio == 0) {
			p1 = g_interfaces.player1.GetData();
			p2 = g_interfaces.player2.GetData();
		}
		else {
			p1 = g_interfaces.player2.GetData();
			p2 = g_interfaces.player1.GetData();
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
		ImGui::Text("Heat Generated: %d", p2->heatGeneratedForCombo);
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