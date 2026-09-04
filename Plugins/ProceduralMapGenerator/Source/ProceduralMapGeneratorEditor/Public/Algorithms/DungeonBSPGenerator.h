// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MapGenAlgorithm.h"
#include "Core/MapAbstractData.h"
#include "DungeonBSPGenerator.generated.h"

class UMapTemplate;

// FASE 1: implementacion de UMapGenAlgorithm que genera dungeons de interior mediante
// particionamiento espacial binario (BSP): divide el area en hojas, coloca una room por hoja,
// conecta las rooms con un Minimum Spanning Tree + un porcentaje de aristas extra (loops),
// asigna tipos de room (Start/End/Boss/Loot/Normal) y finalmente talla las paredes.
UCLASS()
class PROCEDURALMAPGENERATOREDITOR_API UDungeonBSPGenerator : public UMapGenAlgorithm
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FDungeonGrid> Generate(const UMapTemplate* Template, int32 Seed) override;

private:
	// Nodo interno del arbol BSP. Es un struct privado de implementacion, no necesita reflexion.
	struct FBSPNode
	{
		FIntRect Bounds = FIntRect(0, 0, 0, 0);
		TUniquePtr<FBSPNode> Left;
		TUniquePtr<FBSPNode> Right;
		int32 RoomIndex = INDEX_NONE;

		bool IsLeaf() const { return !Left.IsValid() && !Right.IsValid(); }
	};

	struct FRoomEdge
	{
		int32 RoomA = INDEX_NONE;
		int32 RoomB = INDEX_NONE;
		float DistanceSq = 0.f;
	};

	// Paso A: division BSP recursiva. Deja de dividir un nodo si es muy pequeno, se alcanzo la
	// profundidad maxima, ya se alcanzo el presupuesto de rooms (MaxLeaves) o un chance aleatorio
	// lo decide asi (variedad). Ver Generate() para la decision de diseno sobre MaxRooms.
	void SplitNode(FBSPNode& Node, int32 MinLeafSize, int32 MaxDepth, int32 CurrentDepth, int32 MaxLeaves, int32& LeafCounter, FRandomStream& RNG) const;
	void CollectLeaves(FBSPNode& Node, TArray<FBSPNode*>& OutLeaves) const;

	// Paso B: crea una FRoomData dentro de cada hoja, con margen interior y tamano aleatorio.
	void CreateRoomsFromLeaves(const TArray<FBSPNode*>& Leaves, FDungeonGrid& Grid, const UMapTemplate* Template, FRandomStream& RNG) const;

	// Paso C: MST (Prim) sobre los centros de room + aristas extra para loops; talla corridors en L.
	void ConnectRooms(FDungeonGrid& Grid, const UMapTemplate* Template, FRandomStream& RNG) const;
	void CarveCorridor(FDungeonGrid& Grid, const FIntPoint& From, const FIntPoint& To, int32 CorridorWidthCells, FRandomStream& RNG) const;
	void CarveLine(FDungeonGrid& Grid, const FIntPoint& From, const FIntPoint& To, int32 CorridorWidthCells) const;

	// Paso D: Start / End / Boss / Loot / Normal segun la topologia del grafo de conectividad.
	void AssignRoomTypes(FDungeonGrid& Grid, FRandomStream& RNG) const;

	// Paso E: cualquier celda vacia adyacente (4-way) a una celda tallada se convierte en Wall.
	void MarkWalls(FDungeonGrid& Grid) const;
};
