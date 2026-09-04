// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SDockTab;
class FSpawnTabArgs;

class FProceduralMapGeneratorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> SpawnProceduralMapGeneratorTab(const FSpawnTabArgs& SpawnTabArgs);

	static const FName ProceduralMapGeneratorTabName;
};
