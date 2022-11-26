#pragma once
#include "CBR/Metadata.h"

bool placeHooks_palette();

std::shared_ptr<Metadata> RecordCbrMetaData(bool);

char ReplayCbrData();

void CBRLogic(char*);
int CBRLogic(int input, int playerNr, int controllerNr);
void reversalLogic(char* addr, int input, std::shared_ptr<Metadata> meta, int playerNR);
int reversalLogic(int input, std::shared_ptr<Metadata> meta, int playerNR);
void RecordCbrHelperData(std::shared_ptr<Metadata> me, bool PlayerIndex);