// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once


DECLARE_LOG_CATEGORY_EXTERN(LogRedisClientBP, Log, All);


class REDISCLIENTBP_API FRedisClientBPModule : public IModuleInterface
{
public:


	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
};

