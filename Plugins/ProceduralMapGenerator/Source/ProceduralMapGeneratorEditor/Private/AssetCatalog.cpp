// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetCatalog.h"
#include "ProceduralMapGenerator.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/ARFilter.h"
#include "Engine/StaticMesh.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UAssetCatalog::ScanFolders(const TArray<FString>& SourcePaths)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	for (const FString& Path : SourcePaths)
	{
		Filter.PackagePaths.Add(FName(*Path));
	}

	TArray<FAssetData> FoundAssets;
	AssetRegistry.GetAssets(Filter, FoundAssets);

	Assets.Reset();

	for (const FAssetData& AssetData : FoundAssets)
	{
		FAssetEntry Entry;
		Entry.AssetName = AssetData.AssetName.ToString();
		Entry.Mesh = TSoftObjectPtr<UStaticMesh>(AssetData.GetSoftObjectPath());

		if (AssetData.IsAssetLoaded())
		{
			if (const UStaticMesh* LoadedMesh = Cast<UStaticMesh>(AssetData.GetAsset()))
			{
				Entry.Bounds = LoadedMesh->GetBoundingBox().GetSize();
			}
		}

		AutoTagAsset(Entry, AssetData.PackagePath.ToString());

		Assets.Add(Entry);
	}

	UE_LOG(LogProceduralMapGenerator, Log, TEXT("AssetCatalog: escaneados %d assets en %d carpeta(s)."), Assets.Num(), SourcePaths.Num());
}

void UAssetCatalog::AutoTagAsset(FAssetEntry& Entry, const FString& AssetPackagePath) const
{
	static const TMap<FString, FString> FunctionKeywords =
	{
		{ TEXT("WALL"), TEXT("Wall") },
		{ TEXT("FLOOR"), TEXT("Floor") },
		{ TEXT("GROUND"), TEXT("Ground") },
		{ TEXT("CEILING"), TEXT("Ceiling") },
		{ TEXT("DOOR"), TEXT("Door") },
		{ TEXT("WINDOW"), TEXT("Window") },
		{ TEXT("LIGHT"), TEXT("Light") },
		{ TEXT("WATER"), TEXT("Water") },
		{ TEXT("COVER"), TEXT("Cover") },
		{ TEXT("PROP"), TEXT("Prop") },
		{ TEXT("TREE"), TEXT("Vegetation") },
		{ TEXT("BUSH"), TEXT("Vegetation") },
		{ TEXT("ROCK"), TEXT("Rock") },
		{ TEXT("BUILDING"), TEXT("Building") },
		{ TEXT("LOOT"), TEXT("Loot") },
		{ TEXT("SPAWN"), TEXT("SpawnPoint") },
		{ TEXT("CHECKPOINT"), TEXT("Checkpoint") },
		{ TEXT("OBSTACLE"), TEXT("Obstacle") },
		{ TEXT("DECOR"), TEXT("Decoration") },
	};

	static const TMap<FString, FString> StyleKeywords =
	{
		{ TEXT("FOREST"), TEXT("Forest") },
		{ TEXT("DESERT"), TEXT("Desert") },
		{ TEXT("MOUNTAIN"), TEXT("Mountain") },
		{ TEXT("CAVE"), TEXT("Cave") },
		{ TEXT("CITY"), TEXT("City") },
		{ TEXT("CASTLE"), TEXT("Castle") },
		{ TEXT("DUNGEON"), TEXT("Dungeon") },
		{ TEXT("SNOW"), TEXT("Snow") },
		{ TEXT("VOLCANIC"), TEXT("Volcanic") },
		{ TEXT("MEDIEVAL"), TEXT("Medieval") },
		{ TEXT("SCIFI"), TEXT("SciFi") },
		{ TEXT("NATURE"), TEXT("Nature") },
	};

	const FString NameUpper = Entry.AssetName.ToUpper();
	const FString PathUpper = AssetPackagePath.ToUpper();

	for (const TPair<FString, FString>& Pair : FunctionKeywords)
	{
		if (NameUpper.Contains(Pair.Key) || PathUpper.Contains(Pair.Key))
		{
			Entry.FunctionTags.AddUnique(Pair.Value);
		}
	}

	for (const TPair<FString, FString>& Pair : StyleKeywords)
	{
		if (NameUpper.Contains(Pair.Key) || PathUpper.Contains(Pair.Key))
		{
			Entry.StyleTags.AddUnique(Pair.Value);
		}
	}

	TArray<FString> PathParts;
	AssetPackagePath.ParseIntoArray(PathParts, TEXT("/"), true);
	if (PathParts.Num() > 0)
	{
		Entry.StyleTags.AddUnique(PathParts.Last());
	}
}

TArray<FAssetEntry> UAssetCatalog::FindAssetsByTags(const TArray<FString>& Tags, bool bMatchAll) const
{
	TArray<FAssetEntry> Result;

	for (const FAssetEntry& Entry : Assets)
	{
		TArray<FString> AllTags;
		AllTags.Append(Entry.FunctionTags);
		AllTags.Append(Entry.StyleTags);
		AllTags.Append(Entry.GameplayTags);

		bool bMatches = bMatchAll;
		for (const FString& Tag : Tags)
		{
			const bool bHasTag = AllTags.Contains(Tag);
			bMatches = bMatchAll ? (bMatches && bHasTag) : (bMatches || bHasTag);
		}

		if (bMatches)
		{
			Result.Add(Entry);
		}
	}

	return Result;
}

void UAssetCatalog::SetAssetTags(const FString& AssetName, const TArray<FString>& FunctionTags, const TArray<FString>& StyleTags, const TArray<FString>& GameplayTags)
{
	for (FAssetEntry& Entry : Assets)
	{
		if (Entry.AssetName == AssetName)
		{
			Entry.FunctionTags = FunctionTags;
			Entry.StyleTags = StyleTags;
			Entry.GameplayTags = GameplayTags;
			return;
		}
	}
}

FString UAssetCatalog::GetCatalogFilePath()
{
	return FPaths::ProjectSavedDir() / TEXT("ProceduralMapGenerator") / TEXT("AssetCatalog.json");
}

bool UAssetCatalog::SaveCatalog() const
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	if (!FJsonObjectConverter::UStructToJsonObject(GetClass(), this, JsonObject, 0, 0))
	{
		return false;
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	if (!FJsonSerializer::Serialize(JsonObject, Writer))
	{
		return false;
	}

	return FFileHelper::SaveStringToFile(OutputString, *GetCatalogFilePath());
}

bool UAssetCatalog::LoadCatalog()
{
	FString InputString;
	if (!FFileHelper::LoadFileToString(InputString, *GetCatalogFilePath()))
	{
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	return FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), GetClass(), this, 0, 0);
}

void UAssetCatalog::ValidateCatalog(TArray<FString>& OutMissingAssets, TArray<FString>& OutDuplicateNames, TArray<FString>& OutUntaggedAssets) const
{
	OutMissingAssets.Reset();
	OutDuplicateNames.Reset();
	OutUntaggedAssets.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TMap<FString, int32> NameCounts;

	for (const FAssetEntry& Entry : Assets)
	{
		const bool bExists = Entry.Mesh.IsNull() ? false : AssetRegistry.GetAssetByObjectPath(Entry.Mesh.ToSoftObjectPath()).IsValid();
		if (!bExists)
		{
			OutMissingAssets.Add(Entry.AssetName);
		}

		NameCounts.FindOrAdd(Entry.AssetName)++;

		if (Entry.FunctionTags.Num() == 0 && Entry.StyleTags.Num() == 0 && Entry.GameplayTags.Num() == 0)
		{
			OutUntaggedAssets.Add(Entry.AssetName);
		}
	}

	for (const TPair<FString, int32>& Pair : NameCounts)
	{
		if (Pair.Value > 1)
		{
			OutDuplicateNames.Add(Pair.Key);
		}
	}
}
