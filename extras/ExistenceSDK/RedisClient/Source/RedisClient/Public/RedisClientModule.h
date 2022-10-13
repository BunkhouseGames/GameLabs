// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogRedisClientModule, Log, All);


class REDISCLIENT_API FRedisClientModule : public IModuleInterface
{
public:


	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
};

