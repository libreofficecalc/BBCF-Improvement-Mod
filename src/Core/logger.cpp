#include "logger.h"

#include <ctime>
#include <cstdarg>
#include <sstream>

namespace
{
        FILE* g_oFile = nullptr;
        bool g_isLoggingEnabled = false;
}

bool IsLoggingEnabled()
{
        return g_isLoggingEnabled && g_oFile;
}

bool hookSucceeded(PBYTE addr, const char* funcName)
{
        if (!addr)
        {
                LOG(2, "FAILED to hook %s\n", funcName);
                return false;
        }

        LOG(2, "Successfully hooked %s at 0x%p\n", funcName, addr);
        return true;
}

char* getFullDate()
{
        time_t timer;
        char* buffer = (char*)malloc(sizeof(char) * 26);
        if (!buffer)
        {
                return NULL;
        }

        struct tm* tm_info;

        time(&timer);
        tm_info = localtime(&timer);

        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        return buffer;
}

void logger(const char* message, ...)
{
        if (!message || !g_oFile) { return; }

        va_list args;
        va_start(args, message);
        vfprintf(g_oFile, message, args);
        va_end(args);

        fflush(g_oFile);
}

void openLogger()
{
        if (g_oFile)
        {
                return;
        }

        g_oFile = fopen("DEBUG.txt", "w");
        if (!g_oFile)
        {
                g_isLoggingEnabled = false;
                return;
        }

        char* time = getFullDate();

        fprintf(g_oFile, "\n\n\n\n");

        if (time)
        {
                fprintf(g_oFile, "BBCF_FIX START - %s\n", time);
                free(time);
        }
        else
        {
                fprintf(g_oFile, "BBCF_FIX START - {Couldn't get the current time}\n");
        }

        fprintf(g_oFile, "/////////////////////////////////////\n");
        fprintf(g_oFile, "/////////////////////////////////////\n\n");
        fflush(g_oFile);
}

void closeLogger()
{
        if (!g_oFile)
        {
                g_isLoggingEnabled = false;
                return;
        }

        char* time = getFullDate();
        if (time)
        {
                fprintf(g_oFile, "BBCF_FIX STOP - %s\n", time);
                free(time);
        }
        else
        {
                fprintf(g_oFile, "BBCF_FIX STOP - {Couldn't get the current time}\n");
        }

        fclose(g_oFile);
        g_oFile = nullptr;
        g_isLoggingEnabled = false;
}

void SetLoggingEnabled(bool enabled)
{
        if (enabled)
        {
                openLogger();
                g_isLoggingEnabled = g_oFile != nullptr;
        }
        else
        {
                closeLogger();
        }
}

void logSettingsIni()
{
        LOG(1, "settings.ini config:\n");

        std::ostringstream oss;

        //X-Macro
#define SETTING(_type, _var, _inistring, _defaultval) \
        oss << "\t- " << _inistring << " = " << Settings::settingsIni.##_var << "\n";
#include "settings.def"
#undef SETTING

        LOG(1, oss.str().c_str());
}

void logD3DPParams(D3DPRESENT_PARAMETERS* pPresentationParameters, bool isOriginalSettings)
{
        if (isOriginalSettings)
        {
                LOG(1, "Original D3D PresentationParameters:\n");
        }
        else
        {
                LOG(1, "Modified D3D PresentationParameters:\n");
        }

        LOG(1, "\t- BackBufferWidth: %u\n", pPresentationParameters->BackBufferWidth);
        LOG(1, "\t- BackBufferHeight: %u\n", pPresentationParameters->BackBufferHeight);
        LOG(1, "\t- BackBufferFormat: %u\n", pPresentationParameters->BackBufferFormat);
        LOG(1, "\t- BackBufferCount: %u\n", pPresentationParameters->BackBufferCount);
        LOG(1, "\t- SwapEffect: %u\n", pPresentationParameters->SwapEffect);
        LOG(1, "\t- MultiSampleType: %u\n", pPresentationParameters->MultiSampleType);
        LOG(1, "\t- MultiSampleQuality: %d\n", pPresentationParameters->MultiSampleQuality);
        LOG(1, "\t- EnableAutoDepthStencil: %d\n", pPresentationParameters->EnableAutoDepthStencil);
        LOG(1, "\t- FullScreen_RefreshRateInHz: %u\n", pPresentationParameters->FullScreen_RefreshRateInHz);
        LOG(1, "\t- hDeviceWindow: 0x%p\n", pPresentationParameters->hDeviceWindow);
        LOG(1, "\t- Windowed: %d\n", pPresentationParameters->Windowed);
        LOG(1, "\t- Flags: 0x%p\n", pPresentationParameters->Flags);
        LOG(1, "\t- PresentationInterval: 0x%p\n", pPresentationParameters->PresentationInterval);
}
