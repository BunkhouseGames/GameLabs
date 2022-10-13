// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_STATS_GROUP(TEXT("Kamo"), STATGROUP_Kamo, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("KamoAsync"), STATGROUP_KamoAsync, STATCAT_Advanced);
DEFINE_LOG_CATEGORY_STATIC(LogKamoStruct, Log, All);


struct KamoID
{
    KamoID() {
        class_name = "";
        unique_id = "";
    }


    KamoID(FString id_string) 
    {
        FString left;
        FString right;

        id_string.Split(".", &left, &right);

        class_name = left;
        unique_id = right;

        if (!IsValid())
        {
            UE_LOG(LogKamoStruct, Error, TEXT("KamoID: Incorrect format for Kamo ID: %s"), *operator()());
        }
    }


    bool IsValid() const
    {
        bool valid;

        if (IsEmpty())
        {
            valid = true;
        }
        else
        {
            int32 index;
            FString kamo_id = operator()();
            valid = (
                kamo_id == kamo_id.ToLower()
                && kamo_id.FindChar('.', index)
                && !kamo_id.FindChar(' ', index)
                );
        }

        return valid;
    }

    FString class_name;
    FString unique_id;
    FString operator () () const 
	{
		if (IsEmpty())
		{
			return FString(TEXT("(unset)"));
		}
		else
		{
			return *FString::Printf(TEXT("%s.%s"), *class_name, *unique_id);
		}
	}

    bool IsEmpty() const { return unique_id == "" || class_name == ""; }

    bool operator==(const KamoID& other) const
    {
        return (class_name == other.class_name && unique_id == other.unique_id);
    }

    bool operator!=(const KamoID& other) const
    {
        return (class_name != other.class_name || unique_id != other.unique_id);
    }
};

// Enable this object as a key inside hashing containers.
uint32 inline GetTypeHash(const KamoID&) { return 0; }


struct KamoObject
{
    KamoID id;
    FString state;

    bool IsEmpty() const { return id.IsEmpty(); }
};

struct KamoRootObject : KamoObject
{
    KamoID handler_id;
};

struct KamoChildObject : KamoObject
{
    KamoID root_id;
};

struct KamoHandlerObject : KamoObject
{
    FString inbox_address;
};

struct KamoMessage
{
    KamoID sender;
    FString message_type;
    FString payload;
};

struct UE4ServerHandler : KamoHandlerObject
{
    FString ip_address;
    int port;
    FString map_name;
    FString secret;
};