// Copyright Arctic Theory. All Rights Reserved.


#include "HarvestedProxy.h"
#include "HarvestableFieldAsset.h"

#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AHarvestedProxy::AHarvestedProxy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	bReplicates = true;
	// bAlwaysRelevant = true; // NOTE! Turn off when tuning for relevancy
	//NetUpdateFrequency = .05f;

	//Set root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Component"));
	RootComponent->SetMobility(EComponentMobility::Type::Movable);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component"));
	if (Mesh)
	{
		Mesh->SetMobility(EComponentMobility::Type::Movable);
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetCollisionProfileName("BlockAll", false);
		Mesh->SetIsReplicated(true);
	}
}

void AHarvestedProxy::HitPointsReachedZero_Implementation(AActor* ActorResponsible)
{
}

void AHarvestedProxy::TakeHit_Implementation(AActor* ActorResponsible, float Points)
{
	if (HasAuthority() || GetWorld()->IsNetMode(NM_Standalone))
	{
		HitPoints -= Points;
		if (HitPoints <= 0.0f)
		{
			HitPoints = 0.0f;
			HitPointsReachedZero(ActorResponsible);
		}
	}
}

void AHarvestedProxy::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHarvestedProxy, Target);
	DOREPLIFETIME(AHarvestedProxy, HitPoints);
}

void AHarvestedProxy::SetupActor()
{
	if (Target.Asset)
	{
		Mesh->SetStaticMesh(Target.Asset->Mesh);
		Mesh->SetCollisionProfileName(Target.Asset->CollisionProfile.Name);
	}
}


// Called when the game starts or when spawned
void AHarvestedProxy::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (bUseActorLifespan)
		{
			SetLifeSpan(ActorLifespan);
		}

		SetupActor();
	}
}

// Called every frame
void AHarvestedProxy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	return;

	if (!HasAuthority() || Target.Asset == nullptr)
	{
		return;
	}

	if (InitalizationState == NotInitialized)
	{
		InitalizationState = Aligning;
		// Do nothing for a sec
	}

	if (InitalizationState == Aligning && (!bUseAlignmentPeriod || TimeSinceBeginPlay > AlignmentPeriod))
	{
		InitalizationState = Done;
		Mesh->SetCollisionProfileName(Target.Asset->CollisionProfile.Name);
		Mesh->SetSimulatePhysics(true);
	}

	TimeSinceBeginPlay += DeltaTime;

	if (bNotifyWhenFallen && !bHaveNotifiedFallen)
	{
		float Angle = Mesh->GetUpVector().Z;

		if (-LevelThreshold <= Angle && Angle <= LevelThreshold)
		{
			bHaveNotifiedFallen = true;
			OnFallenOnGround.Broadcast();			
		}
	}
}

