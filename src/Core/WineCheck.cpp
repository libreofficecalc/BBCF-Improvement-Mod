#include "WineCheck.h"

#include "logger.h"
#include "RuntimePlatform.h"

bool WineCheck()
{
    bool WineLikely = IsWineOrProton();

    if (WineLikely)
    {
        LOG(1, "Wine/Proton is most likely being used.\n");
    }
    else
    {
        LOG(1, "Wine Has Not Been Detected.\n");
    }

    return WineLikely;
}

