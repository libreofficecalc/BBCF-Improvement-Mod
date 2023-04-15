#pragma once

#include "json.hpp"
#include "CBR/CbrData.h"
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include "CbrUtils.h"

using json = nlohmann::json;
std::string decode64(const std::string& val);
std::string encode64(const std::string& val);
void CbrHTTPPost(json& j, boost::promise<bool>& p);
void CbrHTTPPostNoThreat(json& j);
json cbrHTTP_GetTable(boost::promise<json>& j);
std::string ConvertCbrDataToBase64(CbrData& cbr);
CbrData ConvertCbrDataFromBase64(std::string encodeS);
json cbrHTTP_GetCharData(std::string public_id, boost::promise<json>& j);
json cbrHTTP_GetFilteredTable(std::string PlayerID, std::string PlayerName, std::string PlayerCharacter, std::string OpponentCharacter, std::string ReplayCount, boost::promise<json>& j);
bool cbrHTTP_DeleteCharData(std::string public_id, boost::promise<bool>& j);
json LoadCbrMetadata(std::string PlayerName, std::string PlayerCharacter, std::string OpponentCharacter, std::string ReplayCount, boost::promise<json>& j);
json readCbrMetadata(std::string filename);
json threadDownloadData(bool run);
bool isCbrServerBusy();
json threadDownloadFilteredData(bool run, std::string PlayerID, std::string PlayerName, std::string PlayerCharacter, std::string OpponentCharacter, std::string ReplayCount);
bool isCbrServerActivityHappening();
json threadDownloadCharData(bool run, std::string public_id);
bool threadDeleteCharData(bool run, std::string public_id);
bool isCbrLocalDataUpdating();
json threadGetLocalMetadata(bool run, std::string PlayerName, std::string PlayerCharacter, std::string OpponentCharacter, std::string ReplayCount);
void threadUploadCharData(json& j);
bool updateServerWindow(bool reset);
bool updateLocalWindow(bool reset);
bool setUpdateServerWindow(bool reset);
bool setUpdateLocalWindow(bool reset);
void threadReturnCheck();
json convertCBRtoJson(CbrData& cbr, std::string uploaderID);
class CharacterStorage {
private:



public:



};
