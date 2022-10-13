// Copyright 2021 Arctic Theory ehf.

#pragma once

#include "CoreMinimal.h"

DECLARE_STATS_GROUP(TEXT("Harvestable Fields"), STATGROUP_HarvestableFields, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Refresh"), STAT_HF_Refresh, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("OnSlotChange"), STAT_HF_OnSlotChange, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RefreshAsset"), STAT_HF_RefreshAsset, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("CleanupAsset"), STAT_HF_CleanupAsset, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("PrepareSlots"), STAT_HF_PrepareSlots, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("LineTrace"), STAT_HF_LineTrace, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("UpdateInstanceTransform"), STAT_HF_UpdateInstanceTransform, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("OnCreatePhysicsState"), STAT_HF_OnCreatePhysicsState, STATGROUP_HarvestableFields, );

DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Number of harvestable instances"), STAT_HF_NumHarvestableInstances, STATGROUP_HarvestableFields, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Number of collision bodies"), STAT_HF_NumCollisionBodies, STATGROUP_HarvestableFields, );

// TODO: Check these as they are not used at the moment
DECLARE_CYCLE_STAT_EXTERN(TEXT("Regrow"), STAT_HF_Regrow, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("ExportData"), STAT_HF_ExportData, STATGROUP_HarvestableFields, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("ImportData"), STAT_HF_ImportData, STATGROUP_HarvestableFields, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Number of static slots"), STAT_HF_NumStaticSlots, STATGROUP_HarvestableFields, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Number of dynamic slots"), STAT_HF_NumDynamicSlots, STATGROUP_HarvestableFields, );


DECLARE_LOG_CATEGORY_EXTERN(LogHF, Log, All);

