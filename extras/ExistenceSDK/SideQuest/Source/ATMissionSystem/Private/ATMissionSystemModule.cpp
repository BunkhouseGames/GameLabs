// Copyright 2021 Arctic Theory ehf.. All Rights Reserved.

#include "ATMissionSystemModule.h"
#include "ATMissions.h"

#define LOCTEXT_NAMESPACE "FATMissionSystemModule"

FATMissionSystemModule::FATMissionSystemModule()
{
}


FATMissionSystemModule::~FATMissionSystemModule()
{
}


void FATMissionSystemModule::StartupModule()
{
}

void FATMissionSystemModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FATMissionSystemModule, ATMissionSystem)
