// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.
#pragma once

#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKamoRt, Log, All);

// Borrowed from VaRest
#define KAMO_FUNC (FString(__FUNCTION__))              // Current Class Name + Function Name where this is called
#define KAMO_LINE (FString::FromInt(__LINE__))         // Current Line Number in the code where this is called
#define KAMO_FUNC_LINE (KAMO_FUNC + "(" + KAMO_LINE + ")") // Current Class and Line Number where this is called!
