// Copyright Epic Games, Inc. All Rights Reserved.

#include "MapTemplate.h"

FPrimaryAssetId UMapTemplate::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("MapTemplate"), GetFName());
}
