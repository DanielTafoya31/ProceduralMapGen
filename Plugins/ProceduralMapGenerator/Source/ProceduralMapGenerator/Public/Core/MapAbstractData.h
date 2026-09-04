// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PMGTypes.h"

// FASE 1: representacion abstracta de un dungeon generado (grid + rooms), independiente de
// cualquier algoritmo concreto. Son structs C++ simples (sin reflexion) porque solo se usan
// en tiempo de editor/generacion, nunca se serializan como propiedades de un UObject.

enum class ERoomType : uint8
{
	None,
	Start,      // Punto de inicio del jugador
	End,        // Meta/objetivo final
	Normal,     // Room comun
	Boss,       // Room grande, cerca del final
	Loot,       // Room pequena en rama secundaria
	Corridor    // Pasillo conector
};

struct PROCEDURALMAPGENERATOR_API FMapCell
{
	int32 X = 0;
	int32 Y = 0;
	EAssetFunction CellType = EAssetFunction::Ground;
	bool bIsRoom = false;      // true si pertenece a una room, false si es corridor o vacio
	int32 RoomId = -1;         // Indice en el array de Rooms, -1 si no aplica

	// NOTA: EAssetFunction (Fase 0) no tiene un valor "Empty", asi que el vacio/solido se
	// rastrea aparte con este flag. bIsEmpty = true significa "todavia no tallado" (ni Ground
	// ni Wall); se pone en false al tallar una room/corridor o al convertir la celda en Wall.
	bool bIsEmpty = true;
};

struct PROCEDURALMAPGENERATOR_API FRoomData
{
	FIntRect Bounds = FIntRect(0, 0, 0, 0); // X1,Y1,X2,Y2 en celdas
	ERoomType Type = ERoomType::None;
	TArray<int32> ConnectedRoomIds;
	FVector WorldCenter = FVector::ZeroVector; // Calculado al hacer bake

	int32 Area() const { return (Bounds.Max.X - Bounds.Min.X) * (Bounds.Max.Y - Bounds.Min.Y); }
	int32 Width() const { return Bounds.Max.X - Bounds.Min.X; }
	int32 Height() const { return Bounds.Max.Y - Bounds.Min.Y; }

	FIntPoint GridCenter() const
	{
		return FIntPoint((Bounds.Min.X + Bounds.Max.X) / 2, (Bounds.Min.Y + Bounds.Max.Y) / 2);
	}
};

struct PROCEDURALMAPGENERATOR_API FDungeonGrid
{
	int32 Width = 0;
	int32 Height = 0;
	int32 CellSize = 200;      // Unidades Unreal por celda (default 200 uu)
	TArray<FMapCell> Cells;    // Size = Width * Height. Index = Y * Width + X
	TArray<FRoomData> Rooms;
	int32 StartRoomIndex = -1;
	int32 EndRoomIndex = -1;
	int32 Seed = 0;

	void Init(int32 InWidth, int32 InHeight, int32 InCellSize)
	{
		Width = InWidth;
		Height = InHeight;
		CellSize = InCellSize;

		Cells.Reset(Width * Height);
		Cells.SetNum(Width * Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				FMapCell& Cell = Cells[Y * Width + X];
				Cell.X = X;
				Cell.Y = Y;
			}
		}

		Rooms.Reset();
		StartRoomIndex = -1;
		EndRoomIndex = -1;
	}

	bool IsValidCoord(int32 X, int32 Y) const
	{
		return X >= 0 && Y >= 0 && X < Width && Y < Height;
	}

	FMapCell* GetCell(int32 X, int32 Y)
	{
		return IsValidCoord(X, Y) ? &Cells[Y * Width + X] : nullptr;
	}

	const FMapCell* GetCell(int32 X, int32 Y) const
	{
		return IsValidCoord(X, Y) ? &Cells[Y * Width + X] : nullptr;
	}
};
