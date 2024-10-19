#include "Settings.h"
#include "logger.h"
#include "keycodes.h"

#include "Core/interfaces.h"

#include <atlstr.h>
#include <ctime>
#include <iostream>
#include <fstream>
#include "stringapiset.h"

#define VIEWPORT_DEFAULT 1

settingsIni_t Settings::settingsIni = {};
savedSettings_t Settings::savedSettings = {};


void Settings::applySettingsIni(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	if (settingsIni.viewport != VIEWPORT_DEFAULT)
	{
		pPresentationParameters->BackBufferHeight = settingsIni.renderheight;
		pPresentationParameters->BackBufferWidth = settingsIni.renderwidth;
	}

	switch (Settings::settingsIni.antialiasing)
	{
	case 0:
		pPresentationParameters->MultiSampleType = D3DMULTISAMPLE_NONE;
		pPresentationParameters->MultiSampleQuality = 0;
		break;
		//case 2:
		//	pPresentationParameters->MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
		//	break;
	case 4:
		pPresentationParameters->MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
		break;
	case 5:
	default:
		break;
	}
	g_modVals.enableForeignPalettes = Settings::settingsIni.loadforeignpalettes;
	g_modVals.save_states_save_keycode = Settings::getButtonValue(settingsIni.saveStateKeybind);
	g_modVals.save_states_load_keycode = Settings::getButtonValue(settingsIni.loadStateKeybind);
	g_modVals.replay_takeover_load_keycode = Settings::getButtonValue(settingsIni.loadReplayStateKeybind);
	g_modVals.freeze_frame_keycode = Settings::getButtonValue(Settings::settingsIni.freezeFrameKeybind);
	g_modVals.step_frames_keycode = Settings::getButtonValue(Settings::settingsIni.stepFramesKeybind);
	g_modVals.uploadReplayData = Settings::settingsIni.uploadReplayData;

	
	//CA2W pszwide (host_c_str);
	g_modVals.uploadReplayDataHost = Settings::settingsIni.uploadReplayDataHost;;
	//std::string str2 = Settings::settingsIni.uploadReplayDataEndpoint;
	//CA2W pszwide2(str2.c_str());
	g_modVals.uploadReplayDataEndpoint = Settings::settingsIni.uploadReplayDataEndpoint;
	g_modVals.uploadReplayDataPort = Settings::settingsIni.uploadReplayDataPort;
	//pPresentationParameters->Windowed = !Settings::settingsIni.fullscreen;

	pPresentationParameters->PresentationInterval = settingsIni.vsync ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	
	//pPresentationParameters->Windowed = !settingsIni.fullscreen;
	//if (settingsIni.fullscreen)
	//{
	//	pPresentationParameters->FullScreen_RefreshRateInHz = 60; // savedSettings.adapterRefreshRate;
	//}
}

int Settings::readSettingsFilePropertyInt(LPCWSTR key, LPCWSTR defaultVal, LPCWSTR filename)
{
	CString strNotificationPopups;
	GetPrivateProfileString(_T("Settings"), key, defaultVal, strNotificationPopups.GetBuffer(MAX_PATH), MAX_PATH, filename);
	strNotificationPopups.ReleaseBuffer();
	return _ttoi(strNotificationPopups);
}

float Settings::readSettingsFilePropertyFloat(LPCWSTR key, LPCWSTR defaultVal, LPCWSTR filename)
{
	CString strCustomHUDScale;
	GetPrivateProfileString(_T("Settings"), key, defaultVal, strCustomHUDScale.GetBuffer(MAX_PATH), MAX_PATH, filename);
	strCustomHUDScale.ReleaseBuffer();
	return _ttof(strCustomHUDScale);
}

std::string Settings::readSettingsFilePropertyString(LPCWSTR key, LPCWSTR defaultVal, LPCWSTR filename)
{
	CString strBuffer;
	GetPrivateProfileString(_T("Settings"), key, defaultVal, strBuffer.GetBuffer(MAX_PATH), MAX_PATH, filename);
	strBuffer.ReleaseBuffer();
	CT2CA pszConvertedAnsiString(strBuffer);
	return pszConvertedAnsiString.m_psz;
}

bool Settings::loadSettingsFile()
{
	CString strINIPath;

	_wfullpath((wchar_t*)strINIPath.GetBuffer(MAX_PATH), L"settings.ini", MAX_PATH);
	strINIPath.ReleaseBuffer();

	if (GetFileAttributes(strINIPath) == 0xFFFFFFFF)
	{
		MessageBoxA(NULL, "Settings INI File Was Not Found!", "Error", MB_OK);
		return false;
	}

	void* iniPtr = 0;

	//X-Macro
#define SETTING(_type, _var, _inistring, _defaultval) \
	iniPtr = &settingsIni.##_var; \
	if(strcmp(#_type, "bool") == 0 || strcmp(#_type, "int") == 0) { \
		*(int*)iniPtr = readSettingsFilePropertyInt(L##_inistring, L##_defaultval, strINIPath); } \
	else if(strcmp(#_type, "float") == 0) { \
		*(float*)iniPtr = readSettingsFilePropertyFloat(L##_inistring, L##_defaultval, strINIPath); } \
	else if (strcmp(#_type, "std::string") == 0) { \
		*(std::string*)iniPtr = readSettingsFilePropertyString(L##_inistring, L##_defaultval, strINIPath); }
#include "settings.def"
#undef SETTING

	// Set buttons back to default if their values are incorrect
	if (settingsIni.togglebutton.length() != 2 || settingsIni.togglebutton[0] != 'F')
		settingsIni.togglebutton = "F1";

	if (settingsIni.toggleOnlineButton.length() != 2 || settingsIni.toggleOnlineButton[0] != 'F')
		settingsIni.toggleOnlineButton = "F2";

	if (settingsIni.toggleHUDbutton.length() != 2 || settingsIni.toggleHUDbutton[0] != 'F')
		settingsIni.toggleHUDbutton = "F3";



	
	
	//modValuessettingsIni.loadStateKeybind
	
	return true;
}

void Settings::initSavedSettings()
{
	LOG(7, "initSavedSettings\n");

	switch (settingsIni.viewport)
	{
	case 2:
		LOG(7, " - case 2\n");
		savedSettings.newSourceRect.right = settingsIni.renderwidth;
		savedSettings.newSourceRect.bottom = settingsIni.renderheight;
		savedSettings.newViewport.Width = settingsIni.renderwidth;;
		savedSettings.newViewport.Height = settingsIni.renderheight;
		break;
	case 3:
		LOG(7, " - case 3\n");
		savedSettings.newSourceRect.right = 1280;
		savedSettings.newSourceRect.bottom = 768;
		savedSettings.newViewport.Width = 1280;
		savedSettings.newViewport.Height = 768;
		break;
	case 1:
	default:
		LOG(7, " - case 1, default\n");
		//in this case the value is set in Direct3DDevice9ExWrapper::CreateRenderTargetEx!
		break;
	}

	savedSettings.origViewportRes.x = 0.0;
	savedSettings.origViewportRes.y = 0.0;

	savedSettings.isDuelFieldSprite = false;

	savedSettings.isFiltering = false;
}

short Settings::getButtonValue(std::string button)
{
	auto maybe_keycode = keycode_mapper.find(button);
	if (maybe_keycode != keycode_mapper.end())
		return maybe_keycode->second;
	else
		return 112;

}
//int Settings::changeSetting(std::string setting_name, std::string new_value) { return 1; }
int Settings::changeSetting(std::string setting_name, std::string new_value) {
	//return 1;
	std::string filename = "settings.ini";
	std::string tempfilename = "temp_settings.ini";

	std::fstream inputFile(filename, std::ios::in);
	std::fstream tempFile(tempfilename, std::ios::out);

	
	if (!(inputFile.is_open() && tempFile.is_open())) {
		LOG(2, "[error] Settings::changeSetting: Unable to open the file.");
		return 1;
	}
	else{

		bool found_flag = false;
		std::string line;
		while (getline(inputFile, line)) {
			if (line.find(setting_name) == 0) {
				tempFile << setting_name << " = " << new_value << std::endl;
				found_flag = true;
			}
			else {
				tempFile << line << std::endl;
			}
		}
		if (found_flag == false) {
			//if the setting is not found, add it
			tempFile << "# " << setting_name << " added automatically #" << std::endl;
			tempFile << setting_name << " = " << new_value << std::endl;
		}
		inputFile.close();
		tempFile.close();

		if (remove(filename.c_str()) != 0) {
			//perror("Error deleting original file");
			LOG(2, "[error] Settings::changeSetting:Error deleting original file");
			return 1;
		}

		if (rename(tempfilename.c_str(), filename.c_str()) != 0) {
			//perror("Error renaming temporary file");
			LOG(2, "[error]  Settings::changeSetting: Error renaming temporary file");
			return 1;
		}

		LOG(2, "Settings::changeSetting: File updated successfully.");
		std::cout << "File updated successfully." << std::endl;
	}
	
	return 0;
}
