// 

#pragma once

#include "CoreMinimal.h"
#include "StaticSlotInfo.h"
#include "Components/ActorComponent.h"
#include "InstanceCollisionComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LANDMASSENGINE_API UInstanceCollisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInstanceCollisionComponent();
	
	void SetInstances(const TArray<FTransform>& Transforms, UInstancedStaticMeshComponent* InMeshComponent, FName InCollisionProfile);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMesh* StaticMesh;

	void SetInstanceEnabled(int32 InstanceIndex, bool bEnabled);
	void CreatePhysicsBodies();

	UFUNCTION()
	void OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FTransform> Instances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UInstancedStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName CollisionProfile;
	
	TArray<FBodyInstance*> InstanceBodies;
	bool NeedsCollision = false;
	bool HasCollision = false;
	FBoxSphereBounds Bounds;
	
	virtual void OnCreatePhysicsState() override;
	virtual void OnDestroyPhysicsState() override;
	virtual bool ShouldCreatePhysicsState() const override;
	virtual bool HasValidPhysicsState() const override;
};
