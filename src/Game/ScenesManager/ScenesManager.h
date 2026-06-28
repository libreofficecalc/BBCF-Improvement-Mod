#pragma once
#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
class ScenesManager {
public:
    static void GoToMainMenu();
    static void GoToNextScene();
    static void PlayLoadedReplay();
};