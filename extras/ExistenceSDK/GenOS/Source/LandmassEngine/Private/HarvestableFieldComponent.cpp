// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "HarvestableFieldComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


// Sets default values for this component's properties
UHarvestableFieldComponent::UHarvestableFieldComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

}

TMap<int32, FTransform>  UHarvestableFieldComponent::GetHarvestableFieldStaticState()
{
	// Probably a good idea to do some caching here

	if (staticState.Num() > 0)
	{
		return staticState;
	}

	TArray<FVector> initialPoints = GenerateRandomPointsWithinField();
	TArray<FVector> groundAlignedPoints = GetPointsAlignedWithGround(initialPoints);
	int32 i = 0;
	
	for (FVector point : groundAlignedPoints)
	{
		FTransform transform = FTransform();
		transform.SetLocation(point);
		float randomYaw = random_stream.FRandRange(0.0f, 360.0f);
		transform.SetRotation(FQuat::MakeFromEuler(FVector(random_stream.FRandRange(0.0f, 360.0f), random_stream.FRandRange(0.0f, 360.0f), random_stream.FRandRange(0.0f, 360.0f))));
		float randomScale = random_stream.FRandRange(0.5f, 4.0f);
		transform.SetScale3D(FVector(randomScale,randomScale,randomScale));


		staticState.Add(i, transform);
		i++;
	}

	return staticState;
}

void UHarvestableFieldComponent::BeginPlay()
{
	Super::BeginPlay();

}
// A helper function that gets the points of the USplineComponent and translates that spline into a array of the Edge struct

TArray<Edge> UHarvestableFieldComponent::SplineToArrayOfEdges()
{
	TArray<Edge> result;

	int numSplinePoints = spline->GetNumberOfSplinePoints();

	// Loop through each spline point and get their location as a vector from their index and add it to the result polygon
	for (int i = 0; i < numSplinePoints; i++)
	{
		if (i + 1 >= numSplinePoints)
		{
			// At this point we have reached the end of the points, so we need to connect the final (and current) point to the first point.

			FVector point1 = spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			FVector point2 = spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);

			Edge currentEdge = Edge(point1.X, point1.Y, point2.X, point2.Y);

			result.Add(currentEdge);
		}
		else
		{
			// We connect the current point to the next one.
			FVector point1 = spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			FVector point2 = spline->GetLocationAtSplinePoint(i+1, ESplineCoordinateSpace::World);

			Edge currentEdge = Edge(point1.X, point1.Y, point2.X, point2.Y);

			result.Add(currentEdge);
		}
	}

	return result;
}

// This function generates a number of random points equal to the num_points specified within the edges array and adds them to the array of FVectors passed by references
TArray<FVector> UHarvestableFieldComponent::GenerateRandomPointsWithinPolygonEdges(TArray<Edge> edges)
{
	// We want to make sure the points array is empty when we start this process
	TArray<FVector> points_result;

	// Now we need to save the indexes of the edges
	int n = edges.Num();
	TArray<int> indexes = TArray<int>();
	for (int i = 0; i < n; i++)
	{
		indexes.Add(i);
	}

	int selectedIndex1 = 0;
	int selectedIndex2 = 0;

	// Now we need to generate the random points
	// This is done by selecting two seperate edges from the edges array at the same time (thus the indexes array)
	// Then we find a random point on both edges and find a random point between those points.
	for (int i = 0; i < num_points; i++)
	{
		// First we pick two of the indexes and remove them from the index array.
		selectedIndex1 = indexes[random_stream.RandRange(0, indexes.Num() - 1)];
		indexes.Remove(selectedIndex1);

		selectedIndex2 = indexes[random_stream.RandRange(0, indexes.Num() - 1)];
		indexes.Remove(selectedIndex2);

		// Then we find the random point and add it to the 
		FVector random_point = GenerateRandomPointWithinEdges(edges[selectedIndex1], edges[selectedIndex2]);
		points_result.Add(random_point);

		// Then we add the indexes back into the array
		indexes.Add(selectedIndex1);
		indexes.Add(selectedIndex2);
	}

	return points_result;
}

void UHarvestableFieldComponent::SetSpline(USplineComponent* InputSpline)
{
	spline = InputSpline;
}

// A helper function that gets a random point within two edges, by calling GenerateRandomPointOnEdge for the two edges, then getting a random point with a temporary edge between the two randomly generated points on the seperate edges.
FVector UHarvestableFieldComponent::GenerateRandomPointWithinEdges(Edge e1, Edge e2)
{
	FVector randomPointOnFirstEdge = GenerateRandomPointOnEdge(e1);
	FVector randomPointOnSecondEdge = GenerateRandomPointOnEdge(e2);

	// For the sake of simplicity we make an edge between these two random points
	Edge temp = Edge(randomPointOnFirstEdge.X, randomPointOnFirstEdge.Y, randomPointOnSecondEdge.X, randomPointOnSecondEdge.Y);

	FVector random_point = GenerateRandomPointOnEdge(temp);

	return random_point;
}

// A helper function that gets a random point on an edge.
FVector UHarvestableFieldComponent::GenerateRandomPointOnEdge(Edge e)
{
	FVector point;

	if (e.X1 == e.X2)
	{
		float n = random_stream.FRandRange(0.0f, 1.0f);
		point.X = e.X1;
		point.Y = ((e.Y2 - e.Y1) * n) + e.Y1;
	}
	else
	{
		float n = random_stream.FRandRange(0.0f, 1.0f);
		point.X = ((e.X2 - e.X1) * n) + e.X1;
		point.Y = (e.slope * (point.X - e.X1)) + e.Y1;
	}

	return point;
}

// A wrapper function to make the combined functionality of SplineToArrayOfEdges and GenerateRandomPointsWithinPolygonEdges accessible to blueprints. Currently does not work concave polygons, or rather, irregular polygons.
TArray<FVector> UHarvestableFieldComponent::GenerateRandomPointsWithinField()
{
    // Convert the spline to a polygon that we can then use to generate random points within
	TArray<Edge> edges = SplineToArrayOfEdges();

	TArray<FVector> points_result_container = GenerateRandomPointsWithinPolygonEdges(edges);

	return points_result_container;
}

// Raycast down to the ground and returns a list of points that are valid
TArray<FVector> UHarvestableFieldComponent::GetPointsAlignedWithGround(TArray<FVector> points)
{
	TArray<FVector> alignedPoints;
	
	FCollisionObjectQueryParams QueryParams(FCollisionObjectQueryParams::AllStaticObjects);
	FCollisionQueryParams CollParm(FCollisionQueryParams::DefaultQueryParam);

	for (FVector point : points)
	{
		FVector top = point + FVector(0, 0, 5000);
		FVector bottom = point - FVector(0, 0, 5000);
		FHitResult Result;

		this->GetWorld()->LineTraceSingleByObjectType(Result, top, bottom, QueryParams, CollParm);
		
		if(Result.IsValidBlockingHit())
		{
			alignedPoints.Add(Result.ImpactPoint);
		}
	}

	return alignedPoints;
}

