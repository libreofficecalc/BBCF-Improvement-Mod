#pragma once
#include "IWindow.h"
#include "Overlay/WindowContainer/WindowContainer.h"

class WinePopupWindow : public IWindow
{
public:
    void Update() override;
    WinePopupWindow(const std::string& windowTitle, bool windowClosable,
        WindowContainer& windowContainer, ImGuiWindowFlags windowFlags = 0)
        : IWindow(windowTitle, windowClosable, windowFlags), m_pWindowContainer(&windowContainer) {}
    ~WinePopupWindow() override = default;

protected:
    void Draw() override;
    WindowContainer* m_pWindowContainer = nullptr;
};

