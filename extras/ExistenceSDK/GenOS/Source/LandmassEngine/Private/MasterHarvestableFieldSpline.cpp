// Fill out your copyright notice in the Description page of Project Settings.


#include "MasterHarvestableFieldSpline.h"

#include "Components/SplineComponent.h"


AMasterHarvestableFieldSpline::AMasterHarvestableFieldSpline()
{
	// Create a spline and initialize it with four points making a sort of a box
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Harvestable Field Bounds"));
	Spline->SetClosedLoop(true);
	Spline->RemoveSplinePoint(0);
	Spline->RemoveSplinePoint(0);	
	Spline->AddSplineLocalPoint(FVector(InitialSize, InitialSize, 0.0));
	Spline->AddSplineLocalPoint(FVector(-InitialSize, InitialSize, 0.0));
	Spline->AddSplineLocalPoint(FVector(-InitialSize, -InitialSize, 0.0));
	Spline->AddSplineLocalPoint(FVector(InitialSize, -InitialSize, 0.0));
	Spline->SetupAttachment(RootComponent);
}

FBox AMasterHarvestableFieldSpline::GetSplineBounds() const
{
	// Find smallest bounds around spline by traversing the spline with somewhat fine granularity
	float Increments = 0.1;
	FBox Box;
	Box.Min.X = 10000000.0;
	Box.Max.X = -10000000.0;
	Box.Min.Y = 10000000.0;
	Box.Max.Y = -10000000.0;
	Box.Min.Z = 10000000.0;
	Box.Max.Z = -10000000.0;


	TArray<USplineComponent*> Splines;
	GetComponents<USplineComponent>(Splines);

	for (USplineComponent* SplineComp : Splines)
	{
		for (float Key = 0.0; Key < SplineComp->GetNumberOfSplinePoints(); Key += Increments)
		{
			FVector Location = SplineComp->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World);
			if (Location.X > Box.Max.X) Box.Max.X = Location.X;
			if (Location.X < Box.Min.X) Box.Min.X = Location.X;
			if (Location.Y > Box.Max.Y) Box.Max.Y = Location.Y;
			if (Location.Y < Box.Min.Y) Box.Min.Y = Location.Y;
			if (Location.Z > Box.Max.Z) Box.Max.Z = Location.Z;
			if (Location.Z < Box.Min.Z) Box.Min.Z = Location.Z;
		}
	}

	return Box;
}

FBox AMasterHarvestableFieldSpline::GetBoundingBox() const
{
	FBox SplineBounds = GetSplineBounds();
	
	// We treat the spline as being two dimensional but for the ray trace we must 
	// make sure the bounds cover the ground below the spline.
	SplineBounds.Min.Z -= 25000; // 250 meters below spline
	return SplineBounds;
}

bool AMasterHarvestableFieldSpline::IsPointInSpline(const FVector& Point) const
{
	// Using code from the Internet verbatim. Obvious weirdness includes the use of the defines below and the 
	// fixed buffer size which are potential time bombs. Also turning the spline data into the data stream array 
	// is just a time to market thing.

	// Point - In - Spline - Polygon Algorithm — Testing Whether A Point Is Inside A Complex Polygon With Spline Curves
	// https ://alienryderflex.com/polyspline/

#define  SPLINE     2.
#define  NEW_LOOP   3.
#define  END       -2.

	double  Sx, Sy, Ex, Ey, a, b, sRoot, F, plusOrMinus, topPart, bottomPart, xPart;
	int     i = 0, j, k, start = 0;
	bool    oddNodes = false;
	double X = Point.X;
	double Y = Point.Y;

	double polybuff[1000];

	double* poly = polybuff;

	// Populate polybuff using our spline info.
	TArray<USplineComponent*> Splines;
	GetComponents<USplineComponent>(Splines);

	for (USplineComponent* SplineComp : Splines)
	{
		for (int Index = 0; Index < SplineComp->GetNumberOfSplinePoints(); Index++)
		{
			FVector Location = SplineComp->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
			auto Type = SplineComp->GetSplinePointType(Index);
			/*
					Linear,
					Curve,
					Constant,
					CurveClamped,
					CurveCustomTangent
			*/
			if (Type == ESplinePointType::Curve)
			{
				*poly++ = SPLINE;
			}
			*poly++ = Location.X;
			*poly++ = Location.Y;
		}
		*poly++ = NEW_LOOP;
	}

	*poly++ = END;
	poly = polybuff;


	Y += .000001;   //  prevent the need for special tests when F is exactly 0 or 1

	while (poly[i] != END) 
	{
		j = i + 2; 
		if (poly[i] == SPLINE)
		{
			j++;
		}

		if (poly[j] == END || poly[j] == NEW_LOOP)
		{
			j = start;
		}

		if (poly[i] != SPLINE && poly[j] != SPLINE) 
		{   //  STRAIGHT LINE
			if ((poly[i + 1] < Y && poly[j + 1] >= Y) || (poly[j + 1] < Y && poly[i + 1] >= Y)) 
			{
				if (poly[i] + (Y - poly[i + 1]) / (poly[j + 1] - poly[i + 1]) * (poly[j] - poly[i]) < X)
				{
					oddNodes = !oddNodes;
				}
			}
		}
		else if (poly[j] == SPLINE) 
		{   //  SPLINE CURVE
			a = poly[j + 1]; b = poly[j + 2]; k = j + 3; if (poly[k] == END || poly[k] == NEW_LOOP)
			{
				k = start;
			}
			
			if (poly[i] != SPLINE) 
			{
				Sx = poly[i]; Sy = poly[i + 1];
			}
			else 
			{   //  interpolate hard corner
				Sx = (poly[i + 1] + poly[j + 1]) / 2.; Sy = (poly[i + 2] + poly[j + 2]) / 2.;
			}

			if (poly[k] != SPLINE) 
			{
				Ex = poly[k]; Ey = poly[k + 1];
			}
			else 
			{   //  interpolate hard corner
				Ex = (poly[j + 1] + poly[k + 1]) / 2.; Ey = (poly[j + 2] + poly[k + 2]) / 2.;
			}
			
			bottomPart = 2. * (Sy + Ey - b - b);
			
			if (bottomPart == 0.) 
			{   //  prevent division-by-zero
				b += .0001; bottomPart = -.0004;
			}

			sRoot = 2. * (b - Sy); 
			sRoot *= sRoot; 
			sRoot -= 2. * bottomPart * (Sy - Y);

			if (sRoot >= 0.) 
			{
				sRoot = sqrt(sRoot); topPart = 2. * (Sy - b);
				for (plusOrMinus = -1.; plusOrMinus < 1.1; plusOrMinus += 2.) 
				{
					F = (topPart + plusOrMinus * sRoot) / bottomPart;
					if (F >= 0. && F <= 1.) 
					{
						xPart = Sx + F * (a - Sx); 
						if (xPart + F * (a + F * (Ex - a) - xPart) < X)
						{
							oddNodes = !oddNodes;
						}
					}
				}
			}
		}

		if (poly[i] == SPLINE)
		{
			i++;
		}

		i += 2;

		if (poly[i] == NEW_LOOP) 
		{
			i++; start = i;
		}
	}

	return oddNodes;
}
bool AMasterHarvestableFieldSpline::GetRandomLocation(const FBox& BBox, FVector& Location)
{
	for (int i = 0; i < RandNumInstances; i++)
	{
		for (int j = 0; j < MaxTries; j++)
		{
			FVector Point(
				Rand.FRandRange(BBox.Min.X, BBox.Max.X),
				Rand.FRandRange(BBox.Min.Y, BBox.Max.Y),
				Rand.FRandRange(BBox.Min.Z, BBox.Max.Z)
			);

			if (IsPointInSpline(Point))
			{
				Location = Point;
				return true;
			}
		}
	}
	
	return false;
}
