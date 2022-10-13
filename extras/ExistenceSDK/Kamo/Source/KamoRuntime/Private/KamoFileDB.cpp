// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoFileDB.h"
#include "KamoRuntimeModule.h"
#include "KamoFileHelper.h"

#include "Misc/DateTime.h"


DECLARE_CYCLE_STAT(TEXT("SaveToFile"), STAT_SaveToFile, STATGROUP_Kamo);


KamoFileDB::KamoFileDB()
{
    serializer.GetTask().file_db = this;
}

KamoFileDB::~KamoFileDB()
{
    serializer.EnsureCompletion(true);
}


void KamoFileDB::CloseSession()
{
    KamoFileDriver::CloseSession();

    // Release all remaining handlers just in case
    TArray<FString> Regions;
    file_locks.GetKeys(Regions);
    for (FString& Region : Regions)
    {
        SetHandler(KamoID(Region), KamoID());
    }
}

// Root objects

bool KamoFileDB::AddRootObject(const KamoID& id, const FString& state, bool ignore_if_exists) 
{
	IPlatformFile& pf = FPlatformFileManager::Get().GetPlatformFile();
    
    auto root_path = session_path + "/" + id();
	auto file_path = GetRootFilePath(id);
    
    UE_LOG(LogKamoRuntime, Display, TEXT("KamoFileDB::AddRootObject: %s"), *root_path);
    
    if (pf.FileExists(*file_path)) {
        UE_CLOG(!ignore_if_exists, LogKamoRuntime, Display, TEXT("KamoFileDB::AddRootObject. Already exists: %s"), *id());
        return false;
    }
    
    if (!pf.CreateDirectoryTree(*root_path)) {
        UE_LOG(LogKamoRuntime, Error, TEXT("KamoFileDB::AddRootObject. CreateDirectory failed for: %s"), *root_path);
        return false;
    }
    
    return FKamoFileHelper::AtomicSaveStringToFile(*state, *file_path);
}

bool KamoFileDB::DeleteRootObject(const KamoID& id) {
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    auto root_path = session_path + "/" + id();

	if (!PlatformFile.DirectoryExists(*root_path)) {
		return false;
	}
    
    return PlatformFile.DeleteDirectoryRecursively(*root_path);
}

bool KamoFileDB::UpdateRootObject(const KamoID& id, const FString& state) {
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    auto file_path = GetRootFilePath(id);
    
    if (!PlatformFile.FileExists(*file_path)) {
		UE_LOG(LogKamoRuntime, Error, TEXT("FAILED TO UPDATE ROOT OBJECT - NOT FOUND"));
        return false;
    }
    
    return FKamoFileHelper::AtomicSaveStringToFile(*state, *file_path);
}

KamoRootObject KamoFileDB::GetRootObject(const KamoID& id) const
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    KamoRootObject root_object;
    
    auto file_path = GetRootFilePath(id);
    
    if (!PlatformFile.FileExists(*file_path)) 
    {
        // Fail silently as the caller will log out a warning if applicable.
        return root_object;
    }
    
    FString data;
    FKamoFileHelper::AtomicLoadFileToString(data, *file_path);
    
    auto json_object = GetJsonObject(data);
    
    if (!json_object.IsValid()) {
		UE_LOG(LogKamoRuntime, Error, TEXT("FAILED TO GET ROOT OBJECT - INVALID JSON"));
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

TArray<KamoRootObject> KamoFileDB::FindRootObjects(const FString& class_name) const
{
    IFileManager& FileManager = IFileManager::Get();

	auto path = session_path + "/*";
    
    TArray<KamoRootObject> root_objects;
    
    TArray<FString> output;
    
    FileManager.FindFiles(output, *path, false, true);
    
    for (auto root_name : output) {
        auto id = KamoID(root_name);
        
        auto root_object = GetRootObject(id);
        
        if (class_name != "" && root_object.id.class_name != class_name) {
            continue;
        }
        
        root_objects.Add(root_object);
    }
    
    return root_objects;
}

bool KamoFileDB::SetHandler(const KamoID& id, const KamoID& handler_id) {
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	UE_LOG(LogKamoRuntime, Log, TEXT("KamoFileDB::SetHandler: '%s' to '%s'"), *id(), *handler_id());

	if (handler_id.IsEmpty())		
	{		
		if (!file_locks.Contains(*id()))
		{
			UE_LOG(LogKamoRuntime, Warning, TEXT("SetHandler(null): This instance is not registered for %s."), *id());
		}
		else
		{
            // Reset handler reference
            KamoRootObject ob = GetRootObject(id);
            ob.handler_id = KamoID();
            Set(ob);
            
            // Remove lock
            const auto lock_handle = file_locks.FindAndRemoveChecked(*id());
            delete lock_handle; // Yes, this is how the file is actually closed.
		}
		return true;
	}

    
    auto file_path = GetRootFilePath(id);
    
    if (!PlatformFile.FileExists(*file_path)) 
	{
		UE_LOG(LogKamoRuntime, Error, TEXT("FAILED TO SET HANDLER - ROOT NOT FOUND"));
        return false;
    }

	// Try lock the root object
    if (!handler_id.IsEmpty() && !file_locks.Contains(*id()))
	{
		auto lock_file = file_path + ".lock";
        const auto lock_handle = FKamoFileHelper::OpenWrite(*lock_file, false, false);
		if (!lock_handle)
		{
			UE_LOG(LogKamoRuntime, Error, TEXT("Can't set handler for %s. Failed to acquire lock file."), *id());
			return false;
		}
		file_locks.Add(*id(), lock_handle);
	}

    FString data;
    FKamoFileHelper::AtomicLoadFileToString(data, *file_path);
    
    auto json_object = GetJsonObject(data);
    
    if (!json_object.IsValid()) {
		UE_LOG(LogKamoRuntime, Error, TEXT("FAILED TO SET HANDLER - INVALID JSON"));
        return false;
    }
    
    json_object->SetStringField("handler", handler_id());
    
    auto state = GetJsonString(json_object);
    
    return UpdateRootObject(id, state);
}

// Objects

bool KamoFileDB::AddObject(const KamoID& root_id, const KamoID& id, const FString& state) 
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    KamoChildObject object;
    
    auto root_path = session_path + "/" + root_id();
	auto file_path = GetFilePath(root_id, id);

	//UE_LOG(LogKamoRuntime, Display, TEXT("ADDING CHILD OBJECT: %s"), *file_path);
    
    if (!PlatformFile.DirectoryExists(*root_path)) {
		UE_LOG(LogKamoRuntime, Error, TEXT("FAILED TO ADD CHILD OBJECT - ROOT DIRECTORY NOT FOUND: '%s'"), *root_path);
        return false;
    }
    
    return FKamoFileHelper::AtomicSaveStringToFile(*state, *file_path);
}

bool KamoFileDB::DeleteObject(const KamoID& id){
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    IFileManager& FileManager = IFileManager::Get();
    
    auto file_path = GetObjectPath(id);
    
    if (file_path == "") {
		UE_LOG(LogKamoRuntime, Error, TEXT("FAILED TO DELETE CHILD OBJECT - NOT FOUND"));
        return false;
    }
    
    return PlatformFile.DeleteFile(*file_path);
}


bool KamoFileDB::UpdateObject(const KamoID& id, const FString& state) 
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	auto file_path = GetObjectPath(id);

	if (file_path == "")
	{
		UE_LOG(LogKamoRuntime, Error, TEXT("FAILED TO UPDATE CHILD OBJECT - NOT FOUND"));
		return false;
	}

	return FKamoFileHelper::AtomicSaveStringToFile(*state, *file_path);
}

KamoChildObject KamoFileDB::GetObject(const KamoID& id, bool fail_silently)  const
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    KamoChildObject object;
    
    auto file_path = GetObjectPath(id);
    
    if (!PlatformFile.FileExists(*file_path)) {
		UE_CLOG(!fail_silently, LogKamoRuntime, Error, TEXT("KamoFileDB::GetObject: Not found: %s"), *id());
        return object;
    }
    
    FString data;
    FKamoFileHelper::AtomicLoadFileToString(data, *file_path);
    
    TArray<FString> split;
    file_path.ParseIntoArray(split, TEXT("/"), true);
    
    auto root_name = split[split.Num() - 2];
    
    object.id = id;
    object.state = data;
    object.root_id = KamoID(root_name);
    
    return object;
}

TArray<KamoChildObject> KamoFileDB::FindObjects(const KamoID& root_id, const FString& class_name) const
{
    IFileManager& FileManager = IFileManager::Get();
    
    TArray<KamoChildObject> objects;
    
    FString ext = ".json";
    
    if (!root_id.IsEmpty()) {
        TArray<FString> object_names;
        auto path = session_path + "/" + root_id();

		FileManager.FindFiles(object_names, *path, *ext);
        
        for (auto object_name : object_names) {
			if (object_name == "_this.json") {
				continue;
			}

			auto id = KamoID(object_name.Replace(TEXT(".json"), TEXT("")));
            
            auto object = GetObject(id);
            
            objects.Add(object);
        }
    }
    else if (class_name != "") {
		auto path = session_path + "/*";

        TArray<FString> root_names;
        FileManager.FindFiles(root_names, *path, false, true);
        
        for (auto root_name : root_names) {
            TArray<FString> object_names;
            
            path = session_path + "/" + root_name;
            
			FileManager.FindFiles(object_names, *path, *ext);
            
            for (auto object_name : object_names) {
                auto id = KamoID(object_name);
                
                if (id.class_name != class_name) {
                    continue;
                }
                
                auto object = GetObject(id);
                
                objects.Add(object);
            }
        }
    }
    
    return objects;
}

bool KamoFileDB::MoveObject(const KamoID& id, const KamoID& root_id) {
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    auto current_path = GetObjectPath(id);
    
    if (current_path == "") {
        return false;
    }
    
    // See if destination root object exists, and if not just create it
    if (!PlatformFile.FileExists(*GetRootFilePath(root_id)))
    {
        if (!AddRootObject(root_id, "{}"))
        {
            UE_LOG(LogKamoRuntime, Error, TEXT("KamoFileDB::MoveObject. Target region doesn't exist and an attempt to create one failed.: %s"), *GetObjectPath(root_id));
            return false;
        }
    }
    auto new_path = GetFilePath(root_id, id);
    
    return PlatformFile.MoveFile(*new_path, *current_path);
}

// Handler

bool KamoFileDB::AddHandlerObject(const KamoHandlerObject& handler) {
    auto json_object = GetJsonObject(handler.state);
	if (!json_object)
	{
		UE_LOG(LogKamoRuntime, Error, TEXT("KamoFileDB::AddHandlerObject. Malformed json: %s"), *handler.state);
		return false;
	}
    
    auto current_time = FDateTime::UtcNow().ToIso8601();
    
    json_object->SetStringField("inbox_address", handler.inbox_address);
	json_object->SetStringField("start_time", current_time);
	json_object->SetStringField("last_refresh", current_time);
    
    auto new_state = GetJsonString(json_object);
    
    return AddRootObject(handler.id, new_state);
}

bool KamoFileDB::DeleteHandlerObject(const KamoID& handler_id) 
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

bool KamoFileDB::RefreshHandler(const KamoID& handler_id, const TSharedPtr < FJsonObject >& stats)
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

	//UE_LOG(LogKamoRuntime, Log, TEXT("Handler %s heartbeat %s"), *handler_id(), *current_time);
    return UpdateRootObject(handler_id, new_state);
}

KamoHandlerObject KamoFileDB::GetHandlerInfo(const KamoID& handler_id) const
{
    KamoHandlerObject handler_object;
    
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

bool KamoFileDB::Set(const KamoRootObject& object) {
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


void KamoFileDB::DoWork()
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

        // write out 'next'
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        auto root_file_path = GetRootFilePath(object.root_id);
        if (!PlatformFile.FileExists(*root_file_path))
        {
            UE_LOG(LogKamoRuntime, Error, TEXT("KamoFileDB::SerializerWorker - Root object not found: '%s'"), *root_file_path);
        }
        else
        {
            auto file_path = GetFilePath(object.root_id, object.id);
            if (!FKamoFileHelper::AtomicSaveStringToFile(*object.state, *file_path))
            {
                UE_LOG(LogKamoRuntime, Error, TEXT("KamoFileDB:SerializerWorker - Failed to write file: '%s'"), *file_path);
            }
        }

        // Always remove the object from the queue even though it failed to write out because we might
        // be in an unrecoverable state with the file writing and thus erroring infinitely.
        {
            FScopeLock lock(&mutex);
            objects_for_serialization.Remove(object.id());
        }
    }
}


bool KamoFileDB::Set(const KamoChildObject& object)
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
        serializer.StartBackgroundTask();
    }
    
    return true;	
}

bool KamoFileDB::Set(const KamoHandlerObject& object) {
    auto json_object = GetJsonObject(object.state);
    
    json_object->SetStringField("inbox_address", object.inbox_address);
    
    auto new_state = GetJsonString(json_object);
    
    return AddHandlerObject(object) || UpdateRootObject(object.id, new_state);
}

bool KamoFileDB::Delete(const KamoID& id) {
    return DeleteRootObject(id) || DeleteObject(id);
}


bool KamoFileDB::DeleteChildObject(const KamoID& root_id, const KamoID& id)
{
    auto file_path = GetFilePath(root_id, id);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.DeleteFile(*file_path);
}


TSharedPtr<KamoObject> KamoFileDB::Get(const KamoID& id) const
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

bool KamoFileDB::IsSerializationPending(const KamoID& id, bool bump_priority)
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


bool KamoFileDB::CancelIfPending(const KamoID& id)
{
    FScopeLock lock(&mutex);
    return objects_for_serialization.Remove(id()) > 0;
}


FString KamoFileDB::GetRootFilePath(const KamoID& id) const {
    return session_path + "/" + id() + "/_this.json";
}

FString KamoFileDB::GetFilePath(const KamoID& root_id, const KamoID& kamo_id) const {
    return session_path + "/" + root_id() + "/" + kamo_id() + ".json";
}

FString KamoFileDB::GetObjectPath(const KamoID& id) const {
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    IFileManager& FileManager = IFileManager::Get();

	auto path = session_path + "/*";
    
    TArray<FString> output;
    
    FileManager.FindFiles(output, *path, false, true);
    
    for (auto root_object : output) {
        auto root_id = KamoID(root_object);
        
        auto file_path = GetFilePath(root_id, id);
        
        if (PlatformFile.FileExists(*file_path)) {
            return file_path;
        }
    }
    
    return "";
}

KamoID KamoFileDB::GetObjectIDFromPath(const FString& path) const {
	auto path_str = path;

	TArray<FString> split;
	path_str.ParseIntoArray(split, TEXT("/"));

	path_str = split[split.Num() - 1];

	path_str = path_str.Replace(TEXT(".json"), TEXT(""));

	return KamoID(path_str);
}

TSharedPtr<FJsonObject> KamoFileDB::GetJsonObject(const FString& data) const {
    TSharedPtr<FJsonObject> json_object;
    TSharedRef< TJsonReader<> > json_reader = TJsonReaderFactory<>::Create(*data);
    
    FJsonSerializer::Deserialize(json_reader, json_object);
    
    return json_object;
}

FString KamoFileDB::GetJsonString(const TSharedPtr<FJsonObject>& json_object) const {
    FString json_string;
    TSharedRef< TJsonWriter<> > json_writer = TJsonWriterFactory<>::Create(&json_string);
    FJsonSerializer::Serialize(json_object.ToSharedRef(), json_writer);
    
    return json_string;
}
