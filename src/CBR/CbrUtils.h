#pragma once
#include <string>
#include <chrono>
#include <CBR\CbrData.h>

inline std::string NowToString()
{
	std::chrono::system_clock::time_point p = std::chrono::system_clock::now();
	time_t t = std::chrono::system_clock::to_time_t(p);
	char str[26];
	ctime_s(str, sizeof str, &t);
	return str;
} 

inline float RandomFloat(float a, float b) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

inline std::string getCharacterNameFromID(int id) {
	if (id == 33) { return "es"; };//es
	if (id == 11) { return "ny"; };//nu
	if (id == 34) { return "ma"; };// mai
	if (id == 12) { return "tb"; };// tsubaki
	if (id == 0) { return "rg"; };//ragna
	if (id == 13) { return "hz"; };//hazama
	if (id == 1) { return "jn"; };//jin
	if (id == 14) { return "mu"; };//mu
	if (id == 2) { return "no"; };//noel
	if (id == 15) { return "mk"; };//makoto
	if (id == 3) { return "rc"; };//rachel
	if (id == 16) { return "vh"; };//valk
	if (id == 4) { return "tk"; };//taokaka
	if (id == 17) { return "pt"; };//platinum
	if (id == 5) { return "tg"; };//tager
	if (id == 18) { return "rl"; };//relius
	if (id == 6) { return "lc"; };//litchi
	if (id == 19) { return "iz"; };//izayoi
	if (id == 7) { return "ar"; };//arakune
	if (id == 20) { return "am"; };//amane
	if (id == 8) { return "bn"; };//bang
	if (id == 21) { return "bl"; };//bullet
	if (id == 9) { return "ca"; };//carl
	if (id == 22) { return "az"; };//azrael
	if (id == 10) { return "ha"; };//hakumen
	if (id == 23) { return "kg"; };//kagura
	if (id == 24) { return "kk"; };//koko
	if (id == 27) { return "rm"; };//lambda
	if (id == 28) { return "hb"; };//hibiki
	if (id == 25) { return "tm"; };//terumi
	if (id == 29) { return "ph"; };//nine
	if (id == 26) { return "ce"; };//Celica
	if (id == 30) { return "nt"; };//naoto
	if (id == 31) { return "mi"; };//izanami
	if (id == 32) { return "su"; };//susan
	if (id == 35) { return "jb"; };//jubei
	return "";
}

inline std::string getOpponentCharNameString(CbrData& cbr) {
	std::map<std::string, int> charReplays;
	for (int i = 0; i < cbr.getReplayCount(); i++) {
		auto rf = cbr.getReplayFiles();
		auto& name = rf->at(i).getCharNames()[1];
		auto ok = charReplays.find(name);
		if (ok != charReplays.end()) {
			charReplays[name] += 1;
		}
		else {
			charReplays[name] = 1;
		}
	}
	std::string opponentChar = "";
	int i = 1;
	for (auto const& x : charReplays)
	{
		opponentChar += x.first + "(" + std::to_string(x.second) + "), ";  // string (key)
		if (i % 3 == 0) { opponentChar += "\n"; }
		i++;
	}
	return opponentChar;
}
