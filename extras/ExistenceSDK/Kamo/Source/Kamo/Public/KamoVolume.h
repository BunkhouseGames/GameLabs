// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "Engine/TriggerVolume.h"
#include "KamoVolume.generated.h"

/**
 * 
 */
UCLASS()
class KAMO_API AKamoVolume : public AActor
{
	GENERATED_UCLASS_BODY()

		//~ Begin UObject Interface.
#if WITH_EDITOR
		virtual void LoadedFromAnotherClass(const FName& OldClassName) override;
#endif // WITH_EDITOR	
	//~ End UObject Interface.
};
