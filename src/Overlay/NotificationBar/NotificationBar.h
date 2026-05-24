#pragma once

#include "Overlay/Window/DebugWindow.h"

#include <functional>
#include <queue>
#include <string>

class NotificationBar;

extern NotificationBar* g_notificationBar;

class NotificationBar
{
public:
        void DrawNotifications();
        void AddNotification(const char* text, ...);
        void AddLocalizedNotification(std::function<std::string()> formatter);
        void ClearNotifications();

private:
        void CalculateTextStartingOffset(const std::string& text);

        struct NotificationEntry
        {
                std::function<std::string()> formatter;
                std::string text;
        };

        std::queue<NotificationEntry> m_notifications;

	bool m_isTextOffsetInitialized = false;
	float m_textOffset = 0;
	float m_textWidth = 0;
	float m_screenWidth = 0;
	float m_secondsToSlide = 7;

	// For debug purposes
	friend class DebugWindow;
};
