// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "KamoClientModule.h"
#include "KamoBeaconClient.h"

DEFINE_LOG_CATEGORY(LogKamoClientModule);


FKamoClientModule::FKamoClientModule()
{
#if !UE_BUILD_SHIPPING
    if (GEngine->LocalPlayerClass == ULocalPlayer::StaticClass())
    {
        GEngine->LocalPlayerClass = ULocalKamoPlayer::StaticClass();
    }
    else
    {
        //UE_LOG();
    }
#endif
}


FKamoClientModule::~FKamoClientModule()
{
}


void FKamoClientModule::StartupModule()
{
}

void FKamoClientModule::ShutdownModule()
{
}


FString ULocalKamoPlayer::GetNickname() const
{
    FString PIEPlayerId = Super::GetNickname();
    UWorld* World = GetWorld();

    // Note! The following code is mirrored in KamoClientModule.cpp and KamoBeaconClient.cpp
#if !UE_BUILD_SHIPPING    
    // If we are in PIE we pass in a PIE specific player id.
    if (World->WorldType == EWorldType::PIE)
    {
        PIEPlayerId = FString::Printf(TEXT("pieplayer_%i"), World->GetPackage()->GetPIEInstanceID());
    }
#endif

    return PIEPlayerId;
}


IMPLEMENT_MODULE(FKamoClientModule, KamoClient)

