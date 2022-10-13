// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.


#include "KamoDriver.h"
#include "KamoRuntimeModule.h"

#include "Misc/Parse.h"
#include "Misc/CommandLine.h"


DEFINE_LOG_CATEGORY(LogKamoDriver);


IKamoDriver::IKamoDriver() : 
	initialized(false) 
{
}


IKamoDriver::~IKamoDriver()
{
	if (initialized)
	{
		UE_LOG(LogKamoDriver, Error, TEXT("IKamoDriver destructor called before CloseSession: tenant=%s, session=%s"), *tenant_name, *session_info);
	}
}


FString IKamoDriver::GetSessionURL() const
{
	KamoURLParts parts;
	parts.scheme = GetScheme();
	parts.type = GetDriverType();
	parts.tenant = tenant_name;
	parts.session_info = session_info;
	return parts.GetURL();
}


bool IKamoDriver::CreateSession(const FString& _tenant_name, const FString& _session_info)
{
	if (initialized)
	{
		UE_LOG(LogKamoDriver, Error, TEXT("CreateSession: Driver is already initialized: %s"), *GetSessionURL());
		return false;
	}

	tenant_name = _tenant_name;
	session_info = _session_info;

	if (tenant_name.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("-tenant="), tenant_name);
	}

	if (tenant_name.IsEmpty())
	{
		UE_LOG(LogKamoDriver, Error, TEXT("%s+%s: Can't create session without tenant name."), *GetScheme(), *GetDriverType());
		return false;
	}
		
	if (!OnSessionCreated())
	{
		return false;
	}

	initialized = true;
	return true;
}


void IKamoDriver::CloseSession()
{
	if (initialized)
	{
		initialized = false;
	}
	else
	{
		UE_LOG(LogKamoDriver, Error, TEXT("%s+%s: CloseSession called on uninitialized session."), *GetScheme(), *GetDriverType());
	}
}


bool KamoURLParts::ParseURL(const FString& url, KamoURLParts& parts)
{
	// URL format:
	// <scheme>+<driver type>:<tenant>[:session info]
	// Note: The logic below is shared with kamo-backend project in module kamo.driver.kamodriver.
	FString schemes;
	FString remainder;
	if (!url.Split(TEXT(":"), &schemes, &remainder))
	{
		UE_LOG(LogKamoDriver, Warning, TEXT("Can't parse url, format is not <scheme>+<driver type>:<tenant>[:session info]. Url: %s"), *url);
		return false;
	}

	if (!schemes.Split(TEXT("+"), &parts.scheme, &parts.type))
	{
		UE_LOG(LogKamoDriver, Warning, TEXT("Can't parse url, format is not <scheme>+<driver type>:<tenant>[:session info]. Url: %s"), *url);
		return false;
	}

	parts.tenant = remainder;
	if (remainder.Contains(TEXT(":")))
	{
		remainder.Split(TEXT(":"), &parts.tenant, &parts.session_info);
	}

	return true;
}


FString KamoURLParts::GetURL() const
{
	if (session_info.IsEmpty())
	{
		return FString::Printf(TEXT("%s+%s:%s"), *scheme, *type, *tenant);
	}
	else
	{
		return FString::Printf(TEXT("%s+%s:%s:%s"), *scheme, *type, *tenant, *session_info);
	}
}


bool IKamoDriver::OnSessionCreated()
{
	return true;
}


void IKamoDriver::Tick(float DeltaTime)
{
}