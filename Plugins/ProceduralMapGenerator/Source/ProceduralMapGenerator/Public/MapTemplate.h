// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PMGTypes.h"
#include "PMGStructs.h"
#include "MapTemplate.generated.h"

class UMapGenAlgorithm;

// Data Asset que describe todos los parametros necesarios para generar un mapa: genero, bioma,
// topologia, metricas de espacio, reglas de assets/gameplay y tamano del mapa.
UCLASS(BlueprintType)
class PROCEDURALMAPGENERATOR_API UMapTemplate : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	FString TemplateName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	EMapType MapType = EMapType::Exterior;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	EGameGenre Genre = EGameGenre::Exploration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	EBiomeType Biome = EBiomeType::Forest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	EMapTopology Topology = EMapTopology::Grid2D;

	// Placeholder por ahora: la implementacion de algoritmos concretos llega en la Fase 1.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	TSubclassOf<UMapGenAlgorithm> AlgorithmClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	FSpaceMetrics SpaceMetrics;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	FAssetRules AssetRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	FGameplayRules GameplayRules;

	// Tamano del mapa en celdas/unidades logicas.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	FVector2D MapSize = FVector2D(50.0, 50.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapTemplate")
	int32 DefaultSeed = 0;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
