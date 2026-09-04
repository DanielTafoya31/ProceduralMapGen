// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

PROCEDURALMAPGENERATOR_API DECLARE_LOG_CATEGORY_EXTERN(LogProceduralMapGenerator, Log, All);

class FProceduralMapGeneratorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
