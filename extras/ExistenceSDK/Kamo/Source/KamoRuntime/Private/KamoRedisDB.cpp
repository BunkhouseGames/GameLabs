// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoRedisDB.h"
#include "KamoRuntimeModule.h"
#include "KamoFileHelper.h"


#include "GenericPlatform/GenericPlatformTime.h"

#if WITH_REDIS_CLIENT

static FString session_path; // MAEK COMPILE NOWW

const float region_lock_refresh_interval = 4.4f; // TODO: Have this configurable
const int region_lock_timeout_value = 20;  // 20 second timeout for the region lock. TODO: Have this configurable



class LockMutex : public sw::redis::RedLockMutexVessel
{
public:
    LockMutex(std::shared_ptr<sw::redis::Redis> instance, const FString& lock_name, const FString& lock_id, int timeout) :
        sw::redis::RedLockMutexVessel(*instance),
        _lock_name(lock_name),
        _lock_id(lock_id),
        _timeout(timeout),
        _redisPtr(instance)
    {
        _lock_info.locked = false;
    }

    bool Lock()
    {
        // First try the lock with short timeout just to see if we can acquire it
        _lock_info = sw::redis::RedLockMutexVessel::lock(
            TCHAR_TO_UTF8(*_lock_name),                 // resource
            TCHAR_TO_UTF8(*_lock_id),                   // random_string
            std::chrono::milliseconds(100),             // ttl
            3                                           // retry_count
        );

        if (_lock_info.locked)
        {
            // Immediately extend the lock by our given timeout value
            Refresh();
        }

        if (!_lock_info.locked)
        {
            // See if we can get the lock id
            FString locker = "(lock key missing!)";
            auto val = _redisPtr->get(TCHAR_TO_UTF8(*_lock_name));
            if (val)
            {
                locker = FString(val->c_str());
            }
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::LockMutex on %s failed, already locked by %s."), *_lock_name, *locker);
            return false;
        }


        return _lock_info.locked;
    }

    void Unlock()
    {
        sw::redis::RedLockMutexVessel::unlock(_lock_info);
    }
    
    bool Refresh()
    {
        _lock_info = sw::redis::RedLockMutexVessel::extend_lock(_lock_info, std::chrono::seconds(_timeout));
        return _lock_info.locked;
    }

private:

    FString _lock_name;
    FString _lock_id;
    int _timeout;
    LockInfo _lock_info;
    std::shared_ptr<sw::redis::Redis> _redisPtr;
};



KamoRedisDB::KamoRedisDB() :
    last_region_lock_refresh_seconds(1000.0f)  // High enough number to trigger a refresh on first tick.
{
    serializer.GetTask().db = this;
    region_lock_refresher.GetTask().db = this;
}

KamoRedisDB::~KamoRedisDB()
{
    serializer.EnsureCompletion(true);
    region_lock_refresher.EnsureCompletion(true);
}


void KamoRedisDB::Tick(float DeltaTime)
{
    IKamoDriver::Tick(DeltaTime);

    // Refresh region locks if needed
    last_region_lock_refresh_seconds += DeltaTime;
    if (last_region_lock_refresh_seconds > region_lock_refresh_interval && region_lock_refresher.IsDone())
    {
        last_region_lock_refresh_seconds = 0.0f;
        region_lock_refresher.StartBackgroundTask(GIOThreadPool);
    }
}


// Root objects
bool KamoRedisDB::AddRootObject(const KamoID& id, const FString& state, bool ignore_if_exists) 
{
    FString key = RootKey(id);
    
    try
    {
        if (redisPtr->exists(TCHAR_TO_UTF8(*key)) == 1)
        {
            UE_CLOG(!ignore_if_exists, LogKamoRuntime, Display, TEXT("KamoFileDB::AddRootObject. Already exists: %s"), *id());
            return false;
        }

        if (!redisPtr->set(TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*state)))
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::AddRootObject failed for %s"), *id());
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::AddRootObject failed: %S"), e.what());
        return false;
    }

    return true;
}


bool KamoRedisDB::DeleteRootObject(const KamoID& id) 
{
    // NOTE: Not deleting child keys (yet)
    try
    {
        return redisPtr->del(TCHAR_TO_UTF8(*(RootKey(id)))) == 1;
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::DeleteRootObject of %s failed: %S"), *RootKey(id), e.what());
        return false;
    }
}

bool KamoRedisDB::UpdateRootObject(const KamoID& id, const FString& state) 
{
    FString key = RootKey(id);
    FString data;

    try
    {
        if (redisPtr->exists(TCHAR_TO_UTF8(*key)) != 1)
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::UpdateRootObject: Object doesn't exist: %s"), *id());
            return false;
        }

        if (!redisPtr->set(TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*state)))
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::UpdateRootObject failed for %s"), *id());
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::UpdateRootObject failed: %S"), e.what());
        return false;
    }

    return true;
}

KamoRootObject KamoRedisDB::GetRootObject(const KamoID& id) const
{
    KamoRootObject root_object;
    if (!initialized)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::GetRootObject: Driver not initialized, tenant=%s, session=%s"), *tenant_name, *session_info);
        return root_object;
    }

    FString key = RootKey(id);
    FString data;

    try
    {
        auto val = redisPtr->get(TCHAR_TO_UTF8(*key));
        if (val)
        {
            data = FString(val->c_str());
        }
        else
        {
            // Fail silently as the caller will log out a warning if applicable.
            return root_object;
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::GetRootObject failed: %S"), e.what());
        return root_object;
    }
    
    auto json_object = GetJsonObject(data);
    
    if (!json_object.IsValid()) 
    {
		UE_LOG(LogKamoDriver, Error, TEXT("FAILED TO GET ROOT OBJECT - INVALID JSON"));
        return root_object;
    }
    
    root_object.id = id;
    root_object.state = data;
	
	// Handler objecs don't have handlers themselves
	if (json_object->HasField("handler"))
	{
		FString handler;
		if (json_object->TryGetStringField("handler", handler))
		{
			root_object.handler_id = KamoID(handler);
		}
	}

    return root_object;
}

TArray<KamoRootObject> KamoRedisDB::FindRootObjects(const FString& class_name) const
{
    // Note: 'class_name' is never used.

    TArray<KamoRootObject> objects;
    
    // Example: ko:live:db:region.map_main_p:
    FString pattern = Key(FString::Printf(TEXT("*~")));

    long long cursor = 0;
    long long count = 1000;

    // Get all keys that match the pattern
    std::vector<std::string> keys;
    do
    {
        cursor = redisPtr->scan(cursor, TCHAR_TO_UTF8(*pattern), count, std::back_inserter(keys));
    } 
    while (cursor != 0);

    if (keys.empty())
    {
        return objects;
    }   

    for (auto i = 0; i < keys.size(); i++)
    {
        KamoID root_id, tmp;
        if (ParseKey(keys[i].c_str(), root_id))
        {
            KamoRootObject root_object = GetRootObject(root_id);
            if (class_name != "" && root_object.id.class_name != class_name) 
            {
                continue;
            }

            objects.Add(root_object);
        }
    }

    return objects;
}

bool KamoRedisDB::SetHandler(const KamoID& id, const KamoID& handler_id) 
{
	UE_LOG(LogKamoDriver, Log, TEXT("KamoRedisDB::SetHandler: '%s' to '%s'"), *id(), *handler_id());

    if (!handler_id.IsEmpty() && lock_id.IsEmpty())
    {
        lock_id = FString::Printf(TEXT("%s - %S"), *handler_id(), sw::redis::RedLockUtils::lock_id().c_str());
    }

	if (handler_id.IsEmpty())		
	{		
		if (!region_locks.Contains(*id()))
		{
			UE_LOG(LogKamoDriver, Warning, TEXT("SetHandler(null): This instance is not registered for %s."), *id());
		}
		else
		{
            // Reset handler reference
            KamoRootObject ob = GetRootObject(id);
            ob.handler_id = KamoID();
            Set(ob);

			// Remove lock
            LockMutex* rlmutex = region_locks.FindAndRemoveChecked(*id());
            rlmutex->Unlock();
            delete rlmutex;
		}
		return true;
	}

    
    KamoRootObject root_ob = GetRootObject(id);    
    
    if (root_ob.IsEmpty()) 
	{
		UE_LOG(LogKamoDriver, Error, TEXT("FAILED TO SET HANDLER - ROOT NOT FOUND"));
        return false;
    }

	// Try lock the root object
    if (!handler_id.IsEmpty() && !region_locks.Contains(*id()))
	{
        FString lock_name = Key("locks:" + id());
        LockMutex* rlmutex = new LockMutex(redisPtr, lock_name, lock_id, region_lock_timeout_value);
        region_locks.Add(*id(), rlmutex);

        if (!rlmutex->Lock())
        {
            return false;
        }
	}

    auto json_object = GetJsonObject(root_ob.state);
    
    if (!json_object.IsValid()) 
    {
		UE_LOG(LogKamoDriver, Error, TEXT("FAILED TO SET HANDLER - INVALID JSON"));
        return false;
    }
    
    json_object->SetStringField("handler", handler_id());    
    auto state = GetJsonString(json_object);    
    return UpdateRootObject(id, state);
}

// Objects

bool KamoRedisDB::AddObject(const KamoID& root_id, const KamoID& id, const FString& state) 
{
    FString key = ChildKey(root_id, id);

    try
    {
        if (!redisPtr->set(TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*state)))
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::AddObject %s failed."), *key);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::AddObject %s failed: %S"), *key, e.what());
        return false;
    }

    return true;
}

bool KamoRedisDB::DeleteObject(const KamoID& id)
{
    KamoID root_id = FindRootIDOfChild(id);
    if (root_id.IsValid())
    {
        try
        {
            return redisPtr->del(TCHAR_TO_UTF8(*(ChildKey(root_id, id)))) == 1;
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::DeleteObject failed: %S"), e.what());
            return false;
        }
    }
    else
    {
        return false;
    }
}


bool KamoRedisDB::UpdateObject(const KamoID& id, const FString& state)
{
    KamoID root_id = FindRootIDOfChild(id);
    if (root_id.IsValid())
    {
        return UpdateChildObject(root_id, id, state);
    }
    else
    {
        return false;
    }
}


KamoChildObject KamoRedisDB::GetObject(const KamoID& id, bool fail_silently) const
{
    KamoChildObject object;
    KamoID root_id = FindRootIDOfChild(id, fail_silently);
    if (!root_id.IsValid())
    {
        return object;
    }

    FString key = ChildKey(root_id, id);
    FString data;

    try
    {
        auto val = redisPtr->get(TCHAR_TO_UTF8(*key));
        if (val)
        {
            data = FString(val->c_str());
        }
        else
        {
            UE_CLOG(!fail_silently, LogKamoDriver, Warning, TEXT("KamoRedisDB::GetObject failed to fetch key: %s"), *key);
            return object;
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::GetObject failed: %S"), e.what());
        return object;
    }

    object.id = id;
    object.state = data;
    object.root_id = root_id;
    
    return object;
}


bool KamoRedisDB::ParseKey(const FString& key, KamoID& root_id, KamoID* child_id) const
{
    // Parse 'key' into root_id, kamo_id and class name
    // Example:
    // ko:live:db:region.map_main_p:player.743
    // root_id=region.map_main_p
    // kamo_id=player.743
    FString prefix = Key();
    if (!key.StartsWith(prefix))
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::ParseKey. Key='%s'. Bad prefix, expected: %s"), *key, *prefix);
        return false;
    }

    FString fullkey = key.Mid(prefix.Len() + 1);
    FString root_key;
    FString child_key;

    if (child_id)
    {
        
        if (!fullkey.Split(TEXT(":"), &root_key, &child_key))
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::ParseKey. Key='%s'. Missing ':' after prefix."), *key);
            return false;
        }
    }
    else
    {
        if (!fullkey.EndsWith("~"))
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::ParseKey. Key='%s'. Expected ~ as terminator for root key."), *key);
            return false;
        }
        root_key = fullkey.Left(fullkey.Len()-1);
    }

    KamoID root(root_key);
    if (root.IsEmpty())
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::ParseKey. Key='%s'. Bad root key."), *key);
        return false;
    }

    root_id = root;

    if (child_id)
    {
        *child_id = KamoID(child_key);
    }

    return true;
}


TArray<KamoChildObject> KamoRedisDB::FindObjects(const KamoID& root_id, const FString& class_name) const
{
    TArray<KamoChildObject> objects;
    FString pattern;

    if (!root_id.IsEmpty())
    {
        // Example: ko:live:db:region.map_main_p:*
        pattern = Key(root_id() + ":*");
    }
    else
    {
        // Example: ko:live:db:region.*:player.*
        pattern = Key(FString::Printf(TEXT("region.*:%s.*"), *class_name));
    }

    long long cursor = 0;
    long long count = 1000;

    // Get all keys that match the pattern
    std::vector<std::string> keys;
    do
    {
        cursor = redisPtr->scan(cursor, TCHAR_TO_UTF8(*pattern), count, std::back_inserter(keys));
    }
    while (cursor != 0);

    if (keys.empty())
    {
        return objects;
    }

    // Get all values that match the key set
    std::vector<std::string> values;
    redisPtr->mget(keys.begin(), keys.end(), std::back_inserter(values));

    if (keys.size() != values.size())
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::FindObjects. Got %i keys but % values. Bailing out."), keys.size(), values.size());
        return objects;
    }

    UE_LOG(LogKamoDriver, Display, TEXT("KamoRedisDB::FindObjects. Got %i keys and values."), keys.size());
    
    for (auto i = 0; i < keys.size(); i++)
    {
        KamoChildObject object;
        if (!ParseKey(keys[i].c_str(), object.root_id, &object.id) || object.id.IsEmpty())
        {
            return objects;
        }

        object.state = values[i].c_str();
        objects.Add(object);
    }

    return objects;
}

bool KamoRedisDB::MoveObject(const KamoID& id, const KamoID& root_id) 
{
    KamoID from_root_id = FindRootIDOfChild(id);
    if (from_root_id.IsEmpty())
    {
        return false;
    }

    FString from_key = ChildKey(from_root_id, id);
    FString to_key = ChildKey(root_id, id);

    try
    {
        redisPtr->rename(TCHAR_TO_UTF8(*from_key), TCHAR_TO_UTF8(*to_key));
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::MoveObject of '%s' to '%s' failed: %S"), *from_key, *to_key, e.what());
        return false;
    }

    UE_LOG(LogKamoDriver, Display, TEXT("KamoRedisDB::MoveObject of '%s' to '%s' succeeded."), *from_key, *to_key);

    return true;
}

// Handler

bool KamoRedisDB::AddHandlerObject(const KamoHandlerObject& handler) {
    auto json_object = GetJsonObject(handler.state);
	if (!json_object)
	{
		UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::AddHandlerObject. Malformed json: %s"), *handler.state);
		return false;
	}
    
    auto current_time = FDateTime::UtcNow().ToIso8601();
    
    json_object->SetStringField("inbox_address", handler.inbox_address);
	json_object->SetStringField("start_time", current_time);
	json_object->SetStringField("last_refresh", current_time);
    
    auto new_state = GetJsonString(json_object);
    
    return AddRootObject(handler.id, new_state);
}

bool KamoRedisDB::DeleteHandlerObject(const KamoID& handler_id) 
{
	// Reset all root objects that are referencing this handler and remove the lock.
	for (auto ob : FindRootObjects(""))
	{
		if (ob.handler_id == handler_id)
		{
			ob.handler_id = KamoID();
			Set(ob);
		}
	}

    return DeleteRootObject(handler_id);
}

bool KamoRedisDB::RefreshHandler(const KamoID& handler_id, const TSharedPtr < FJsonObject >& stats)
{
    auto root_object = GetRootObject(handler_id);
    
    if (root_object.IsEmpty()) {
        return false;
    }
    
    auto json_object = GetJsonObject(root_object.state);
    
    auto current_time = FDateTime::UtcNow().ToIso8601();
    
    json_object->SetStringField("last_refresh", current_time);
    json_object->SetObjectField("stats", stats);
    
    auto new_state = GetJsonString(json_object);

	UE_LOG(LogKamoDriver, VeryVerbose, TEXT("Handler %s heartbeat %s"), *handler_id(), *current_time);
    return UpdateRootObject(handler_id, new_state);
}

KamoHandlerObject KamoRedisDB::GetHandlerInfo(const KamoID& handler_id) const
{
    KamoHandlerObject handler_object;
    
    // See if there's any handler record available
    FString key = RootKey(handler_id);
    FString data;

    try
    {
        if (redisPtr->exists(TCHAR_TO_UTF8(*RootKey(handler_id))) == 0)
        {
            return handler_object;
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::GetHandlerInfo failed: %S"), e.what());
        return handler_object;
    }
        
    auto root_object = GetRootObject(handler_id);
    
    if (root_object.IsEmpty()) {
        return handler_object;
    }
    
    auto json_object = GetJsonObject(root_object.state);
    
    handler_object.id = root_object.id;
    handler_object.state = root_object.state;
    handler_object.inbox_address = json_object->GetStringField("inbox_address");
    
    return handler_object;
}

// Unified

bool KamoRedisDB::Set(const KamoRootObject& object) {
    auto json_object = GetJsonObject(object.state);
    
	if (!object.IsEmpty())
	{
		if (object.handler_id.IsEmpty())
		{
			json_object->SetField("handler", MakeShared<FJsonValueNull>());
		}
		else
		{
			json_object->SetStringField("handler", object.handler_id());
		}		
	}
    
    auto new_state = GetJsonString(json_object);
    
    return AddRootObject(object.id, new_state, true) || UpdateRootObject(object.id, new_state);
}


void KamoRedisDB::DoWork()
{
    // While there are objects to be serialized, pop one from the map that has the highest priority
    for (;;)
    {
        SerializationRecord object;
        object.priority = -1;
        {
            FScopeLock lock(&mutex);
            if (objects_for_serialization.Num() == 0)
            {
                return; 
            }

            for (auto it = objects_for_serialization.CreateIterator(); it; ++it)
            {
                if (it.Value().priority >= object.priority)
                {
                    object = it.Value();
                }
            }
        }
        
        FString key = ChildKey(object.root_id, object.id);
        try
        {
            if (!redisPtr->set(TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*object.state)))
            {
                UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::DoWork failed for %s"), *key);
            }
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::UpdateRootObject failed: %S"), e.what());
        }

        // Always remove the object from the queue even though it failed to write out because we might
        // be in an unrecoverable state with the file writing and thus erroring infinitely.
        {
            FScopeLock lock(&mutex);
            objects_for_serialization.Remove(object.id());
        }
    }    
}


void KamoRedisDB::RefreshRegionLocks()
{
    // Refresh the locks
    for (auto rl : region_locks)
    {
        rl.Value->Refresh();
    }
}


bool KamoRedisDB::Set(const KamoChildObject& object)
{
    SerializationRecord rec = { object.id, object.root_id, object.state, 0 };
    
    {
        FScopeLock lock(&mutex);
        if (objects_for_serialization.Contains(object.id()))
        {
            // Bump priority by one.
            // TODO: Have class based priority jumps, i.e. important stuff gets bumped by 10 while
            // insignificant stuff gets bumped by less.
            // TODO: Add max. age priority as well to prevent starvation.
            rec.priority = objects_for_serialization[object.id()].priority + 1;
        }
        objects_for_serialization.Add(object.id(), rec);
    }

    
    if (serializer.IsDone())
    {
        serializer.StartBackgroundTask(GIOThreadPool);
    }
    else
    {
    }
    
    return true;	
}

bool KamoRedisDB::Set(const KamoHandlerObject& object) {
    auto json_object = GetJsonObject(object.state);
    
    json_object->SetStringField("inbox_address", object.inbox_address);
    
    auto new_state = GetJsonString(json_object);
    
    return AddHandlerObject(object) || UpdateRootObject(object.id, new_state);
}

bool KamoRedisDB::Delete(const KamoID& id) {
    return DeleteRootObject(id) || DeleteObject(id);
}


bool KamoRedisDB::DeleteChildObject(const KamoID& root_id, const KamoID& id)
{
    try
    {
        return redisPtr->del(TCHAR_TO_UTF8(*(ChildKey(root_id, id)))) == 1;
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::DeleteChildObject %s failed: %S"), *ChildKey(root_id, id), e.what());
        return false;
    }
}


TSharedPtr<KamoObject> KamoRedisDB::Get(const KamoID& id) const
{
    TSharedPtr<KamoObject> object_ptr;
    
    auto handler_object = GetHandlerInfo(id);
    
    if (!handler_object.IsEmpty()) {
        object_ptr = MakeShareable(new KamoObject);
        
        object_ptr->id = handler_object.id;
        object_ptr->state = handler_object.state;
        
        return object_ptr;
    }
    
    auto root_object = GetRootObject(id);
    
    if (!root_object.IsEmpty()) {
        object_ptr = MakeShareable(new KamoObject);
        
        object_ptr->id = root_object.id;
        object_ptr->state = root_object.state;
        
        return object_ptr;
    }
    
    auto object = GetObject(id);
    
    if (!object.id.IsEmpty()) {
        object_ptr = MakeShareable(new KamoObject);
        
        object_ptr->id = object.id;
        object_ptr->state = object.state;
        
        return object_ptr;
    }
    
    return object_ptr;
}

// Helpers

bool KamoRedisDB::IsSerializationPending(const KamoID& id, bool bump_priority)
{
    FScopeLock lock(&mutex);
    if (id.IsEmpty())
    {
        // See if any object is pending serialization
        return objects_for_serialization.Num() > 0;
    }
    else if (objects_for_serialization.Contains(id()))
    {
        if (bump_priority)
        {
            SerializationRecord& rec = objects_for_serialization[id()];
            if (rec.priority < 1000000000)
            {
                // TODO: Class based bumpness
                rec.priority += 1000;
            }
        }
        return true;
    }
    return false;
}


bool KamoRedisDB::CancelIfPending(const KamoID& id)
{
    FScopeLock lock(&mutex);
    return objects_for_serialization.Remove(id()) > 0;
}


TSharedPtr<FJsonObject> KamoRedisDB::GetJsonObject(const FString& data) const {
    TSharedPtr<FJsonObject> json_object;
    TSharedRef< TJsonReader<> > json_reader = TJsonReaderFactory<>::Create(*data);
    
    FJsonSerializer::Deserialize(json_reader, json_object);
    
    return json_object;
}

FString KamoRedisDB::GetJsonString(const TSharedPtr<FJsonObject>& json_object) const {
    FString json_string;
    TSharedRef< TJsonWriter<> > json_writer = TJsonWriterFactory<>::Create(&json_string);
    FJsonSerializer::Serialize(json_object.ToSharedRef(), json_writer);
    
    return json_string;
}



FString KamoRedisDB::RootKey(const KamoID& root_id) const
{
    return Key(root_id() + "~");
}


FString KamoRedisDB::ChildKey(const KamoID& root_id, const KamoID& child_id) const
{
    return Key(root_id() + ":" + child_id());
}


KamoID KamoRedisDB::FindRootIDOfChild(const KamoID& child_id, bool fail_silently) const
{
    TArray<KamoChildObject> objects;
    FString pattern;
    KamoID root_id;

    // Example: To search for "player.514" the pattern is: ko:live:db:region.*:player.514
    pattern = Key("region.*:") + child_id();
    long long cursor = 0;
    long long count = 1000;

    // Get all keys that match the pattern
    //std::vector<std::string> keys;
    std::vector<std::string> keys;
    do
    {
        cursor = redisPtr->scan(cursor, TCHAR_TO_UTF8(*pattern), count, std::back_inserter(keys));
    } 
    while (cursor != 0);

    if (!keys.empty())
    {
        if (keys.size() > 1)
        {
            UE_CLOG(!fail_silently, LogKamoDriver, Error, TEXT("KamoRedisDB::FindRootIDOfChild: Object %s found in %i places!"), *child_id(), keys.size());
        }
        KamoID found_child_id;
        ParseKey(keys[0].c_str(), root_id, &found_child_id);
    }
    else
    {
        UE_CLOG(!fail_silently, LogKamoDriver, Error, TEXT("KamoRedisDB::FindRootIDOfChild: Object %s not found!"), *child_id());
    }

    return root_id;
}


bool KamoRedisDB::UpdateChildObject(const KamoID& root_id, const KamoID& child_id, const FString& state)
{
    // Make sure we have the region lock
    if (!region_locks.Contains(root_id()))
    {
        UE_LOG(LogKamoDriver, Warning, TEXT("KamoRedisDB::UpdateChildObject updating %s in region %s but we don't have it locked. THIS MUST BE FIXED!"), *child_id(), *root_id());
        // ...letting this pass for now but this is bad.
    }

    FString key = ChildKey(root_id, child_id);
    try
    {
        if (!redisPtr->set(TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*state)))
        {
            UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::UpdateChildObject failed for %s"), *key);
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogKamoDriver, Error, TEXT("KamoRedisDB::UpdateChildObject failed: %S"), e.what());
    }

    return true;
}

#endif // WITH_REDIS_CLIENT
