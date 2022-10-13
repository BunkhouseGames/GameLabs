// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/Polys.h"
#include "Runtime/Engine/Classes/Components/SplineComponent.h"
#include "Components/ActorComponent.h"
#include "Math/RandomStream.h"

#include "HarvestableFieldComponent.generated.h"

USTRUCT(BlueprintType)
struct FHarvestableSlotData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 id;

	UPROPERTY(BlueprintReadWrite)
	FString type;

};

struct Edge
{
	float X1, Y1, X2, Y2;
	float slope;

	Edge(float x1, float y1, float x2, float y2)
	{
		X1 = x1;
		Y1 = y1;
		X2 = x2;
		Y2 = y2;

		slope = (y2 - y1) / (x2 - x1);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LANDMASSENGINE_API UHarvestableFieldComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	FRandomStream random_stream;

public:	
	// Sets default values for this component's properties
	UHarvestableFieldComponent();

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, Category = "HarvestableFieldHelperFunction", EditAnywhere, Meta = (ExposeOnSpawn = "true"))
	int32 initial_seed;

	UPROPERTY(BlueprintReadWrite, Category = "HarvestableFieldHelperFunction", EditAnywhere, Meta = (ExposeOnSpawn = "true"))
	int32 num_points = 150;

	UFUNCTION(BlueprintCallable)
	void SetSpline(USplineComponent* InputSpline);

	UPROPERTY(BlueprintReadWrite)
	TArray<FHarvestableSlotData> Slots;

	UFUNCTION(BlueprintCallable, Category = "HarvestableFieldHelperFunction")
	TArray<FVector> GenerateRandomPointsWithinField();

	UFUNCTION()
	TArray<FVector> GetPointsAlignedWithGround(TArray<FVector> points);

	UFUNCTION(BlueprintCallable)
	TMap<int32, FTransform> GetHarvestableFieldStaticState();

	UPROPERTY()
	TMap<int32, FTransform>	staticState;

	UPROPERTY(BlueprintReadWrite)
	TMap<int32, FHarvestableSlotData> modifiedState;


private:

	TArray<Edge> SplineToArrayOfEdges();
	TArray<FVector> GenerateRandomPointsWithinPolygonEdges(TArray<Edge> edges);
	FVector GenerateRandomPointWithinEdges(Edge e1, Edge e2);
	FVector GenerateRandomPointOnEdge(Edge e);

	USplineComponent* spline;

};
