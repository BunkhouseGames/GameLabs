// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once

#include "KamoDriver.h"



/*
Kamo File Backend driver interface.
*/


class KamoFileDriver : public virtual IKamoDriver {

public:
    
    // IKamoDriver
    FString GetScheme() const override { return "file"; }
    bool OnSessionCreated() override;

    // Utility function for file based drivers, returns a path to ~/.kamo/<tenant>/<driver>
    FString GetHomePath() const;

protected:
    FString session_path;

};
