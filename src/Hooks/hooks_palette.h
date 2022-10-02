#pragma once
#include "CBR/Metadata.h"

bool placeHooks_palette();

std::shared_ptr<Metadata> RecordCbrMetaData(char*, bool);

char ReplayCbrData();

void CBRLogic(char*);