// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "KamoDriver.h"

#include "CoreMinimal.h"
#include "Templates/UniquePtr.h"
#include "KamoStructs.h"
#include "Dom/JsonObject.h"


class KAMORUNTIME_API IKamoDB : public virtual IKamoDriver
{
    
public:
	virtual ~IKamoDB() { };

	virtual class IKamoDriver* GetDriver() = 0;

    static TUniquePtr<IKamoDB> CreateDriver(const FString& name);
    
    // Unified object API
    virtual bool Set(const KamoRootObject& object) = 0;
    virtual bool Set(const KamoChildObject& object) = 0;
    virtual bool Set(const KamoHandlerObject& object) = 0;
    virtual bool Delete(const KamoID& id) = 0;
    virtual TSharedPtr<KamoObject> Get(const KamoID& id) const = 0;

    // Check if object is pending serialization to DB and bump priority if needed
    virtual bool IsSerializationPending(const KamoID& id, bool bump_priority = true) = 0;
    virtual bool CancelIfPending(const KamoID& id) = 0;
    
    // Root object API
    virtual bool AddRootObject(const KamoID& id, const FString& state, bool ignore_if_exists=false) = 0;
    virtual bool DeleteRootObject(const KamoID& id) = 0;
    virtual bool DeleteChildObject(const KamoID& root_id, const KamoID& id) = 0;
    virtual bool UpdateRootObject(const KamoID& id, const FString& state) = 0;
    virtual KamoRootObject GetRootObject(const KamoID& id) const = 0;
    // 'class_name' is optional
    virtual TArray<KamoRootObject> FindRootObjects(const FString& class_name) const = 0;
    virtual bool SetHandler(const KamoID& id, const KamoID& handler_id) = 0;
    
    // Object API
    virtual bool AddObject(const KamoID& root_id, const KamoID& id, const FString& state) = 0;
    virtual bool DeleteObject(const KamoID& id) = 0;
    virtual bool UpdateObject(const KamoID& id, const FString& state) = 0;
    virtual KamoChildObject GetObject(const KamoID& id, bool fail_silently = false) const = 0;
    // Specify either 'root_id', 'class_name'
    virtual TArray<KamoChildObject> FindObjects(const KamoID& root_id, const FString& class_name) const = 0;
    virtual bool MoveObject(const KamoID& id, const KamoID& root_id) = 0;
    
    // Handler API
    virtual bool AddHandlerObject(const KamoHandlerObject& handler) = 0;
    virtual bool DeleteHandlerObject(const KamoID& handler_id) = 0;
    virtual bool RefreshHandler(const KamoID& handler_id, const TSharedPtr < FJsonObject >& stats) = 0;
    virtual KamoHandlerObject GetHandlerInfo(const KamoID& handler_id) const = 0;
};
