
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
	std::string build_file_name();
	void archive_replays();
};