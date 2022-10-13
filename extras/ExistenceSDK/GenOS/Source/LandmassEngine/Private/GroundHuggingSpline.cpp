// Fill out your copyright notice in the Description page of Project Settings.


#include "GroundHuggingSpline.h"


UGroundHuggingSpline::UGroundHuggingSpline()
{
	SourceSpline = CreateDefaultSubobject<USplineComponent>(TEXT("SourceSpline"));
	SourceSpline->CreationMethod = EComponentCreationMethod::Instance;
}

void UGroundHuggingSpline::RebuildSpline()
{
	ClearSplinePoints(false);

	if (!SourceSpline)
	{
		return;
	}

	float SplineLength = SourceSpline->GetSplineLength();
	int32 NumPoints = SplineLength / Interval + 1;

	for (int32 PointIndex = 0; PointIndex < NumPoints; ++PointIndex)
	{
		float Distance = PointIndex * Interval;
		Distance = FMath::Min(Distance, SplineLength);
		FVector Location = SourceSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		FVector P0 = Location;
		P0.Z += Range;
		FVector P1 = Location;
		P1.Z -= Range;

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.MobilityType = EQueryMobilityType::Static;
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		if (GetWorld()->LineTraceSingleByObjectType(HitResult, P0, P1, ObjectQueryParams))
		{
			Location = HitResult.ImpactPoint;
		}
		AddSplineWorldPoint(Location);
	}
}
