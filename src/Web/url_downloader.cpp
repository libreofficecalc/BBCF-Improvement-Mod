#include "url_downloader.h"

#include "Core/utils.h"
#include "Overlay/Logger/ImGuiLogger.h"

#include <wininet.h>


#include <iostream>
#include "Core/interfaces.h"
#include <atlstr.h>

#pragma comment(lib,"wininet.lib")

std::string DownloadUrl(std::wstring& wUrl)
{
	std::string url(wUrl.begin(), wUrl.end());

	HINTERNET connect = InternetOpen(L"MyBrowser", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	if (!connect)
	{
		g_imGuiLogger->Log("[error] DownloadUrl failed. Connection Failed or Syntax error with URL\n'%s'\n", url.c_str());
		return "";
	}

	HINTERNET OpenAddress = InternetOpenUrl(connect, wUrl.c_str(), NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION, 0);

	if (!OpenAddress)
	{
		DWORD ErrorNum = GetLastError();
		g_imGuiLogger->Log("[error] DownloadUrl failed. Failed to open URL\n'%s'\ncode: %d\n", url.c_str(), ErrorNum);
		InternetCloseHandle(connect);
		return "";
	}

	std::string receivedData = "";

	char DataReceived[4096];
	DWORD NumberOfBytesRead = 0;
	while (InternetReadFile(OpenAddress, DataReceived, 4096, &NumberOfBytesRead) && NumberOfBytesRead)
	{
		receivedData.append(DataReceived, NumberOfBytesRead);
	}

	InternetCloseHandle(OpenAddress);
	InternetCloseHandle(connect);

	return receivedData;
}

unsigned long DownloadUrlBinary(std::wstring& wUrl, void** outBuffer)
{
	std::string url(wUrl.begin(), wUrl.end());

	HINTERNET connect = InternetOpen(L"MyBrowser", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	if (!connect)
	{
		g_imGuiLogger->Log(
			"[error] DownloadUrlBinary failed. Connection Failed or Syntax error with URL\n'%s'\n",
			url.c_str()
		);
		return 0;
	}

	HINTERNET OpenAddress = InternetOpenUrl(connect, wUrl.c_str(), NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION, 0);

	if (!OpenAddress)
	{
		DWORD ErrorNum = GetLastError();
		g_imGuiLogger->Log(
			"[error] DownloadUrlBinary failed. Failed to open URL\n'%s'\ncode: %d\n",
			url.c_str(), ErrorNum
		);
		InternetCloseHandle(connect);
		return 0;
	}

	DWORD numberOfBytesRead = 0;
	DWORD returnedBytesRead = 0;
	bool result = false;
	do
	{
		char buffer[2000];
		result = InternetReadFile(OpenAddress, buffer, sizeof(buffer), &numberOfBytesRead);

		// Re-allocate memory
		char* tempData = new char[returnedBytesRead + numberOfBytesRead];
		memcpy(tempData, *outBuffer, returnedBytesRead);
		memcpy(tempData + returnedBytesRead, buffer, numberOfBytesRead);
		SAFE_DELETE_ARRAY(*outBuffer);
		*outBuffer = tempData;

		returnedBytesRead += numberOfBytesRead;

	} while (result && numberOfBytesRead);

	InternetCloseHandle(OpenAddress);
	InternetCloseHandle(connect);

	return returnedBytesRead;
}

//int UploadReplayBinary() { return 1; }

int UploadReplayBinary() {
    HINTERNET hInternet = NULL, hConnect = NULL, hRequest = NULL;
 //   const wchar_t* serverAddress = L"50.118.225.175"; // IP address
    CA2W pszwide (g_modVals.uploadReplayDataHost.c_str());
    const wchar_t* serverAddress = pszwide;
    //INTERNET_PORT port = 5000; // Port number
    INTERNET_PORT port = g_modVals.uploadReplayDataPort;
    //const wchar_t* urlPath = L"/upload"; // Path on the server
    CA2W pszwide2 (g_modVals.uploadReplayDataEndpoint.c_str());
    const wchar_t* urlPath = pszwide2;
    //return 0;
    // Step 1: Open Internet session
    hInternet = InternetOpen(L"im", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        // std::cerr << "Failed to open internet session: " << GetLastError() << std::endl;
        DWORD error_num = GetLastError();
        g_imGuiLogger->Log("[error] UploadReplayBinary failed. Failed to open internet session.\n\thost: '%s'\n\tendpoint: '%s'\n\tport: %d\n\terror code: %d\n", 
            g_modVals.uploadReplayDataHost.c_str(),
            g_modVals.uploadReplayDataEndpoint.c_str(), 
            g_modVals.uploadReplayDataPort,
            error_num);
        return error_num;
    }
    return 0;
   //  Step 2: Connect to the server
    hConnect = InternetConnect(hInternet, serverAddress, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        //std::cerr << "Failed to connect to the server: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternet);
        DWORD error_num = GetLastError();
        g_imGuiLogger->Log("[error] UploadReplayBinary failed. Failed to connect to the server.\n\thost: '%s'\n\tendpoint: '%s'\n\tport: %d\n\terror code: %d\n",
            g_modVals.uploadReplayDataHost.c_str(),
            g_modVals.uploadReplayDataEndpoint.c_str(),
            g_modVals.uploadReplayDataPort,
            error_num);
        return GetLastError();
    }
 
    // Step 3: Open the request
    hRequest = HttpOpenRequest(hConnect, L"POST", urlPath, NULL, NULL, NULL, 0, 0);
    if (!hRequest) {
        // std::cerr << "Failed to open HTTP request: " << GetLastError() << std::endl;
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        DWORD error_num = GetLastError();
        g_imGuiLogger->Log("[error] UploadReplayBinary failed. Failed to open HTTP request.\n\thost: '%s'\n\tendpoint: '%s'\nport: %d\n\terror code: %d\n",
            g_modVals.uploadReplayDataHost.c_str(),
            g_modVals.uploadReplayDataEndpoint.c_str(),
            g_modVals.uploadReplayDataPort,
            error_num);     
        return GetLastError();
    }

    // Step 4: Set request headers
    LPCWSTR headers = L"Content-Type: application/octet-stream\r\n";//L"Content-Type: multipart/form-data"; //L"Content-Type: application/octet-stream\r\n";
    if (!HttpAddRequestHeaders(hRequest, headers, wcslen(headers), HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD)) {
        // std::cerr << "Failed to set request headers: " << GetLastError() << std::endl;
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        DWORD error_num = GetLastError();
        g_imGuiLogger->Log("[error] UploadReplayBinary failed. Failed to set request headers.\n\thost: '%s'\n\tendpoint: '%s'\n\tport: %d\n\terror code: %d\n",
            g_modVals.uploadReplayDataHost.c_str(),
            g_modVals.uploadReplayDataEndpoint.c_str(),
            g_modVals.uploadReplayDataPort,
            error_num);
        return GetLastError();
    }



     //Get replay file data adress from memory
    int bbcf_base_adress = (int)GetBbcfBaseAdress();
    char* file_data_in_mem = (char*)(bbcf_base_adress + 0x115b478);
    DWORD fileSize = 0x10000; //all replays without exception are 64kib
    // Allocate memory for file data
    LPBYTE fileData = new BYTE[fileSize];
    // DWORD bytesRead;
    if (! *file_data_in_mem) {
        delete[] fileData;
        return 1;
    }
    memcpy(fileData, file_data_in_mem, fileSize);

    // Step 5: Send the request
    if (!HttpSendRequest(hRequest, NULL, 0, fileData, fileSize)) {
        //std::cerr << "Failed to send request: " << GetLastError() << std::endl;
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        delete[] fileData;
        DWORD error_num = GetLastError();
        g_imGuiLogger->Log("[error] UploadReplayBinary failed. Failed to send request.\n\thost: '%s'\n\tendpoint: '%s'\n\tport: %d\n\terror code: %d\n",
            g_modVals.uploadReplayDataHost.c_str(),
            g_modVals.uploadReplayDataEndpoint.c_str(),
            g_modVals.uploadReplayDataPort,
            error_num);
        return GetLastError();
    }

    

    // Step 6: Close handles
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    delete[] fileData;
    g_imGuiLogger->Log("UploadReplayBinary successful.\n\thost: '%s'\n\tendpoint: '%s'\n\tport: %d\n",
        g_modVals.uploadReplayDataHost.c_str(),
        g_modVals.uploadReplayDataEndpoint.c_str(),
        g_modVals.uploadReplayDataPort);
    return 0;
}
	