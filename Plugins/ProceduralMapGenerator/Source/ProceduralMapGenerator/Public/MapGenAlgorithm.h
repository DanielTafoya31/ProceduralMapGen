// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/MapAbstractData.h" // FASE 1
#include "MapGenAlgorithm.generated.h"

class UMapTemplate; // FASE 1

// Base de los algoritmos de generacion. La Fase 0 solo tenia el placeholder vacio;
// la Fase 1 anade el contrato Generate() que implementan los algoritmos concretos (ej. BSP).
UCLASS(Abstract, BlueprintType)
class PROCEDURALMAPGENERATOR_API UMapGenAlgorithm : public UObject
{
	GENERATED_BODY()

public:
	// FASE 1: genera la estructura abstracta del mapa a partir del Template y el Seed dados.
	// Debe ser determinista: mismo Template + mismo Seed => mismo resultado siempre.
	virtual TSharedPtr<FDungeonGrid> Generate(const UMapTemplate* Template, int32 Seed) { return nullptr; }
};
