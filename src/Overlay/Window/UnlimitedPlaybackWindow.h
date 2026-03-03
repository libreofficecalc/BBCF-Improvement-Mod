#pragma once

#include "IWindow.h"

class UnlimitedPlaybackWindow : public IWindow {
public:
    UnlimitedPlaybackWindow(const std::string& windowTitle, bool windowClosable,
        ImGuiWindowFlags windowFlags = 0)
        : IWindow(windowTitle, windowClosable, windowFlags) {}

    ~UnlimitedPlaybackWindow() override = default;

protected:
    void BeforeDraw() override;
    void Draw() override;
};

