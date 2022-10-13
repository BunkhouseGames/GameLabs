// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "KamoStructs.h"

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "KamoRuntimeInterface.generated.h"

#if WITH_SERVER_CODE
	#define WITH_KAMO_RUNTIME 1
#else
	#define WITH_KAMO_RUNTIME 0
#endif


UENUM(BlueprintType)
enum class EKamoLoadChildObject : uint8
{
	Success,
	NotOurRegion,
	NotFound,
	RegisterFailed,
};
 