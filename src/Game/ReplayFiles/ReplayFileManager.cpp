#include "ReplayFileManager.h"
#include "Core/utils.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <regex>
#include <experimental/filesystem>
#include "Game/characters.h"


#define REPLAY_FILE_SIZE 65536
#define REPLAY_FOLDER_PATH "./Save/Replay/"
#define REPLAY_ARCHIVE_FOLDER_PATH "./Save/Replay/archive/"
    



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
            
            std::regex r("\.+(replay\\d\\d.dat)");
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
            auto new_fname = build_file_name();
            save_replay(replay_archive_folder_path + new_fname);
        }

        }   
