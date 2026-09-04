// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MapBaker.generated.h"

struct FDungeonGrid;
struct FAssetEntry;
class UMapTemplate;
class UAssetCatalog;
class UStaticMesh;
class UWorld;
class AActor;

// FASE 1: instancia StaticMeshActors en el World del editor a partir de un FDungeonGrid ya
// generado y validado, usando el AssetCatalog para resolver que mesh usar en cada celda/room.
// Todo el bake queda envuelto en un FScopedTransaction para soportar Ctrl+Z.
UCLASS()
class PROCEDURALMAPGENERATOREDITOR_API UMapBaker : public UObject
{
	GENERATED_BODY()

public:
	bool BakeToWorld(const FDungeonGrid* Grid, const UMapTemplate* Template, UAssetCatalog* Catalog, UWorld* World);

	// Numero de actors instanciados en la ultima llamada a BakeToWorld (para reportar en la UI).
	int32 GetLastSpawnedActorCount() const { return LastSpawnedActorCount; }

private:
	AActor* SpawnMeshActor(UWorld* World, AActor* AttachParent, UStaticMesh* Mesh, const FTransform& Transform);

	// Busca en el catalogo el mejor asset para una funcion+estilo dados. Si no hay coincidencia
	// de estilo, cae a cualquier asset que cumpla solo la funcion. Selecciona por peso (Weight)
	// usando BakeRNG para mantener el bake determinista respecto al Seed del grid.
	const FAssetEntry* FindBestAsset(UAssetCatalog* Catalog, const FString& FunctionTag, const FString& StyleTag);

	void BakeGroundAndWalls(const FDungeonGrid& Grid, const UMapTemplate& Template, UAssetCatalog* Catalog, UWorld* World, AActor* Container);
	void BakeRoomMarkers(const FDungeonGrid& Grid, const UMapTemplate& Template, UAssetCatalog* Catalog, UWorld* World, AActor* Container);
	void BakeCeilings(const FDungeonGrid& Grid, const UMapTemplate& Template, UAssetCatalog* Catalog, UWorld* World, AActor* Container);

	FRandomStream BakeRNG;
	int32 LastSpawnedActorCount = 0;
};
