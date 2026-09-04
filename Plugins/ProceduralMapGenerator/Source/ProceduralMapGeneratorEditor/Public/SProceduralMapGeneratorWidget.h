// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SComboBox.h"
#include "Styling/SlateColor.h" // FASE 1
#include "PMGTypes.h"

class UProceduralMapGeneratorSubsystem;
class UMapTemplate;
struct FAssetEntry;
struct FDungeonGrid; // FASE 1
class SEditableTextBox;
class SMapPreviewWidget; // FASE 1
class ITableRow;
class STableViewBase;

// Panel dockable del editor (Window > Procedural Map Generator). Fase 0: expone template,
// catalogo de assets, configuracion rapida y acciones placeholder.
class SProceduralMapGeneratorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProceduralMapGeneratorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildTemplateSection();
	TSharedRef<SWidget> BuildAssetSection();
	TSharedRef<SWidget> BuildQuickConfigSection();
	TSharedRef<SWidget> BuildActionsSection();
	TSharedRef<SWidget> BuildPreviewSection(); // FASE 1

	void RefreshTemplateOptions();
	TSharedRef<SWidget> OnGenerateTemplateComboWidget(TSharedPtr<FString> InItem);
	void OnTemplateSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	FText GetSelectedTemplateText() const;

	FReply OnCreateNewTemplateClicked();
	FReply OnEditTemplateClicked();

	FReply OnScanAssetsClicked();
	FText GetAssetCountText() const;
	TSharedRef<ITableRow> OnGenerateAssetRow(TSharedPtr<FAssetEntry> Item, const TSharedRef<STableViewBase>& OwnerTable);

	FReply OnAddSourcePathClicked();
	FReply OnRemoveSourcePathClicked(TSharedPtr<FString> PathToRemove);
	TSharedRef<ITableRow> OnGenerateSourcePathRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);

	int32 GetSeed() const;
	void OnSeedChanged(int32 NewSeed);

	int32 GetMapSizeX() const;
	void OnMapSizeXChanged(int32 NewValue);
	int32 GetMapSizeY() const;
	void OnMapSizeYChanged(int32 NewValue);

	void OnBiomeSelectionChanged(TSharedPtr<EBiomeType> NewSelection, ESelectInfo::Type SelectInfo);
	FText GetSelectedBiomeText() const;

	void OnGenreSelectionChanged(TSharedPtr<EGameGenre> NewSelection, ESelectInfo::Type SelectInfo);
	FText GetSelectedGenreText() const;

	FReply OnValidateAssetsClicked();
	FReply OnPreviewAbstractClicked();
	FReply OnGenerateMapClicked();
	FReply OnSaveConfigClicked();

	FText GetStatusText() const { return StatusText; }

	UProceduralMapGeneratorSubsystem* GetSubsystem() const;

	// FASE 1
	UMapTemplate* GetSelectedMapTemplate() const;

	// Corre Generate() + Validate() para el template/seed/mapsize actuales de la UI. Devuelve
	// true si el dungeon es jugable; OutSummary siempre trae un checklist legible (✓/✗ por cada
	// regla) que se muestra bajo el preview, tanto en exito como en fallo.
	bool RunDungeonGeneration(TSharedPtr<FDungeonGrid>& OutGrid, FString& OutSummary);

	FText GetPreviewStatusText() const { return PreviewStatusText; }
	FSlateColor GetPreviewStatusColor() const { return PreviewStatusColor; }

private:
	TArray<TSharedPtr<FString>> TemplateOptions;
	TSharedPtr<FString> SelectedTemplateOption;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> TemplateComboBox;

	TArray<TSharedPtr<FString>> SourcePaths;
	TSharedPtr<SListView<TSharedPtr<FString>>> SourcePathListView;
	TSharedPtr<SEditableTextBox> NewSourcePathTextBox;

	TArray<TSharedPtr<FAssetEntry>> DisplayedAssets;
	TSharedPtr<SListView<TSharedPtr<FAssetEntry>>> AssetListView;

	TArray<TSharedPtr<EBiomeType>> BiomeOptions;
	TSharedPtr<EBiomeType> SelectedBiome;

	TArray<TSharedPtr<EGameGenre>> GenreOptions;
	TSharedPtr<EGameGenre> SelectedGenre;

	int32 Seed = 0;
	int32 MapSizeX = 50;
	int32 MapSizeY = 50;

	FText StatusText;

	// FASE 1
	TSharedPtr<SMapPreviewWidget> PreviewWidget;
	TSharedPtr<FDungeonGrid> CurrentGrid;
	FText PreviewStatusText;
	FSlateColor PreviewStatusColor = FSlateColor(FLinearColor::White);
};
