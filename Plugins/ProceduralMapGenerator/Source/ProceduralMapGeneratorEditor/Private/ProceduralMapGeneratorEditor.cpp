// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProceduralMapGeneratorEditor.h"
#include "SProceduralMapGeneratorWidget.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/AppStyle.h"

const FName FProceduralMapGeneratorEditorModule::ProceduralMapGeneratorTabName(TEXT("ProceduralMapGenerator"));

void FProceduralMapGeneratorEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ProceduralMapGeneratorTabName,
		FOnSpawnTab::CreateRaw(this, &FProceduralMapGeneratorEditorModule::SpawnProceduralMapGeneratorTab))
		.SetDisplayName(NSLOCTEXT("ProceduralMapGeneratorEditor", "TabTitle", "Procedural Map Generator"))
		.SetTooltipText(NSLOCTEXT("ProceduralMapGeneratorEditor", "TabTooltip", "Abre el panel de Procedural Map Generator"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FProceduralMapGeneratorEditorModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProceduralMapGeneratorTabName);
	}
}

TSharedRef<SDockTab> FProceduralMapGeneratorEditorModule::SpawnProceduralMapGeneratorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SProceduralMapGeneratorWidget)
		];
}

IMPLEMENT_MODULE(FProceduralMapGeneratorEditorModule, ProceduralMapGeneratorEditor)
