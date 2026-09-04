// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PMGTypes.h"
#include "PMGStructs.generated.h"

class UStaticMesh;

// Metricas de espacio configurables por genero de juego (pasillos, coberturas, pistas, saltos, etc.)
USTRUCT(BlueprintType)
struct PROCEDURALMAPGENERATOR_API FSpaceMetrics
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Ceiling")
	float MinCeilingHeight = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Ceiling")
	float MaxCeilingHeight = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Corridor")
	float MinCorridorWidth = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Corridor")
	float MaxCorridorWidth = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Cover")
	float CoverHeight = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Cover")
	float CoverDensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Racing")
	float TrackWidth = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Racing")
	float MinCurveRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Fighting")
	float ArenaSize = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Fighting")
	int32 PlatformCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Platformer")
	float MaxJumpDistance = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Platformer")
	float MaxJumpHeight = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Exploration")
	float POIDensity = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceMetrics|Exploration")
	float ResourceDensity = 0.1f;
};

// Entrada del catalogo de assets: que ES el asset, para que bioma sirve y que HACE en gameplay.
USTRUCT(BlueprintType)
struct PROCEDURALMAPGENERATOR_API FAssetEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	FString AssetName;

	// Que ES: "Ground", "Wall", "Cover", etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	TArray<FString> FunctionTags;

	// Para que bioma: "Medieval", "SciFi", "Nature", etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	TArray<FString> StyleTags;

	// Que HACE: "Climbable", "Breakable", "SpawnPoint", etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	TArray<FString> GameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	FVector Bounds = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	bool bCanRotate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	bool bCanScale = false;

	// Probabilidad relativa de seleccion dentro de su categoria.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetEntry")
	float Weight = 1.0f;
};

// Reglas de asignacion/escaneo de assets para un template.
USTRUCT(BlueprintType)
struct PROCEDURALMAPGENERATOR_API FAssetRules
{
	GENERATED_BODY()

	// Carpetas a escanear (paths de Content Browser, ej: /Game/Environments/CastlePack).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetRules")
	TArray<FString> SourcePaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetRules")
	TArray<EBiomeType> CompatibleBiomes;

	// Minimo de assets necesarios por categoria para considerar el catalogo valido.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetRules")
	float MinimumAssetCount = 5.0f;
};

// Reglas de gameplay que debe cumplir el mapa generado.
USTRUCT(BlueprintType)
struct PROCEDURALMAPGENERATOR_API FGameplayRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayRules")
	int32 MinRooms = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayRules")
	int32 MaxRooms = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayRules")
	bool bRequireStartPoint = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayRules")
	bool bRequireEndPoint = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayRules")
	bool bRequireConnectivity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayRules", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Difficulty = 0.5f;
};
