// Copyright Directive Games, Inc. All Rights Reserved.

#pragma once

#include "KamoBeaconClient.h"
#include "KamoRt.h"


#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OnlineBeaconHostObject.h"
#include "KamoBeaconHost.generated.h"


class UKamoRuntime;


/**
 * A beacon host for relaying Kamo Registry information
 */
UCLASS(transient, notplaceable, config=Engine)
class KAMO_API AKamoBeaconHostObject : public AOnlineBeaconHostObject, public IKamoBeaconServerInterface
{
	GENERATED_UCLASS_BODY()

	//~ Begin AOnlineBeaconHost Interface 
	virtual AOnlineBeaconClient* SpawnBeaconActor(class UNetConnection* ClientConnection) override;
	virtual void OnClientConnected(class AOnlineBeaconClient* NewClientActor, class UNetConnection* ClientConnection) override;
	//~ End AOnlineBeaconHost Interface 

	//~ Begin IKamoBeaconServerInterface Interface 
	virtual bool LookupPlayer(const FString& InPlayerIdentity, FKamoPlayerInfo& OutPlayerInfo) const override;
	virtual bool RefreshWorldState(AKamoWorldState* OutWorldState) override;
	virtual void LaunchServerForPlayer(const FKamoPlayerInfo& PlayerInfo) override;
	virtual void MovePlayerToRegion(const FString& InObjectID, const FString& InRegion, const FString& SpawnTarget) override;
	//~ End IKamoBeaconServerInterface Interface 

	virtual bool Init(UKamoRuntime* InKamoRuntime);
	
	// Update some state TEMP HACK
	void Update(float DeltaTime);

protected:

	bool GetHandlerInfo(const KamoID& HandlerId, FKamoHandlerInfo& HandlerInfo) const;

	UPROPERTY()
	UKamoRuntime* KamoRuntime;

	UPROPERTY()
	AKamoWorldState* KamoWorldState;

	float TimeSinceWorldStateUpdate;
	float TimeSincePlayerInfoUpdate;

	bool bStaleHandlersReported = false;
};
