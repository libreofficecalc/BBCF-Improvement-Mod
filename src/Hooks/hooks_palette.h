#pragma once
#include "CBR/Metadata.h"

bool placeHooks_palette();

std::shared_ptr<Metadata> RecordCbrMetaData(char*, bool);

char ReplayCbrData();

void CBRLogic(char*);
void reversalLogic(char* addr, int input, std::shared_ptr<Metadata> meta, int playerNR);
void RecordCbrHelperData(std::shared_ptr<Metadata> me, bool PlayerIndex);