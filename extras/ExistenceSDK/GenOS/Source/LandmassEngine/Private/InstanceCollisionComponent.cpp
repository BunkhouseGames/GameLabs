// 


#include "InstanceCollisionComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UInstanceCollisionComponent::UInstanceCollisionComponent()
{
}

void UInstanceCollisionComponent::SetInstances(const TArray<FTransform>& Transforms, UInstancedStaticMeshComponent* InMeshComponent, FName InCollisionProfile)
{
	MeshComponent = InMeshComponent;
	Bounds = MeshComponent->CalcBounds(MeshComponent->GetComponentToWorld());
	Bounds.BoxExtent.Z = 10000.0f;

	Instances = Transforms;
	CollisionProfile = InCollisionProfile;

	GetOwner()->OnActorBeginOverlap.AddDynamic(this, &UInstanceCollisionComponent::OnActorBeginOverlap);
}

void UInstanceCollisionComponent::SetInstanceEnabled(int32 InstanceIndex, bool bEnabled)
{
	if (InstanceBodies.Num() == 0)
	{
		NeedsCollision = true;
		CreatePhysicsBodies();
	}
	FBodyInstance* & Instance = InstanceBodies[InstanceIndex];
	if (Instance && !bEnabled)
	{
		Instance->SetResponseToAllChannels(ECR_Ignore);
	}
}

void UInstanceCollisionComponent::CreatePhysicsBodies()
{
	if (!MeshComponent)
	{
		return;
	}

	check(InstanceBodies.Num() == 0);

	int32 NumInstances = Instances.Num();
	InstanceBodies.Reserve(NumInstances);

	for (int32 Index = 0; Index < NumInstances; ++Index)
	{
		FBodyInstance* BodyInstance = new FBodyInstance();
		BodyInstance->InstanceBodyIndex = Index;
		if (!CollisionProfile.IsNone())
		{
			BodyInstance->SetCollisionProfileName(CollisionProfile);
		}
		InstanceBodies.Add(BodyInstance);
	}
	
	UBodySetup* BodySetup = MeshComponent->GetStaticMesh()->GetBodySetup();

	FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();
	FBodyInstance::InitStaticBodies(InstanceBodies, Instances, BodySetup, MeshComponent, PhysScene);
	HasCollision = true;
	GetOwner()->OnActorBeginOverlap.RemoveDynamic(this, &UInstanceCollisionComponent::OnActorBeginOverlap);

	INC_DWORD_STAT_BY(STAT_HF_NumCollisionBodies, InstanceBodies.Num());

	UE_LOG(LogHF, Display, TEXT("Created collision for %s"), *GetFName().ToString());
}

void UInstanceCollisionComponent::OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!MeshComponent)
	{
		// Mesh hasn't been set up
		return;
	}
	if (HasCollision)
	{
		// Collision has already been set up
		return;
	}

	if (Cast<APawn>(OtherActor))
	{
		NeedsCollision = true;
		RecreatePhysicsState();
	}
}

void UInstanceCollisionComponent::OnCreatePhysicsState()
{
	SCOPE_CYCLE_COUNTER(STAT_HF_OnCreatePhysicsState);
	if (NeedsCollision && !HasCollision)
	{
		CreatePhysicsBodies();
	}
	bPhysicsStateCreated = true;
}

void UInstanceCollisionComponent::OnDestroyPhysicsState()
{
	for (auto Instance : InstanceBodies)
	{
		if (Instance)
		{
			Instance->TermBody();
		}
	}
	INC_DWORD_STAT_BY(STAT_HF_NumCollisionBodies, -InstanceBodies.Num());
	
	InstanceBodies.Empty();
	HasCollision = false;
	bPhysicsStateCreated = false;
}

bool UInstanceCollisionComponent::ShouldCreatePhysicsState() const
{
	return MeshComponent && Instances.Num();
}

bool UInstanceCollisionComponent::HasValidPhysicsState() const
{
	return InstanceBodies.Num() == Instances.Num();
}
