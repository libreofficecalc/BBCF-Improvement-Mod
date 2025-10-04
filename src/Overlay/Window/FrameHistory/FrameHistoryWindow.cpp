#include "FrameHistoryWindow.h"
#include "Game/gamestates.h"


#include "Core/interfaces.h"
#include "Game/gamestates.h"
#include "imgui_internal.h"
#include "Core/utils.h"

#include <array>
#include <vector>
#include <cmath>

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define MIN(a,b)            (((a) > (b)) ? (b) : (a))


std::vector<std::array<ImVec2, 2>> DottedLine(ImVec2 start, ImVec2 end, int divisions, float empty_ratio) {
	std::vector<std::array<ImVec2, 2>> dotted_line = std::vector<std::array<ImVec2, 2>>();
	ImVec2 direction = ImVec2(end.x - start.x, end.y - start.y);
	float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

	direction.x /= length;
	direction.y /= length;

	float seg_len = length / divisions;
	float sep_len = seg_len * empty_ratio;
	seg_len *= 1 - empty_ratio;


	for (int i = 0; i < divisions; ++i) {
		std::array<ImVec2, 2> segment = {
		  ImVec2(start.x + direction.x * i * (seg_len + sep_len), start.y + direction.y * i * (seg_len + sep_len)),
		  ImVec2(start.x + direction.x * (i + 1) * (seg_len + sep_len) - sep_len * direction.x, start.y + direction.y * (i + 1) * (seg_len + sep_len) - sep_len * direction.y
				 ) };
		dotted_line.push_back(segment);
	}
	return dotted_line;
}

void DrawSegWithOffset(ImDrawList* drawlist, ImVec2 offset, ImVec2 seg_start, ImVec2 seg_end) {
	seg_start.x += offset.x;
	seg_end.x += offset.x;
	seg_start.y += offset.y;
	seg_end.y += offset.y;
	drawlist->AddLine(seg_start, seg_end, IM_COL32(255, 255, 255, 255), 1.);
}

ImVec2 MakeBox(ImColor color, ImVec2 offset, float width, float height) {


	// Get the current ImGui cursor position
	// ImVec2 p = ImGui::GetCursorScreenPos();
	ImVec2 p = offset;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw a rectangle with color
	draw_list->AddRectFilled(
		p, ImVec2(p.x + width, p.y + height),
		(ImU32)color);



	return ImVec2(p.x + width, p.y + height);
}


ImVec2 MakeBall(ImColor color, float radius_proportion, ImVec2 offset, float width, float height) {

  radius_proportion = MAX(MIN(radius_proportion, 1.), 0.);
  
  ImVec2 center = ImVec2(offset.x + width * 0.5, offset.y + height * 0.5);

  float radius = radius_proportion * MIN(width, height) * 0.5;

  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  // Draw a rectangle with color
  draw_list->AddCircleFilled(
      center, radius,
      (ImU32)color);

  // Advance the ImGui cursor to claim space in the window (otherwise the window
  // will appear small and needs to be resized)

  return ImVec2(offset.x + width, offset.y + height);
}

bool FrameHistoryWindow::hasWorldTimeMoved() {
	bool res = *g_gameVals.pFrameCount > last_frame;
	last_frame = *g_gameVals.pFrameCount;
	return res;
}


void FrameHistoryWindow::Update()
{
	if (!m_windowOpen || !isFrameHistoryEnabledInCurrentState())
	{
		history.clear();
		return;
	}

	// May want to come up with our own function to check if time moved.
	// The current implementation doesn't check if we missed frames.
	if (!g_interfaces.player1.IsCharDataNullPtr() &&
		!g_interfaces.player2.IsCharDataNullPtr() && hasWorldTimeMoved()) {

		history.updateHistory(resetting);
	}

	BeforeDraw();

	// TODO: Try using beginchild instead. If this continues causing problems with other windows.
	ImGui::Begin("##FrameHistory", nullptr, m_windowFlags);

	Draw();

	ImGui::End();

	AfterDraw();
}

// Use this to push styles and such
void FrameHistoryWindow::BeforeDraw() {}
// pop styles, clean up drawing state
void FrameHistoryWindow::AfterDraw() {}

void FrameHistoryWindow::Draw() {
		// borrow the history queue
		StatePairQueue& queue = history.read();
		int frame_idx = 0;

		ImGui::Text("Player 1:");
		// Rows starting point. Be careful where you place this
		ImVec2 cursor_p = ImGui::GetCursorScreenPos();

		float text_vertical_spacing = 20.;
		const int rows = 2 * 2;





		// Reclaim space after player 1 rows so Player 2 appears below
		ImGui::Dummy(ImVec2(0, (height + spacing) * ((rows >> 1) - 1) + height));
		ImGui::Text("Player 2:");
		for (StatePairQueue::reverse_iterator elem = queue.rbegin(); elem != queue.rend(); ++elem) {
			PlayerFrameState p1state = elem->front();
			PlayerFrameState p2state = elem->back();

			// determine colors
			// Need to make it more rubust later, format of the colors:
			// col_arr[0xb]: [p1_r1_r, p1_r1_g, p1_r1_b, ?, ?, ?, p2_r1_r, p2_r1_g, p2_r1_b, ?, ?, ? ]
			std::array<float, 3 * rows> col_arr;


			std::array<float, 3> color;
			color = kindtoColor(p1state.kind);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr));


			color = kindtoColor(p2state.kind);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 6 * 1);




			ImVec2 prev_cursor_p = cursor_p;

			float next_x;

			// NOTE: Add a -3 to the indices beyond this point (since the loop below expects to pass 6 indices for each i (iteration)
			// But, these outside calls only increment by 3

			// draw a box, mind how much it has moved beyond the (width, height)
			// Draw box & write the new cursor pos
			cursor_p = MakeBox(ImColor(col_arr[0], col_arr[1], col_arr[2]), cursor_p, width, height);

			// Remember where the box finished drawing, to get the new column
			next_x = cursor_p.x + spacing;

			for (int i = 1; i < (rows >> 1); ++i) {
				// carriage return 
				cursor_p = ImVec2(prev_cursor_p.x, cursor_p.y + spacing);

				auto cursor_p2 = cursor_p;

				cursor_p = MakeBox(ImColor(0, 0, 0), cursor_p, width, height);

				
				// TODO: Could do with a more concise implementation
				auto cursor_tmp = cursor_p2;
				if (bool(p1state.invul & Attribute::T)) {
					cursor_tmp.x = cursor_p2.x + width - width / 5;
					MakeBox(ImColor(255, 255, 255), cursor_tmp, width / 5, height);
				};
				if (bool(p1state.invul & Attribute::P)) {
					cursor_tmp = cursor_p2;
					cursor_tmp.x = cursor_p2.x;
					MakeBox(ImColor(255, 255, 255), cursor_tmp, width / 5, height);
				};
				if (bool(p1state.invul & Attribute::H)) {
					// the lines have 1/8th of the element height
					cursor_tmp = cursor_p2;
					cursor_tmp.y = cursor_p2.y;
					MakeBox(ImColor(255, 255, 255), cursor_tmp, width, height / 5);
				};
				if (bool(p1state.invul & Attribute::B)) {
					cursor_tmp = cursor_p2;
					cursor_tmp.y = cursor_p2.y + height / 2 - ((height / 5) / 2);
					MakeBox(ImColor(255, 255, 255), cursor_tmp, width, height / 5);
				}
				if (bool(p1state.invul & Attribute::F)) {
					cursor_tmp = cursor_p2;
					cursor_tmp.y = cursor_p2.y + height - (height / 5);
					MakeBox(ImColor(255, 255, 255), cursor_tmp, width, height / 5);

				}


			}
			cursor_p.x = prev_cursor_p.x;
			cursor_p.y += text_vertical_spacing + spacing;

			// add another -3 to the indices beyond this point
			cursor_p = MakeBox(ImColor(
				col_arr[(rows >> 1) * 3 + 0], 
				col_arr[(rows >> 1) * 3 + 1], 
				col_arr[(rows >> 1) * 3 + 2])
				, cursor_p, width, height);
			
			for (int i = (rows >> 1) + 1; i < rows; ++i) {

				// carriage return 
				cursor_p = ImVec2(prev_cursor_p.x, cursor_p.y + spacing);

				auto cursor_p2 = cursor_p;
				
				cursor_p = MakeBox(ImColor(0, 0, 0), cursor_p, width, height);
				

				auto cursor_tmp = cursor_p2;
				if (bool(p2state.invul & Attribute::T)) {
					cursor_tmp.x = cursor_p2.x + width - width / 5;
					MakeBox(ImColor(255, 255, 255), cursor_tmp, width / 5, height);
				};
				if (bool(p2state.invul & Attribute::P)) {
					cursor_tmp = cursor_p2;
					cursor_tmp.x = cursor_p2.x;
					MakeBox(ImColor(255, 255, 255), cursor_tmp, width / 5, height);
				};
				if (bool(p2state.invul & Attribute::H)) {
					cursor_tmp = cursor_p2;
					cursor_tmp.y = cursor_p2.y;
					MakeBox(ImColor(255, 255, 255), cursor_tmp, width, height / 5);
				};
				if (bool(p2state.invul & Attribute::B)) {
					cursor_tmp = cursor_p2;
					cursor_tmp.y = cursor_p2.y + height / 2 - ((height / 5) / 2);
					MakeBox(ImColor(250, 255, 255), cursor_tmp, width, height / 5);
				}
				if (bool(p2state.invul & Attribute::F)) {
					cursor_tmp = cursor_p2;
					cursor_tmp.y = cursor_p2.y + height - (height / 5);
					MakeBox(ImColor(250, 255, 255), cursor_tmp, width, height / 5);

				}

			}
			cursor_p.x = next_x;
			cursor_p.y = prev_cursor_p.y;

			ImVec2 line_start = ImVec2(0., height * -0.25);
			ImVec2 line_end = ImVec2(0., ((rows - 1) >> 1) * (height + spacing) + height * 1.25);
			float length_y = line_end.y - line_start.y;

			ImVec2 midpoint_p1 = ImVec2((prev_cursor_p.x + cursor_p.x + width) * 0.5, cursor_p.y);
			ImVec2 midpoint_p2 = ImVec2(midpoint_p1.x, midpoint_p1.y + length_y + text_vertical_spacing + spacing - height * 0.25);


			// Draw markings, use a different thickness based on i
			if ((frame_idx + 1) % 4 == 0 && (frame_idx + 1) != 0) {
				float emptiness = 0.5;
				int dashes = 10;
				if ((frame_idx + 1) % 16 == 0) {
					dashes = 1;
					emptiness = 0.;
				}

				ImDrawList* draw_list = ImGui::GetWindowDrawList();


				// halfway between the boxes
				std::vector<std::array<ImVec2, 2>> dotted_line = DottedLine(
					line_start,
					// take off the last bit of spacing (notice we add height at the end)
					line_end,
					dashes,
					emptiness);
				for (auto line : dotted_line) {
					DrawSegWithOffset(draw_list, midpoint_p1, line[0], line[1]);
					DrawSegWithOffset(draw_list, midpoint_p2, line[0], line[1]);
				}

			}


			++frame_idx;
		}	
}
