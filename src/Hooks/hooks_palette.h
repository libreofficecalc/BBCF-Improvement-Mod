#pragma once
#include "CBR/Metadata.h"
#include "CBR/CbrUtils.h"

bool placeHooks_palette();

std::shared_ptr<Metadata> RecordCbrMetaData(bool, int);

char ReplayCbrData();

void CBRLogic(char* addr, int hirarchy, bool readOnly, bool writeOnly, int playerNr, bool netplayMemory = false);
int CBRLogic(int input, int playerNr, int controllerNr, bool readOnly, bool writeOnly, bool netplayMemory = false);
int CBRLogic(int input, int hirarchy, int playerNr, int controllerNr, bool readOnly, bool writeOnly, bool netplayMemory = false);
void reversalLogic(char* addr, int input, std::shared_ptr<Metadata> meta, int playerNR);
int reversalLogic(int input, int playerNR, bool readOnly, bool writeOnly);
void RecordCbrHelperData(std::shared_ptr<Metadata> me, bool PlayerIndex);
int netaLogic(int input, std::shared_ptr<Metadata> meta, int playerNR, bool readOnly, bool writeOnl);