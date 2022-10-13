// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#pragma warning(disable: 4250)

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKamoDriver, Log, All);


/*
Kamo Backend driver interface.
*/

struct KamoURLParts
{
    FString scheme;
    FString type;
    FString tenant;
    FString session_info;

    // URL format:
    // <scheme>+<driver type>:<tenant>[:session info]

    static bool ParseURL(const FString& url, KamoURLParts& parts);
    FString GetURL() const;
};

class KAMORUNTIME_API IKamoDriver 
{
 
protected:

    FString tenant_name;
    FString session_info;  // Optional driver agnostic session info.
    bool initialized;

public:
    IKamoDriver();
    virtual ~IKamoDriver();
    
    // Type info
    virtual FString GetDriverType() const = 0;
    virtual FString GetScheme() const = 0;    
    FString GetSessionURL() const;

    // Session handling
    bool CreateSession(const FString& _tenant_name, const FString& _session_info=FString());
    virtual void CloseSession();
    virtual bool OnSessionCreated();
    virtual void Tick(float DeltaTime);

    // Create helpers
    template <typename T>
    static TUniquePtr<T> CreateDriverFromURL(const FString& url)
    {
        KamoURLParts parts;
        if (!KamoURLParts::ParseURL(url, parts))
        {
            return nullptr;
        }
        TUniquePtr<T> driver = T::CreateDriver(parts.scheme);
        if (driver == nullptr)
        {
            return nullptr;
        }
        if (!driver->CreateSession(parts.tenant, parts.session_info))
        {
            return nullptr;
        }
        return driver;
    }
};
