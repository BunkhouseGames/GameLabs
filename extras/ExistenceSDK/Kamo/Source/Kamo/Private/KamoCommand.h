// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KamoState.h"
#include "KamoCommand.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class KAMO_API UKamoCommand : public UObject
{
	GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, Category = "KamoCommand")
    static UKamoCommand* CreateKamoCommand(FString command, FString parent, FString id, UKamoState* parameters);
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn="true"), Category = "KamoCommand")
    FString command;
	
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoCommand")
    FString parent;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoCommand")
    FString id;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "KamoCommand")
    UKamoState* parameters;
};
