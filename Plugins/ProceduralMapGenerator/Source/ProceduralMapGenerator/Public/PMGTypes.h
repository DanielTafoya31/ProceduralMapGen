// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PMGTypes.generated.h"

UENUM(BlueprintType)
enum class EMapType : uint8
{
	Interior,
	Exterior,
	Hybrid
};

UENUM(BlueprintType)
enum class EGameGenre : uint8
{
	Shooter,
	Racing,
	Fighting,
	Platformer,
	Exploration,
	Adventure,
	Puzzle,
	Survival,
	Other
};

UENUM(BlueprintType)
enum class EBiomeType : uint8
{
	Forest,
	Desert,
	Mountain,
	Cave,
	City,
	Castle,
	Dungeon,
	Underwater,
	Snow,
	Volcanic,
	Custom
};

UENUM(BlueprintType)
enum class EAssetFunction : uint8
{
	Ground,
	Wall,
	Ceiling,
	Floor,
	Cover,
	Obstacle,
	Decoration,
	Prop,
	Loot,
	SpawnPoint,
	Checkpoint,
	Door,
	Window,
	Light,
	Water,
	Vegetation,
	Rock,
	Building
};

UENUM(BlueprintType)
enum class EMapTopology : uint8
{
	Grid2D,
	Grid3D,
	Graph,
	Voxel,
	Spline
};
