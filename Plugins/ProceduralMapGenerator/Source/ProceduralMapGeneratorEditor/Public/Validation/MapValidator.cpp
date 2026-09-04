// Copyright Epic Games, Inc. All Rights Reserved.

#include "MapValidator.h"
#include "Core/MapAbstractData.h"
#include "MapTemplate.h"
#include "Containers/Queue.h"

bool UMapValidator::ValidateDungeon(const FDungeonGrid* Grid, const UMapTemplate* Template, FString& OutError)
{
	OutError.Reset();

	if (!Grid)
	{
		OutError = TEXT("El grid generado es nulo.");
		return false;
	}

	return ValidateStartEndExist(Grid, OutError)
		&& ValidateConnectivity(Grid, OutError)
		&& ValidateRoomSizes(Grid, Template, OutError);
}

bool UMapValidator::ValidateStartEndExist(const FDungeonGrid* Grid, FString& OutError)
{
	if (!Grid || Grid->Rooms.Num() == 0)
	{
		OutError = TEXT("El dungeon no genero ninguna room.");
		return false;
	}

	if (!Grid->Rooms.IsValidIndex(Grid->StartRoomIndex))
	{
		OutError = TEXT("No existe una room de Start valida.");
		return false;
	}

	if (!Grid->Rooms.IsValidIndex(Grid->EndRoomIndex))
	{
		OutError = TEXT("No existe una room de End valida.");
		return false;
	}

	if (Grid->StartRoomIndex == Grid->EndRoomIndex)
	{
		OutError = TEXT("Start y End son la misma room.");
		return false;
	}

	return true;
}

bool UMapValidator::ValidateConnectivity(const FDungeonGrid* Grid, FString& OutError)
{
	if (!Grid || !Grid->Rooms.IsValidIndex(Grid->StartRoomIndex) || !Grid->Rooms.IsValidIndex(Grid->EndRoomIndex))
	{
		OutError = TEXT("No se puede validar conectividad sin Start/End validos.");
		return false;
	}

	const int32 RoomCount = Grid->Rooms.Num();

	TArray<bool> Visited;
	Visited.Init(false, RoomCount);
	Visited[Grid->StartRoomIndex] = true;

	TQueue<int32> BFSQueue;
	BFSQueue.Enqueue(Grid->StartRoomIndex);

	int32 VisitedCount = 1;
	int32 Current = INDEX_NONE;
	while (BFSQueue.Dequeue(Current))
	{
		for (int32 Neighbor : Grid->Rooms[Current].ConnectedRoomIds)
		{
			if (Grid->Rooms.IsValidIndex(Neighbor) && !Visited[Neighbor])
			{
				Visited[Neighbor] = true;
				++VisitedCount;
				BFSQueue.Enqueue(Neighbor);
			}
		}
	}

	if (!Visited[Grid->EndRoomIndex])
	{
		OutError = TEXT("No hay camino desde Start hasta End.");
		return false;
	}

	if (VisitedCount != RoomCount)
	{
		OutError = FString::Printf(TEXT("Existen rooms aisladas: solo %d de %d rooms son alcanzables desde Start."), VisitedCount, RoomCount);
		return false;
	}

	return true;
}

bool UMapValidator::ValidateRoomSizes(const FDungeonGrid* Grid, const UMapTemplate* Template, FString& OutError)
{
	if (!Grid || !Template || Grid->CellSize <= 0)
	{
		return true; // sin datos suficientes para validar tamanos, no bloquear la generacion
	}

	const int32 MinSizeCells = FMath::Max(1, FMath::RoundToInt(Template->SpaceMetrics.MinCorridorWidth / Grid->CellSize));

	for (const FRoomData& Room : Grid->Rooms)
	{
		if (Room.Width() < MinSizeCells || Room.Height() < MinSizeCells)
		{
			OutError = FString::Printf(TEXT("Una o mas rooms son mas pequenas que el minimo permitido (%d celdas)."), MinSizeCells);
			return false;
		}
	}

	return true;
}
