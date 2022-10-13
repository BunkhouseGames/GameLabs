// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "RedisClientModule.h"
#include "Redis++Library.inl"

DEFINE_LOG_CATEGORY(LogRedisClientModule);


void FRedisClientModule::StartupModule()
{
}

void FRedisClientModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FRedisClientModule, RedisClient)
