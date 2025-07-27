#include "ReplayFileManager.h"
#include "Core/utils.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <regex>
#include <experimental/filesystem>
#include "Game/characters.h"
#include <Core/interfaces.h>

#include <wininet.h>
#include <atlstr.h>
#include <Web/url_downloader.h>
#include "ReplayList.h"

//#define REPLAY_FILE_SIZE 65536
//#define REPLAY_FOLDER_PATH "./Save/Replay/"
//#define REPLAY_ARCHIVE_FOLDER_PATH "./Save/Replay/archive/"
    



void replace_all(
    std::string& s,
    std::string const& toReplace,
    std::string const& replaceWith
) {
    std::string buf;
    std::size_t pos = 0;
    std::size_t prevPos;

    // Reserves rough estimate of final size of string.
    buf.reserve(s.size());

    while (true) {
        prevPos = pos;
        pos = s.find(toReplace, pos);
        if (pos == std::string::npos)
            break;
        buf.append(s, prevPos, pos - prevPos);
        buf += replaceWith;
        pos += toReplace.size();
    }

    buf.append(s, prevPos, s.size() - prevPos);
    s.swap(buf);
}
    ReplayFileManager::ReplayFileManager() {
        isLoaded = false;
    }
    ReplayFileManager::ReplayFileManager(std::string file_path) {
        isLoaded = load_replay(file_path);
    }

    bool ReplayFileManager::save_replay(std::string file_path) {
        //std::wstring file_path2 = utf8_to_utf16(file_path);
        if (isLoaded) {
                std::ofstream out(file_path,std::ios::binary);
                if (out.is_open()) {
                    out.write((char *)&replay_file, REPLAY_FILE_SIZE);
                    out.close();
                    return true;
                }
                return false;
               
        }
        return false;
    

    }
	bool ReplayFileManager::load_replay(std::string file_path) {
            std::string fpath = REPLAY_FOLDER_PATH;
            fpath += file_path;
            if (fpath.find(".dat") == std::string::npos) {
                fpath += ".dat";
            }
   
            std::streampos fileSize;
            std::ifstream file(fpath, std::ios::binary);
            if (file.good()) {
                file.seekg(0, std::ios::end);
                fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                file.read((char*)&replay_file, fileSize);
                isLoaded = true;
                return true;
            }

            else {
                isLoaded = false;
                return false;
            }


        } 

	std::string ReplayFileManager::build_file_name() {
        std::wstring p1ws = std::wstring(replay_file.p1_name);
        std::wstring p2ws = std::wstring(replay_file.p2_name);
        std::string p1n = utf16_to_utf8(p1ws);
        std::string p2n = utf16_to_utf8(p2ws);
        replace_all(p1n, ".", "");
        replace_all(p2n, ".", "");
        p1n = p1n.substr(0, 5);
        p2n = p2n.substr(0, 5);
        std::string dt = replay_file.date1;
        std::string month = dt.substr(4, 3);
        std::string day = dt.substr(8, 2);
        std::string time = dt.substr(11, 5);
        std::string year = dt.substr(22, 2);
        std::string p1_toonstr = getCharacterNameByIndexA(replay_file.p1_toon);
        std::string p2_toonstr = getCharacterNameByIndexA(replay_file.p2_toon);
        replace_all(month, "Jan", "01");
        replace_all(month, "Feb", "02");
        replace_all(month, "Mar", "03");
        replace_all(month, "Apr", "04");
        replace_all(month, "May", "05");
        replace_all(month, "Jun", "06");
        replace_all(month, "Jul", "07");
        replace_all(month, "Aug", "08");
        replace_all(month, "Sep", "09");
        replace_all(month, "Oct", "10");
        replace_all(month, "Nov", "11");
        replace_all(month, "Dec", "12");
        //auto fin = year + "_" + month+ "_"+ day+"T"+time + "_" + "("+ p1_toonstr+ ")"+  p1n + "_V_" + "(" + p2_toonstr + ")"+ p2n + ".dat";
        auto fin = year + month + day + "T" + time + "_"  + p1n + "_V_" + p2n + ".dat";
        
        replace_all(fin, ":", "_");
        replace_all(fin, " ", "_");



        return fin;
    }
	void ReplayFileManager::archive_replays() {
        std::vector<std::string> replay_paths;
        for (const auto& entry : std::experimental::filesystem::directory_iterator(REPLAY_FOLDER_PATH)) {
            std::string path = std::string(entry.path().string());
            
            std::regex r(".+(replay\\d\\d.dat)");
            std::smatch m;
            std::regex_search(path, m, r);
            if (m[1].str() != "") {
                replay_paths.push_back(m[1].str());
                auto tst = m[1].str();
            }
         
        }
        std::string replay_archive_folder_path = REPLAY_ARCHIVE_FOLDER_PATH;
        CreateDirectory(L"./Save/Replay/archive/", NULL);
        for (auto el : replay_paths) {
            load_replay(el);
            //load_replay("test.dat");
            auto new_fname = build_file_name();
            save_replay(replay_archive_folder_path + new_fname);
        }

    }   


void ReplayFileManager::bbcf_sort_replay_list() {
    // call the default sorting procedure, which sorts from newest to oldest
    // this procedure also runs after opening replay theater from main menu

    char* base = GetBbcfBaseAdress();
    char* replay_list = base + 0xAA9808;
    char* sort_func = base + 0xBDAA0;

    __asm {
        pushad

        mov ecx, replay_list
        push 0
        call sort_func

        popad
    }
}

void ReplayFileManager::load_replay_list_default() {
    // reload replay_list.dat

    char* base = GetBbcfBaseAdress();
    ReplayList* replay_list = (ReplayList*)(base + 0xAA9808);
    char* replay_file_template = base + 0x4AA66C;

    WriteToProtectedMemory((uintptr_t)replay_file_template, "replay%02d.dat", 15);
    template_modified = false;

    std::ifstream f("Save/replay_list.dat", std::ios::binary);
    f.seekg(8, std::ios_base::beg);

    int j = 0, n = 0;
    while (true) {
        f.read((char*) & replay_list->replays[j], 0x390);
        
        if (f.eof()) break;

        if (replay_list->replays[j].data()->valid >= 0) {
            //replay_list->order[j] = 99 - j;
            n += 1;
        }
        else {
            //replay_list->order[j] = 99; // push empty to the back
        }
        j += 1;
    }
    f.close();

    // if we have less than 100 replays, hide the rest
    for (; j < 100; j++) {
        replay_list->replays[j].data()->valid = 0; // at offset 8 there is 1 or 0 meaning used or not used
        //replay_list->order[j] = 99; // push empty items to the back
    }
    replay_list->count = n;

    int* view = (int*)(base + 0xE9329C);
    view[0] = 0; // reset current selected item
    view[1] = 0; // first index in visible range
    view[2] = 9; // last index in visible range
    view[3] = 0; // ???

    bbcf_sort_replay_list();
}

void ReplayFileManager::load_replay_list_default_repair() {
    // overwrite replay list with data from real replays, ignoring replay_list.dat
    // doesn't modify replay_list.dat directly, but sets a flag for bbcf to save it

    char* base = GetBbcfBaseAdress();
    ReplayList* replay_list = (ReplayList*)(base + 0xAA9808);
    char* replay_file_template = base + 0x4AA66C;

    WriteToProtectedMemory((uintptr_t)replay_file_template, "replay%02d.dat", 15);
    template_modified = false;
    
    int n = 0;
    for (int j = 0; j < 100; j++) {
        std::string name = std::to_string(j);
        name = "Save/Replay/replay" + std::string(2 - min(2, name.length()), '0') + name + ".dat";
        
        if (std::experimental::filesystem::exists(name)) {
            std::ifstream f(name, std::ios::binary);
            f.seekg(8, std::ios_base::beg);
            f.read((char*)&replay_list->replays[j], 0x390);
            f.close();

            n += 1;
            //replay_list->order[j] = n - 1 - j; // set order, most recent replay first
        }
        else {
            replay_list->replays[j].data()->valid = 0;
            //replay_list->order[j] = 99; // push empty items to the back
        }
    }
    replay_list->count = n;

    int* view = (int*)(base + 0xE9329C);
    view[0] = 0; // reset current selected item
    view[1] = 0; // first index in visible range
    view[2] = 9; // last index in visible range
    view[3] = 0; // ???

    bbcf_sort_replay_list();

    // tell bbcf to write this to file later
    *(base + 0x1304BA4) = 1;
}
bool ReplayFileManager::check_file_validity(ReplayFile* file) {
    if (file->p1_toon < 0x0 || file->p1_toon > 0x24) {
        return false;
    }
    if (file->p2_toon < 0x0 || file->p2_toon > 0x24) {
        return false;
    }
    if (file->winner_maybe < 0 || file->winner_maybe >2) {
        return false;
    }
    return true;
}
void ReplayFileManager::load_replay_list_from_archive(int page) {
    // load filenames
    std::vector<std::string> all_filenames;
    for (const auto& f : std::experimental::filesystem::directory_iterator(REPLAY_ARCHIVE_FOLDER_PATH)) {
        std::string pp = f.path().filename().string();
        all_filenames.push_back(pp);
    }
        
    std::sort(all_filenames.begin(), all_filenames.end(), std::greater<std::string>()); // descending filename order means newest to oldest

    const int page_size = 100;
    std::vector<std::string> page_filenames = std::vector<std::string>(all_filenames.begin() + max(0, min(page * page_size, all_filenames.size())),
                                                                        all_filenames.begin() + max(0, min((page + 1) * page_size, all_filenames.size())));

    // overwrite replay list
    char* base = GetBbcfBaseAdress();
    ReplayList* replay_list = (ReplayList*)(base + 0xAA9808);
    char* replay_file_template = base + 0x4AA66C;

    CreateDirectory(L"./Save/Replay/tmp/", NULL); // the 100 visible files will be copied into a new dir
    WriteToProtectedMemory((uintptr_t)replay_file_template, "tmp/rp%02d.dat", 15);
    template_modified = true;

    int n = page_filenames.size();
    int j = 0;
    int valid_replay_count = 0;
    for (; j < n; j++) {
        
  
        //std::filesystem::copy_file(std::filesystem::path(replay_dir + "/" + page_filenames[j]), std::filesystem::path(new_name)); // crashes?
        // copy file with basic C++
        std::ifstream f(REPLAY_ARCHIVE_FOLDER_PATH + page_filenames[j], std::ios::binary);


        f.seekg(8, std::ios_base::beg);
        f.read((char*)&replay_list->replays[valid_replay_count], 0x390);

        if (check_file_validity( (replay_list->replays[valid_replay_count].data()) )) {
            //only copy the file to tmp if it's valid
            std::string new_name = std::to_string(valid_replay_count);
            new_name = "Save/Replay/tmp/rp" + std::string(2 - min(2, new_name.length()), '0') + new_name + ".dat";
            f.seekg(0, std::ios_base::beg);
            std::ofstream dest(new_name, std::ios::binary);
            dest << f.rdbuf();
            dest.close();
            valid_replay_count += 1;
        }
        f.close();
        //replay_list->order[j] = n - 1 - j; // set order, most recent replay first
    }
    // if we have less than 100 replays, hide the rest
    for (; valid_replay_count < 100; valid_replay_count++) {
        replay_list->replays[valid_replay_count].data()->valid = 0;
        //replay_list->order[j] = 99; // push empty items to the back
    }
    replay_list->count = n;
        
    int* view = (int*)(base + 0xE9329C);
    view[0] = 0; // reset current selected item
    view[1] = 0; // first index in visible range
    view[2] = 9; // last index in visible range
    view[3] = 0; // ???

    bbcf_sort_replay_list();
}

std::string url_escape(std::string s) {
    replace_all(s, "%", "%25");
    replace_all(s, "?", "%3F");
    replace_all(s, "&", "%26");
    return s;
}

void ReplayFileManager::load_replay_list_from_db(int page, int character1, std::string player1, int character2, std::string player2) {

    HINTERNET hInternet = NULL, hConnect = NULL, hRequest = NULL;
    CA2W pszwide("bbreplay.ovh");
    const wchar_t* serverAddress = pszwide;
    INTERNET_PORT port = 443;

    std::string urlPath = "/api/replays?page=" + std::to_string(page+1);
    if (character1 != -1) urlPath += "&p1_character_id=" + std::to_string(character1);
    if (player1 != "") urlPath += "&p1=" + url_escape(player1);
    if (character2 != -1) urlPath += "&p2_character_id=" + std::to_string(character2);
    if (player2 != "") urlPath += "&p2=" + url_escape(player2);

    hInternet = InternetOpen(L"im", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return;
    }

    hConnect = InternetConnect(hInternet, serverAddress, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return;
    }

    hRequest = HttpOpenRequest(hConnect, L"GET", utf8_to_utf16(urlPath).data(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    DWORD numberOfBytesRead = 0;
    bool result = false;
    std::string response_body;
    do
    {
        char buffer[2000];
        result = InternetReadFile(hRequest, buffer, sizeof(buffer)-1, &numberOfBytesRead);
            
        buffer[numberOfBytesRead] = 0;
        response_body += buffer;

    } while (result && numberOfBytesRead);

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);


    std::vector<std::string> all_filenames;

    int pos = 0;
    while (true) {
        int p0 = response_body.find("\"filename\":\"", pos);
        if (p0 == -1) break;
        int p1 = response_body.find("\"", p0 + 12);
        if (p1 == -1) break;
        int p2 = p0 + 12;
        all_filenames.push_back(response_body.substr(p2, p1 - p2));
        pos = p1 + 1;
    }

    const int page_size = 100;
    std::vector<std::string> page_filenames = std::vector<std::string>(all_filenames.begin(),
        all_filenames.begin() + min(page_size, all_filenames.size()));



    // overwrite replay list
    char* base = GetBbcfBaseAdress();
    ReplayList* replay_list = (ReplayList*)(base + 0xAA9808);
    char* replay_file_template = base + 0x4AA66C;

    CreateDirectory(L"./Save/Replay/tmp/", NULL); // the 100 visible files will be copied into a new dir
    WriteToProtectedMemory((uintptr_t)replay_file_template, "tmp/rp%02d.dat", 15);
    template_modified = true;

    int n = page_filenames.size();
    int j = 0;
    for (; j < n; j++) {
        std::string new_name = std::to_string(j);
        new_name = "Save/Replay/tmp/rp" + std::string(2 - min(2, new_name.length()), '0') + new_name + ".dat";
            
        ReplayFile* rp = 0;
        DownloadUrlBinary(L"http://" + utf8_to_utf16(g_modVals.uploadReplayDataHost) + L"/uploads/" + utf8_to_utf16(page_filenames[j]), (void**)&rp);

        std::ofstream file(new_name, std::fstream::binary);
        file.write((char*)rp, sizeof(*rp));
        file.close();

        // items in replay_list.dat are initial parts of replay files in the list
        memcpy(&replay_list->replays[j], (char*)rp + 8, 0x390);

        //replay_list->order[j] = n - 1 - j; // set order, most recent replay first
    }
    // if we have less than 100 replays, hide the rest
    for (; j < 100; j++) {
        replay_list->replays[j].data()->valid = 0;
        //replay_list->order[j] = 99; // push empty items to the back
    }
    replay_list->count = n;

    int* view = (int*)(base + 0xE9329C);
    view[0] = 0; // reset current selected item
    view[1] = 0; // first index in visible range
    view[2] = 9; // last index in visible range
    view[3] = 0; // ???

    bbcf_sort_replay_list();
}

ReplayFileManager g_rep_manager;