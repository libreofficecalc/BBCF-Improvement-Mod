#include "LogWindow.h"

#include "Core/Localization.h"

void LogWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
}

void LogWindow::Draw()
{
	if (ImGui::Button(Messages.Clear()))
	{
		m_logger.Clear();
	}
	ImGui::SameLine();
	bool copyPressed = ImGui::Button(Messages.Copy_to_clipboard());
	ImGui::SameLine();
	m_filter.Draw(Messages.Search(), -100.0f);
	ImGui::Separator();
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (copyPressed)
	{
		ImGui::LogToClipboard();
		m_logger.Log(Messages.System_log_copied());
	}

	ImGuiLogBuffer* logBuffer = (ImGuiLogBuffer*)m_logger.GetBuffer();
	if (m_filter.IsActive())
	{
		const char* bufBegin = logBuffer->textBuffer.begin();
		const char* line = bufBegin;
		for (int line_no = 0; line != NULL; line_no++)
		{
			const char* lineEnd = (line_no < logBuffer->lineOffsets.Size) ? bufBegin + logBuffer->lineOffsets[line_no] : NULL;
			if (m_filter.PassFilter(line, lineEnd))
			{
				ImGui::TextUnformatted(line, lineEnd);
			}
			line = lineEnd && lineEnd[1] ? lineEnd + 1 : NULL;
		}
	}
	else
	{
		ImGui::TextUnformatted(logBuffer->textBuffer.begin());
	}

	// Handle automatic scrolling
	if (m_prevScrollMaxY < ImGui::GetScrollMaxY())
	{
		// Scroll down automatically only if we didnt scroll up or we closed the window
		if (m_prevScrollMaxY - 5 <= ImGui::GetScrollY())
		{
			ImGui::SetScrollY(ImGui::GetScrollMaxY());
		}
		m_prevScrollMaxY = ImGui::GetScrollMaxY();
	}

	ImGui::EndChild();
}
