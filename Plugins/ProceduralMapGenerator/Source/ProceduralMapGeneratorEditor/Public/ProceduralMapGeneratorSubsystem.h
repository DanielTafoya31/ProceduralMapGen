// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Core/MapAbstractData.h" // FASE 1
#include "ProceduralMapGeneratorSubsystem.generated.h"

class UAssetCatalog;
class UMapTemplate;

// Punto central del editor: mantiene el catalogo de assets, cachea los templates disponibles
// y guarda el estado de generacion actual (template/seed activos).
UCLASS()
class PROCEDURALMAPGENERATOREDITOR_API UProceduralMapGeneratorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "ProceduralMapGenerator")
	UAssetCatalog* GetAssetCatalog() const { return AssetCatalog; }

	UFUNCTION(BlueprintCallable, Category = "ProceduralMapGenerator")
	void ScanAssets(const TArray<FString>& SourcePaths);

	UFUNCTION(BlueprintCallable, Category = "ProceduralMapGenerator")
	void RefreshTemplateCache();

	const TArray<TObjectPtr<UMapTemplate>>& GetAvailableTemplates() const { return CachedTemplates; }

	// FASE 1: fachada de generacion. Corre el algoritmo BSP y valida el resultado; el grid se
	// devuelve (valido o no) para que la UI pueda previsualizarlo, y OutError describe el primer
	// check de validacion que fallo (vacio si el dungeon es jugable).
	TSharedPtr<FDungeonGrid> GeneratePreview(const UMapTemplate* Template, int32 Seed, FString& OutError);

	// FASE 1: instancia en el nivel actual del editor el grid dado (o, si es nullptr, el ultimo
	// grid generado por GeneratePreview).
	bool BakeMap(const FDungeonGrid* Grid, const UMapTemplate* Template);

	TSharedPtr<FDungeonGrid> GetLastGeneratedGrid() const { return LastGeneratedGrid; } // FASE 1
	int32 GetLastBakedActorCount() const { return LastBakedActorCount; } // FASE 1

	UPROPERTY()
	TObjectPtr<UMapTemplate> ActiveTemplate = nullptr;

	UPROPERTY()
	int32 ActiveSeed = 0;

private:
	UPROPERTY()
	TObjectPtr<UAssetCatalog> AssetCatalog = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UMapTemplate>> CachedTemplates;

	// FASE 1
	TSharedPtr<FDungeonGrid> LastGeneratedGrid;
	int32 LastBakedActorCount = 0;
};
