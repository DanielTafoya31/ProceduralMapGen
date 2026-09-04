// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProceduralMapGeneratorSubsystem.h"
#include "AssetCatalog.h"
#include "MapTemplate.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

// FASE 1
#include "Algorithms/DungeonBSPGenerator.h"
#include "Validation/MapValidator.h"
#include "Core/MapBaker.h"
#include "Editor.h"
#include "Engine/World.h"

void UProceduralMapGeneratorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	AssetCatalog = NewObject<UAssetCatalog>(this, TEXT("AssetCatalog"));
	AssetCatalog->LoadCatalog();

	RefreshTemplateCache();
}

void UProceduralMapGeneratorSubsystem::Deinitialize()
{
	if (AssetCatalog)
	{
		AssetCatalog->SaveCatalog();
	}

	Super::Deinitialize();
}

void UProceduralMapGeneratorSubsystem::ScanAssets(const TArray<FString>& SourcePaths)
{
	if (AssetCatalog)
	{
		AssetCatalog->ScanFolders(SourcePaths);
		AssetCatalog->SaveCatalog();
	}
}

void UProceduralMapGeneratorSubsystem::RefreshTemplateCache()
{
	CachedTemplates.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> FoundAssets;
	AssetRegistry.GetAssetsByClass(UMapTemplate::StaticClass()->GetClassPathName(), FoundAssets, true);

	for (const FAssetData& AssetData : FoundAssets)
	{
		if (UMapTemplate* Template = Cast<UMapTemplate>(AssetData.GetAsset()))
		{
			CachedTemplates.Add(Template);
		}
	}
}

// FASE 1
TSharedPtr<FDungeonGrid> UProceduralMapGeneratorSubsystem::GeneratePreview(const UMapTemplate* Template, int32 Seed, FString& OutError)
{
	OutError.Reset();
	LastGeneratedGrid.Reset();

	if (!Template)
	{
		OutError = TEXT("No hay un template seleccionado.");
		return nullptr;
	}

	UDungeonBSPGenerator* Generator = NewObject<UDungeonBSPGenerator>(this);
	TSharedPtr<FDungeonGrid> Grid = Generator->Generate(Template, Seed);

	if (!Grid.IsValid())
	{
		OutError = TEXT("El generador no produjo un resultado valido.");
		return nullptr;
	}

	LastGeneratedGrid = Grid;

	UMapValidator* Validator = NewObject<UMapValidator>(this);
	FString ValidationError;
	if (!Validator->ValidateDungeon(Grid.Get(), Template, ValidationError))
	{
		OutError = ValidationError;
	}

	return Grid;
}

// FASE 1
bool UProceduralMapGeneratorSubsystem::BakeMap(const FDungeonGrid* Grid, const UMapTemplate* Template)
{
	const FDungeonGrid* GridToBake = Grid ? Grid : LastGeneratedGrid.Get();
	if (!GridToBake || !Template || !AssetCatalog || !GEditor)
	{
		return false;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return false;
	}

	UMapBaker* Baker = NewObject<UMapBaker>(this);
	const bool bSuccess = Baker->BakeToWorld(GridToBake, Template, AssetCatalog, EditorWorld);
	LastBakedActorCount = bSuccess ? Baker->GetLastSpawnedActorCount() : 0;

	return bSuccess;
}
