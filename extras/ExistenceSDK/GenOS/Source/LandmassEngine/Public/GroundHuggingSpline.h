// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "GroundHuggingSpline.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LANDMASSENGINE_API UGroundHuggingSpline : public USplineComponent
{
	GENERATED_BODY()

public:
	UGroundHuggingSpline();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USplineComponent* SourceSpline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Interval = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Range = 1000.0f;
	
	UFUNCTION(BlueprintCallable, CallInEditor)
	void RebuildSpline();
};
