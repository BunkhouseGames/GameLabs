// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KamoDB.h"
#include "KamoFileDriver.h"
#include "KamoStructs.h"
#include "KamoFileHelper.h"

#include "CoreMinimal.h"
#include "Json.h"
#include "HAL/PlatformFileManager.h"
#include "Json.h"
#include "Async/AsyncWork.h"
#include "Misc/ScopeLock.h"


/**
 * 
 */
class KAMORUNTIME_API KamoFileDB : public IKamoDB, public KamoFileDriver
{
    FString GetRootFilePath(const KamoID& id) const;
    FString GetFilePath(const KamoID& root_id, const KamoID& kamo_id) const;
    
    FString GetObjectPath(const KamoID& id) const;
	KamoID GetObjectIDFromPath(const FString& path) const;
    
    TSharedPtr<FJsonObject> GetJsonObject(const FString& data) const;
    FString GetJsonString(const TSharedPtr<FJsonObject>& json_object) const;

    // File locks
    TMap<FString, IKamoFileHandle*> file_locks;

    // Serialization job management
    struct SerializationRecord
    {
        KamoID id;
        KamoID root_id;
        FString state;
        int priority;
    };

    FCriticalSection mutex;
    TMap<FString, SerializationRecord> objects_for_serialization;

    // Serializer worker
    class FDBSerializerWorker : public FNonAbandonableTask
    {
    public:
        KamoFileDB* file_db;
        friend class FAsyncTask<FDBSerializerWorker>;

        FORCEINLINE TStatId GetStatId() const
        {
            RETURN_QUICK_DECLARE_CYCLE_STAT(FDBSerializerWorker, STATGROUP_KamoAsync);
        }

        void DoWork() { file_db->DoWork(); }        
    };

    FAsyncTask<FDBSerializerWorker> serializer;
    void DoWork();

    
public:
	KamoFileDB();
	virtual ~KamoFileDB() override;

	// IKamoDriver
	FString GetDriverType() const override { return "db"; }
    void CloseSession() override;

	// IKamoDB
	class IKamoDriver* GetDriver() override { return nullptr; }
    
    // Unified object API
    virtual bool Set(const KamoRootObject& object);
    virtual bool Set(const KamoChildObject& object);
    virtual bool Set(const KamoHandlerObject& object);
    virtual bool Delete(const KamoID& id);
    virtual TSharedPtr<KamoObject> Get(const KamoID& id) const;

    // Check if object is pending serialization to DB and bump priority if needed
    virtual bool IsSerializationPending(const KamoID& id, bool bump_priority = true);
    
    // Remove object from serialization queue if it's there. Returns true if removed.
    virtual bool CancelIfPending(const KamoID& id) override;
    
    // Root object API
    virtual bool AddRootObject(const KamoID& id, const FString& state, bool ignore_if_exists=false);
    virtual bool DeleteRootObject(const KamoID& id);
    virtual bool UpdateRootObject(const KamoID& id, const FString& state);
    virtual KamoRootObject GetRootObject(const KamoID& id) const;
    // 'class_name' is optional
    virtual TArray<KamoRootObject> FindRootObjects(const FString& class_name) const;
    virtual bool SetHandler(const KamoID& id, const KamoID& handler_id);

    // Object API
    virtual bool AddObject(const KamoID& root_id, const KamoID& id, const FString& state);
    virtual bool DeleteObject(const KamoID& id);
    virtual bool DeleteChildObject(const KamoID& root_id, const KamoID& id) override;
    virtual bool UpdateObject(const KamoID& id, const FString& state);
    virtual KamoChildObject GetObject(const KamoID& id, bool fail_silently = false) const override;
    // Specify either 'root_id', 'class_name'
    virtual TArray<KamoChildObject> FindObjects(const KamoID& root_id, const FString& class_name) const;
    virtual bool MoveObject(const KamoID& id, const KamoID& root_id);
    
    // Handler API
    virtual bool AddHandlerObject(const KamoHandlerObject& handler);
    virtual bool DeleteHandlerObject(const KamoID& handler_id);
    virtual bool RefreshHandler(const KamoID& handler_id, const TSharedPtr < FJsonObject >& stats);
    virtual KamoHandlerObject GetHandlerInfo(const KamoID& handler_id) const;
};
