// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PMGStructs.h"
#include "AssetCatalog.generated.h"

// Catalogo de assets escaneados desde el Asset Registry, con tagging automatico/manual,
// busqueda por tags, persistencia en JSON y validacion basica.
UCLASS()
class PROCEDURALMAPGENERATOREDITOR_API UAssetCatalog : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AssetCatalog")
	TArray<FAssetEntry> Assets;

	// Escanea recursivamente las carpetas indicadas en busca de UStaticMesh y reconstruye el catalogo.
	void ScanFolders(const TArray<FString>& SourcePaths);

	// Busca assets que tengan todos (bMatchAll=true) o alguno (bMatchAll=false) de los tags dados.
	TArray<FAssetEntry> FindAssetsByTags(const TArray<FString>& Tags, bool bMatchAll) const;

	// Override manual de tags para un asset ya presente en el catalogo (no modifica el asset en disco).
	void SetAssetTags(const FString& AssetName, const TArray<FString>& FunctionTags, const TArray<FString>& StyleTags, const TArray<FString>& GameplayTags);

	bool SaveCatalog() const;
	bool LoadCatalog();

	void ValidateCatalog(TArray<FString>& OutMissingAssets, TArray<FString>& OutDuplicateNames, TArray<FString>& OutUntaggedAssets) const;

	static FString GetCatalogFilePath();

private:
	void AutoTagAsset(FAssetEntry& Entry, const FString& AssetPackagePath) const;
};
