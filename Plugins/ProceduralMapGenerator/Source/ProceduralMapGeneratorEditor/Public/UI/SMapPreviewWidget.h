// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FDungeonGrid;
struct FMapCell;

// FASE 1: preview 2D del dungeon abstracto. Pinta un rectangulo de color por celda directamente
// en OnPaint (sin crear un SWidget por celda) para que siga siendo eficiente con grids grandes.
class SMapPreviewWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMapPreviewWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetGrid(TSharedPtr<FDungeonGrid> InGrid);

	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	FLinearColor GetCellColor(const FMapCell& Cell) const;

	TSharedPtr<FDungeonGrid> Grid;
	int32 PreviewCellSize = 4; // Pixels por celda en el preview
};
