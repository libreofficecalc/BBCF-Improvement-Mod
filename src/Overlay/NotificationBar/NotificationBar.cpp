#include "NotificationBar.h"

#include "Core/Settings.h"

#include <cstdarg>
#include <cstdio>

#include <imgui.h>

NotificationBar* g_notificationBar = new NotificationBar();

void NotificationBar::DrawNotifications()
{
        if (m_notifications.empty())
                return;

        NotificationEntry& entry = m_notifications.front();

        if (entry.formatter)
        {
                std::string refreshed = entry.formatter();
                if (refreshed != entry.text)
                {
                        entry.text = std::move(refreshed);
                        m_isTextOffsetInitialized = false;
                }
        }

        if (ImGui::BeginMainMenuBar())
        {
                m_screenWidth = ImGui::GetWindowWidth();

                if (!m_isTextOffsetInitialized)
                {
                        CalculateTextStartingOffset(entry.text);
                        m_isTextOffsetInitialized = true;
                }

                ImGui::SetCursorPosX(m_textOffset);
                ImGui::TextUnformatted(entry.text.c_str());

                ImGui::EndMainMenuBar();
        }

	m_textOffset -= ImGui::GetIO().DeltaTime * (m_screenWidth / m_secondsToSlide);

	bool isTextOffscreen = m_textOffset < -m_textWidth;
	if (isTextOffscreen)
	{
                m_notifications.pop();
                m_isTextOffsetInitialized = false;
        }
}

void NotificationBar::AddNotification(const char* text, ...)
{
        if (!Settings::settingsIni.notifications)
                return;

        va_list args;
        va_start(args, text);

        const int size = std::vsnprintf(nullptr, 0, text, args) + 1;
        std::string msg(size, ' ');

        vsprintf(&msg[0], text, args);
        va_end(args);

        NotificationEntry entry;
        entry.text = std::move(msg);
        entry.formatter = [cached = entry.text]() { return cached; };

        m_notifications.push(std::move(entry));
}

void NotificationBar::AddLocalizedNotification(std::function<std::string()> formatter)
{
        if (!Settings::settingsIni.notifications || !formatter)
                return;

        NotificationEntry entry;
        entry.formatter = std::move(formatter);
        entry.text = entry.formatter();

        m_notifications.push(std::move(entry));
}

void NotificationBar::ClearNotifications()
{
	m_notifications = {};
	m_isTextOffsetInitialized = false;
}

void NotificationBar::CalculateTextStartingOffset(const std::string& text)
{
	ImGui::TextColored(ImVec4(0, 0, 0, 0), text.c_str());
	m_textWidth = ImGui::GetItemRectSize().x;
	m_textOffset = m_screenWidth;
}
