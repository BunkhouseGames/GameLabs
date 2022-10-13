// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "KamoFileMQ.h"
#include "KamoFileHelper.h"

#include "KamoFileMQ.h"
#include "HAL/PlatformFileManager.h"


KamoFileMQ::KamoFileMQ()
{
}


KamoFileMQ::~KamoFileMQ()
{
}


void KamoFileMQ::CloseSession()
{
	KamoFileDriver::CloseSession();
}

bool KamoFileMQ::CreateMessageQueue() 
{   
	if (!DeleteMessageQueue())
	{
        return false;
	}

	auto path = session_path;
	path /= session_info;
	IPlatformFile& pf = FPlatformFileManager::Get().GetPlatformFile();

	if (!pf.CreateDirectoryTree(*path))
	{
		UE_LOG(LogKamoDriver, Error, TEXT("Cannot create directory: %s"), *path);
		return false;
	}
    
    return true;
}


bool KamoFileMQ::DeleteMessageQueue()
{
    auto path = session_path;
    path /= session_info;

	// This is to guard against wiping the file system clean
	if (session_path.IsEmpty() || session_info.IsEmpty() || path.Len() < 5)
	{
		UE_LOG(LogKamoDriver, Error, TEXT("Cannot create directory, base path or name is invalid: %s"), *path);
		return false;
	}

	IPlatformFile& pf = FPlatformFileManager::Get().GetPlatformFile();

	if (pf.FileExists(*path))
	{
		UE_LOG(LogKamoDriver, Error, TEXT("Cannot create directory, target is a file: %s"), *path);
		return false;
	}

	if (pf.DirectoryExists(*path))
	{
		if (!pf.DeleteDirectoryRecursively(*path))
		{
			UE_LOG(LogKamoDriver, Error, TEXT("Cannot delete directory: %s"), *path);
			return false;
		}
	}

	return true;
}


bool KamoFileMQ::SendMessage(const FString& inbox_address, const FString& message_type, const FString& payload)
{
    FString key;
    if (inbox_address.IsEmpty())
    {
        key = session_info;
    }
    else
    {
        KamoURLParts parts;
        if (!KamoURLParts::ParseURL(inbox_address, parts))
        {
            return false;
        }
        key = parts.session_info;
    }

    FString inbox_path = session_path + "/" + key;

    if (message_type == "command") {
        IPlatformFile& pf = FPlatformFileManager::Get().GetPlatformFile();
        
        if (!pf.DirectoryExists(*inbox_path)) {
            return false;
        }
        
        auto guid = FGuid::NewGuid().ToString();
        auto current_time = FDateTime::UtcNow().ToIso8601().Replace(TEXT(":"), TEXT("-"));;
        
        auto file_name = current_time + "_" + guid.Left(8) + ".json";
        
        auto file_path = inbox_path + "/" + file_name;
        
        return FKamoFileHelper::AtomicSaveStringToFile(payload, *file_path);
    }
    
    return false;
}

bool KamoFileMQ::ReceiveMessage(KamoMessage& message)
{
    IPlatformFile& pf = FPlatformFileManager::Get().GetPlatformFile();
    TArray<FString> file_paths;
    FString ext = ".json";
    FString inbox_path = session_path + "/" + session_info;
    
    pf.FindFiles(file_paths, *inbox_path, *ext);

    if (file_paths.Num() == 0) 
    {
        return false;
    }
    
    auto file_path = file_paths[0];
    
    FString data;
    FKamoFileHelper::AtomicLoadFileToString(data, *file_path);
    
    message.message_type = "command";
    message.payload = data;
    
    pf.DeleteFile(*file_path);
    
    return true;
}
