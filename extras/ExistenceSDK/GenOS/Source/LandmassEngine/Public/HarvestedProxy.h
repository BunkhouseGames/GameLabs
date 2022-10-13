// Copyright Arctic Theory. All Rights Reserved.

#pragma once

#include "TargetSlotInfo.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HarvestedProxy.generated.h"

UCLASS(BlueprintType, Blueprintable)
class LANDMASSENGINE_API AHarvestedProxy : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHarvestedProxy();

	UPROPERTY(BlueprintReadOnly, Replicated)
	FTargetSlotInfo Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	float HitPoints = 1.0f;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly)
	void TakeHit(AActor* ActorResponsible, float Points);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void HitPointsReachedZero(AActor* ActorResponsible);
	
	UPROPERTY(EditAnywhere, Category = Harvested, AdvancedDisplay)
	bool bUseAlignmentPeriod;

	// How long in seconds to allow for spawn alignment before turning on physics
	UPROPERTY(EditAnywhere, Category=Harvested, AdvancedDisplay, meta = (editcondition = "bUseAlignmentPeriod", ClampMin = "0.001", UIMin = "0.001"))
	float AlignmentPeriod = 0.1;

	UPROPERTY(EditAnywhere, Category = Harvested, AdvancedDisplay)
	bool bUseActorLifespan;

	// Actor will delete iself after its lifetime(in seconds) has passed.
	UPROPERTY(EditAnywhere, Category = Harvested, meta = (editcondition = "bUseActorLifespan", ClampMin = "0.001", UIMin = "0.001"))
	float ActorLifespan = 1000.0;

	// Notify when object is level with the ground (i.e. a tree is fallen and lies on the ground). TODO: Extend this as this only works on level ground. 
	UPROPERTY(EditAnywhere, Category = Harvested, AdvancedDisplay)
	bool bNotifyWhenFallen;

	UPROPERTY(EditAnywhere, Category = Harvested, meta = (editcondition = "bNotifyWhenFallen", ClampMin = "0.001", UIMin = "0.001"))
	float LevelThreshold = 0.05; // The object is level if its angle is within 5%

	bool bHaveNotifiedFallen = false;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFallenOnGround);

	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnFallenOnGround OnFallenOnGround;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* Mesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void SetupActor();
	
	enum
	{
		NotInitialized,
		Aligning,
		Done
	} InitalizationState = NotInitialized;

	float TimeSinceBeginPlay = 0.0;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
