
#pragma once
#include <stdint.h>
#include "ReplayFile.h"
#include <vector>
#include <string>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define REPLAY_FILE_SIZE 65536
#define REPLAY_FOLDER_PATH "./Save/Replay/"
#define REPLAY_ARCHIVE_FOLDER_PATH "./Save/Replay/archive/"
class ReplayFileManager {
public:
	ReplayFile replay_file;
	bool isLoaded = false;
	ReplayFileManager();
	ReplayFileManager(std::string file_path);
	bool save_replay(std::string file_path); // return 1 means save sucessful, return -1 means error
	bool load_replay(std::string file_path); // return 1 means load sucessful, return -1 means error
	bool check_file_validity(ReplayFile* file);// return true if valid, false if invalid.
	std::string build_file_name();
	void archive_replays();

	bool template_modified = false;
	
	void bbcf_sort_replay_list();
	void load_replay_list_default();
	void load_replay_list_default_repair();
	void load_replay_list_from_archive(int page);
	void load_replay_list_from_db(int page, int character1 = -1, std::string player1 = "", int character2 = -1, std::string player2 = "");
};

extern ReplayFileManager g_rep_manager;