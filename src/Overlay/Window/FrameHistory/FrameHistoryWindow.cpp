#include "FrameHistoryWindow.h"
#include <vector>
#include "Game/gamestates.h"

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


void FrameHistoryWindow::Update()
{
	if (!m_windowOpen || !isFrameHistoryENabledInCurrentState())
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
	ImGui::Begin("##FrameHistory", nullptr, m_overlayWindowFlags);

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
		const int rows = 3 * 2;
		// The number of rows on different elevations
		const int unmerged_rows = rows - 2;
		const double circle_proportion = 0.75;




		// Reclaim space after player 1 rows so Player 2 appears below
		ImGui::Dummy(ImVec2(0, (height + spacing) * ((unmerged_rows >> 1) - 1) + height));
		ImGui::Text("Player 2:");
		for (StatePairQueue::reverse_iterator elem = queue.rbegin(); elem != queue.rend(); ++elem) {
			PlayerFrameState p1state = elem->front();
			PlayerFrameState p2state = elem->back();

			// determine colors
			std::array<float, 3 * rows> col_arr;

			// TODO: Change design to a box for tpo and an inscribed circle for hbf
			// One row for each set of attributes
			std::array<Attribute, 3> hbf = { Attribute::H, Attribute::B, Attribute::F };
			std::array<Attribute, 3> tpo = { Attribute::T, Attribute::P, Attribute::O };

			std::array<float, 3> color;
			color = kindtoColor(p1state.kind);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr));
			color = attributetoColor(p1state.invul, hbf);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 1);
			color = attributetoColor(p1state.invul, tpo);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 2);
			//color = attributetoColor(p1state.guardp);
			//std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 2);
			//color = attributetoColor(p1state.attack);
			//std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 3);

			color = kindtoColor(p2state.kind);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 3);
			color = attributetoColor(p2state.invul, hbf);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 4);
			color = attributetoColor(p2state.invul, tpo);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 5);



			ImVec2 prev_cursor_p = cursor_p;

			float next_x;

			// NOTE: Add a -3 to the indices beyond this point (since the loop below expects to surpass 6 indices for each i (iteration)
			// But, these outside calls only increment by 3

			// draw a box, mind how much it has moved beyond the (width, height)
			// Draw box & write the new cursor pos
			cursor_p = MakeBox(ImColor(col_arr[0], col_arr[1], col_arr[2]), cursor_p, width, height);

			// Remember where the box finished drawing, to get the new column
			next_x = cursor_p.x + spacing;

			for (int i = 1; i < (unmerged_rows >> 1); ++i) {
				// carriage return 
				cursor_p = ImVec2(prev_cursor_p.x, cursor_p.y + spacing);
				MakeBox(ImColor(col_arr[i * 6 + 0 - 3], col_arr[i * 6 + 1 - 3], col_arr[i * 6 + 2 - 3]), cursor_p, width, height);
				// TODO: Skip a row
				cursor_p = MakeBall(ImColor(col_arr[i * 6 + 0], col_arr[i * 6 + 1], col_arr[i * 6 + 2]), circle_proportion, cursor_p, width, height);
			}
			cursor_p.x = prev_cursor_p.x;
			cursor_p.y += text_vertical_spacing + spacing;

			// add another -3 to the indices beyond this point
			cursor_p = MakeBox(ImColor(
				col_arr[(unmerged_rows >> 1) * 6 + 0 - 3], 
				col_arr[(unmerged_rows >> 1) * 6 + 1 - 3], 
				col_arr[(unmerged_rows >> 1) * 6 + 2 - 3])
				, cursor_p, width, height);
			
			for (int i = (unmerged_rows >> 1) + 1; i < unmerged_rows; ++i) {
				// carriage return 
				cursor_p = ImVec2(prev_cursor_p.x, cursor_p.y + spacing);
				MakeBox(ImColor(col_arr[i * 6 + 0 - 6], col_arr[i * 6 + 1 - 6], col_arr[i * 6 + 2 - 6]), cursor_p, width, height);
				cursor_p = MakeBall(ImColor(col_arr[i * 6 + 0 - 3], col_arr[i * 6 + 1 - 3], col_arr[i * 6 + 2 - 3]), circle_proportion, cursor_p, width, height);
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
