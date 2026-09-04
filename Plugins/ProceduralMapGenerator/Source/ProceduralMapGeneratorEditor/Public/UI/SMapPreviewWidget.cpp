// Copyright Epic Games, Inc. All Rights Reserved.

#include "SMapPreviewWidget.h"
#include "Core/MapAbstractData.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void SMapPreviewWidget::Construct(const FArguments& InArgs)
{
}

void SMapPreviewWidget::SetGrid(TSharedPtr<FDungeonGrid> InGrid)
{
	Grid = InGrid;
	Invalidate(EInvalidateWidget::Layout);
}

FVector2D SMapPreviewWidget::ComputeDesiredSize(float) const
{
	if (!Grid.IsValid() || Grid->Width <= 0 || Grid->Height <= 0)
	{
		return FVector2D(200.f, 200.f);
	}

	return FVector2D(Grid->Width * PreviewCellSize, Grid->Height * PreviewCellSize);
}

FLinearColor SMapPreviewWidget::GetCellColor(const FMapCell& Cell) const
{
	if (Cell.bIsEmpty)
	{
		return FLinearColor(0.05f, 0.05f, 0.05f, 1.0f); // Empty/Background
	}

	if (Cell.CellType == EAssetFunction::Wall)
	{
		return FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // Wall
	}

	if (Cell.bIsRoom && Grid.IsValid() && Grid->Rooms.IsValidIndex(Cell.RoomId))
	{
		switch (Grid->Rooms[Cell.RoomId].Type)
		{
		case ERoomType::Start: return FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);
		case ERoomType::End:   return FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
		case ERoomType::Boss:  return FLinearColor(0.5f, 0.0f, 1.0f, 1.0f);
		case ERoomType::Loot:  return FLinearColor(1.0f, 0.8f, 0.0f, 1.0f);
		default:               return FLinearColor(0.8f, 0.8f, 0.8f, 1.0f); // Ground (room normal)
		}
	}

	return FLinearColor(0.6f, 0.6f, 0.6f, 1.0f); // Ground fuera de una room = Corridor
}

int32 SMapPreviewWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!Grid.IsValid())
	{
		return LayerId;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const FVector2D CellSizePx(static_cast<float>(PreviewCellSize), static_cast<float>(PreviewCellSize));

	for (int32 Y = 0; Y < Grid->Height; ++Y)
	{
		for (int32 X = 0; X < Grid->Width; ++X)
		{
			const FMapCell* Cell = Grid->GetCell(X, Y);
			if (!Cell)
			{
				continue;
			}

			const FVector2D CellPosition(X * PreviewCellSize, Y * PreviewCellSize);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(CellSizePx, FSlateLayoutTransform(CellPosition)),
				WhiteBrush,
				ESlateDrawEffect::None,
				GetCellColor(*Cell)
			);
		}
	}

	return LayerId + 1;
}
