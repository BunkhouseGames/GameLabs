// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "KamoDriver.h"

#include "CoreMinimal.h"

#if WITH_REDIS_CLIENT
#include <redis++/redis++.h>


#pragma warning(disable: 4458) // declaration of '?' hides class member
#pragma warning(disable: 4101) // '?': unreferenced local variable

#if defined(__clang__)
    #pragma clang diagnostic ignored "-Wshadow"
#endif


//#pragma warning(disable: 2011) //  'sw::redis::tls::TlsOptions' : 'struct' type redefinition
//#pragma warning(disable: 2084) //  function 'bool sw::redis::tls::enabled(const sw::redis::tls::TlsOptions &)' already has a body
//#pragma warning(disable: 2027) //  use of undefined type 'sw::redis::tls::TlsOptions'
//#pragma warning(disable: 2371) //  'sw::redis::tls::TlsContextUPtr' : redefinition; different basic typese of undefined type 'sw::redis::tls::TlsOptions'
//#pragma warning(disable: 2660) //  'redisCreateSSLContext' : function does not take 2 arguments
//#pragma warning(disable: 2678) //  binary '!' : no operator found which takes a left - hand operand of type 'sw::redis::tls::TlsContextUPtr' (or there is no acceptable conversion)

/*
Kamo Redis Backend driver interface.
*/

#define CHECK_REDIS() do {if (RedisContext == nullptr) {UE_LOG(); return false;} } while false


// FString to char* helper
class Str : public sw::redis::StringView
{
public:
    char buffer[256];
    Str(const FString& string)
    {

    }

};


typedef std::shared_ptr<sw::redis::Redis> RedisSP;


class KamoRedisDriver : public virtual IKamoDriver {

public:

    KamoRedisDriver();
    
    // IKamoDriver
    FString GetScheme() const override { return "redis"; }
    bool OnSessionCreated() override;
    void CloseSession() override;

    // Implement namespace for redis keys.
    // Format: ko:<tenant>:<scheme>[:session info]
    FString Key(const FString& key = FString()) const;
    static FString Key(const KamoURLParts& parts, const FString& key = FString());

protected:

    RedisSP redisPtr;
    RedisSP CreateRedisInstance(const sw::redis::ConnectionOptions& connectionOptions, const sw::redis::ConnectionPoolOptions& poolOptions) const;
};

#endif // WITH_REDIS_CLIENT
