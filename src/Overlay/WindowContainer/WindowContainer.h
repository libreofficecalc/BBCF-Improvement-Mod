#pragma once
#include "WindowType.h"
#include "Overlay/Window/IWindow.h"

#include <map>

typedef std::map<WindowType_, IWindow*> WindowMap;

class WindowContainer
{
public:
	WindowContainer();
	~WindowContainer()
	{
		for (const auto& window : m_windows)
		{
			delete window.second;
		}
	}

	template <class T>
	T* GetWindow(WindowType_ type)
	{
		const auto it = m_windows.find(type);
		return it != m_windows.end() ? static_cast<T*>(it->second) : nullptr;
	}
	IWindow* GetWindow(WindowType_ type)
	{
		const auto it = m_windows.find(type);
		return it != m_windows.end() ? it->second : nullptr;
	}
	const WindowMap& GetWindows() const { return m_windows; }
private:
	void AddWindow(WindowType_ type, IWindow* pWindow) { m_windows[type] = pWindow; }

	WindowMap m_windows;
};
