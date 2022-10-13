// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#ifndef REDISPLUSLUS_LIBRARY_INL
#define REDISPLUSLUS_LIBRARY_INL

#if defined(_MSC_VER)
#pragma warning(disable: 4101) // '?': unreferenced local variable
#pragma warning(disable: 4458) // declaration of '?' hides class member
#pragma warning(disable: 4005) // 'TEXT': macro redefinition
#include "Windows/MinWindows.h"
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wshadow"
#endif

#include <redis++/transaction.cpp>
#include <redis++/command_options.cpp>
#include <redis++/subscriber.cpp>
#include <redis++/command.cpp>
#include <redis++/shards_pool.cpp>
#include <redis++/connection_pool.cpp>
#include <redis++/shards.cpp>
#include <redis++/crc16.cpp>
#include <redis++/errors.cpp>
#include <redis++/sentinel.cpp>
#include <redis++/connection.cpp>
#include <redis++/pipeline.cpp>
#include <redis++/reply.cpp>
#include <redis++/redis_cluster.cpp>
#include <redis++/redis.cpp>
#include <redis++/recipes/redlock.cpp>

#endif // REDISPLUSLUS_LIBRARY_INL
