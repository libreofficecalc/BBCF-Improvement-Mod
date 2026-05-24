#include "IWindow.h"
#include "Core/logger.h"

void IWindow::Update()
{
	if (!m_windowOpen)
	{
		return;
	}
	if (m_windowTitle.find("Unlimited Replay Takeover") != std::string::npos) {
		LOG(1, "[URT][WIN] Update begin title='%s' open=%d closable=%d flags=0x%X\n",
			m_windowTitle.c_str(),
			m_windowOpen ? 1 : 0,
			m_windowClosable ? 1 : 0,
			static_cast<unsigned int>(m_windowFlags));
	}

	BeforeDraw();

	ImGui::Begin(m_windowTitle.c_str(), GetWindowOpenPointer(), m_windowFlags);
	Draw();
	ImGui::End();

	AfterDraw();
}

void IWindow::Open()
{
	m_windowOpen = true;
	if (m_windowTitle.find("Unlimited Replay Takeover") != std::string::npos) {
		LOG(1, "[URT][WIN] Open title='%s'\n", m_windowTitle.c_str());
	}
}

void IWindow::Close()
{
	m_windowOpen = false;
	if (m_windowTitle.find("Unlimited Replay Takeover") != std::string::npos) {
		LOG(1, "[URT][WIN] Close title='%s'\n", m_windowTitle.c_str());
	}
}

void IWindow::ToggleOpen()
{
	m_windowOpen = !m_windowOpen;
	if (m_windowTitle.find("Unlimited Replay Takeover") != std::string::npos) {
		LOG(1, "[URT][WIN] ToggleOpen title='%s' newOpen=%d\n", m_windowTitle.c_str(), m_windowOpen ? 1 : 0);
	}
}

bool IWindow::IsOpen() const
{
	return m_windowOpen;
}

void IWindow::SetWindowFlag(ImGuiWindowFlags flag)
{
	m_windowFlags |= flag;
}

void IWindow::ClearWindowFlag(ImGuiWindowFlags flag)
{
	m_windowFlags &= ~flag;
}

bool * IWindow::GetWindowOpenPointer()
{
	if (m_windowClosable)
	{
		return &m_windowOpen;
	}

	return nullptr;
}
