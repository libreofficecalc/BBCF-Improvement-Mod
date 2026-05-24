#pragma once

#include "IWindow.h"
class WindowContainer;

class UnlimitedPlaybackWindow : public IWindow {
public:
    UnlimitedPlaybackWindow(const std::string& windowTitle, bool windowClosable,
        WindowContainer& windowContainer, ImGuiWindowFlags windowFlags = 0)
        : IWindow(windowTitle, windowClosable, windowFlags), m_pWindowContainer(&windowContainer) {}

    ~UnlimitedPlaybackWindow() override = default;

protected:
    void BeforeDraw() override;
    void Draw() override;

private:
    WindowContainer* m_pWindowContainer = nullptr;
};
