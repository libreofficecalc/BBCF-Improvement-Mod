#pragma once

#include "IWindow.h"
class WindowContainer;

class UnlimitedReplayTakeoverWindow : public IWindow {
public:
    UnlimitedReplayTakeoverWindow(const std::string& windowTitle, bool windowClosable,
        WindowContainer& windowContainer, ImGuiWindowFlags windowFlags = 0)
        : IWindow(windowTitle, windowClosable, windowFlags), m_pWindowContainer(&windowContainer) {}

    ~UnlimitedReplayTakeoverWindow() override = default;

protected:
    void BeforeDraw() override;
    void Draw() override;

private:
    WindowContainer* m_pWindowContainer = nullptr;
};
