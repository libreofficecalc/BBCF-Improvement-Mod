#pragma once
#include "json.hpp"
#include "CBR/CbrData.h"

using json = nlohmann::json;
void testHTTPPost(json& j);
json testHTTPReq();
std::string ConvertCbrDataToBase64(CbrData& cbr);
class CharacterStorage {
private:



public:



};
