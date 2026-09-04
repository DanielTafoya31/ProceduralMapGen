// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MapValidator.generated.h"

struct FDungeonGrid;
class UMapTemplate;

// FASE 1: valida que un FDungeonGrid generado sea jugable (start/end existen, todo es
// alcanzable desde Start, las rooms cumplen el tamano minimo). Los tres checks individuales son
// publicos (no solo el agregado ValidateDungeon) para que la UI pueda mostrar un checklist
// detallado en el panel de Preview en vez de un unico booleano.
UCLASS()
class PROCEDURALMAPGENERATOREDITOR_API UMapValidator : public UObject
{
	GENERATED_BODY()

public:
	bool ValidateDungeon(const FDungeonGrid* Grid, const UMapTemplate* Template, FString& OutError);

	bool ValidateStartEndExist(const FDungeonGrid* Grid, FString& OutError);
	bool ValidateConnectivity(const FDungeonGrid* Grid, FString& OutError); // BFS desde Start
	bool ValidateRoomSizes(const FDungeonGrid* Grid, const UMapTemplate* Template, FString& OutError);
};
