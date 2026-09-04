// Copyright Epic Games, Inc. All Rights Reserved.

#include "SProceduralMapGeneratorWidget.h"
#include "ProceduralMapGeneratorSubsystem.h"
#include "AssetCatalog.h"
#include "MapTemplate.h"
#include "PMGStructs.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"

#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/DataAssetFactory.h"
#include "Misc/MessageDialog.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/Package.h"
#include "Styling/CoreStyle.h"

// FASE 1
#include "UI/SMapPreviewWidget.h"
#include "Core/MapAbstractData.h"
#include "Validation/MapValidator.h"

#define PMG_NS "ProceduralMapGeneratorEditor"

void SProceduralMapGeneratorWidget::Construct(const FArguments& InArgs)
{
	BiomeOptions.Reset();
	for (uint8 i = 0; i <= static_cast<uint8>(EBiomeType::Custom); ++i)
	{
		BiomeOptions.Add(MakeShared<EBiomeType>(static_cast<EBiomeType>(i)));
	}
	SelectedBiome = BiomeOptions[0];

	GenreOptions.Reset();
	for (uint8 i = 0; i <= static_cast<uint8>(EGameGenre::Other); ++i)
	{
		GenreOptions.Add(MakeShared<EGameGenre>(static_cast<EGameGenre>(i)));
	}
	SelectedGenre = GenreOptions[0];

	RefreshTemplateOptions();

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT(PMG_NS, "PanelTitle", "Procedural Map Generator"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT(PMG_NS, "PanelVersion", "v0.2.0 - Fase 1"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(8.f) [ BuildTemplateSection() ]
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f) [ BuildAssetSection() ]
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f) [ BuildQuickConfigSection() ]
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f) [ BuildActionsSection() ]
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f) [ BuildPreviewSection() ] // FASE 1

			+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
			[
				SNew(STextBlock)
				.Text(this, &SProceduralMapGeneratorWidget::GetStatusText)
				.AutoWrapText(true)
			]
		]
	];
}

TSharedRef<SWidget> SProceduralMapGeneratorWidget::BuildTemplateSection()
{
	return SNew(SBorder)
	.Padding(8.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT(PMG_NS, "TemplateSectionTitle", "Template"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SAssignNew(TemplateComboBox, SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&TemplateOptions)
			.OnGenerateWidget(this, &SProceduralMapGeneratorWidget::OnGenerateTemplateComboWidget)
			.OnSelectionChanged(this, &SProceduralMapGeneratorWidget::OnTemplateSelectionChanged)
			[
				SNew(STextBlock)
				.Text(this, &SProceduralMapGeneratorWidget::GetSelectedTemplateText)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT(PMG_NS, "CreateTemplate", "Create New Template"))
				.OnClicked(this, &SProceduralMapGeneratorWidget::OnCreateNewTemplateClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(NSLOCTEXT(PMG_NS, "EditTemplate", "Edit Template"))
				.OnClicked(this, &SProceduralMapGeneratorWidget::OnEditTemplateClicked)
			]
		]
	];
}

TSharedRef<SWidget> SProceduralMapGeneratorWidget::BuildAssetSection()
{
	return SNew(SBorder)
	.Padding(8.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT(PMG_NS, "AssetSectionTitle", "Assets"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
			[
				SAssignNew(NewSourcePathTextBox, SEditableTextBox)
				.HintText(NSLOCTEXT(PMG_NS, "SourcePathHint", "/Game/Environments/..."))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(NSLOCTEXT(PMG_NS, "AddPath", "Add Path"))
				.OnClicked(this, &SProceduralMapGeneratorWidget::OnAddSourcePathClicked)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f).MaxHeight(100.f)
		[
			SAssignNew(SourcePathListView, SListView<TSharedPtr<FString>>)
			.ListItemsSource(&SourcePaths)
			.OnGenerateRow(this, &SProceduralMapGeneratorWidget::OnGenerateSourcePathRow)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SButton)
			.Text(NSLOCTEXT(PMG_NS, "ScanAssets", "Scan Assets"))
			.OnClicked(this, &SProceduralMapGeneratorWidget::OnScanAssetsClicked)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(this, &SProceduralMapGeneratorWidget::GetAssetCountText)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f).MaxHeight(200.f)
		[
			SAssignNew(AssetListView, SListView<TSharedPtr<FAssetEntry>>)
			.ListItemsSource(&DisplayedAssets)
			.OnGenerateRow(this, &SProceduralMapGeneratorWidget::OnGenerateAssetRow)
		]
	];
}

TSharedRef<SWidget> SProceduralMapGeneratorWidget::BuildQuickConfigSection()
{
	return SNew(SBorder)
	.Padding(8.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT(PMG_NS, "QuickConfigTitle", "Configuracion Rapida"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(NSLOCTEXT(PMG_NS, "SeedLabel", "Seed:"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SSpinBox<int32>)
				.MinValue(0)
				.Value(this, &SProceduralMapGeneratorWidget::GetSeed)
				.OnValueChanged(this, &SProceduralMapGeneratorWidget::OnSeedChanged)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(NSLOCTEXT(PMG_NS, "MapSizeLabel", "Map Size (X/Y):"))
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SSpinBox<int32>)
				.MinValue(1)
				.MaxValue(1000)
				.Value(this, &SProceduralMapGeneratorWidget::GetMapSizeX)
				.OnValueChanged(this, &SProceduralMapGeneratorWidget::OnMapSizeXChanged)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f)
			[
				SNew(SSpinBox<int32>)
				.MinValue(1)
				.MaxValue(1000)
				.Value(this, &SProceduralMapGeneratorWidget::GetMapSizeY)
				.OnValueChanged(this, &SProceduralMapGeneratorWidget::OnMapSizeYChanged)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SComboBox<TSharedPtr<EBiomeType>>)
			.OptionsSource(&BiomeOptions)
			.OnGenerateWidget_Lambda([](TSharedPtr<EBiomeType> Item)
			{
				return SNew(STextBlock).Text(UEnum::GetDisplayValueAsText(*Item));
			})
			.OnSelectionChanged(this, &SProceduralMapGeneratorWidget::OnBiomeSelectionChanged)
			[
				SNew(STextBlock).Text(this, &SProceduralMapGeneratorWidget::GetSelectedBiomeText)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SComboBox<TSharedPtr<EGameGenre>>)
			.OptionsSource(&GenreOptions)
			.OnGenerateWidget_Lambda([](TSharedPtr<EGameGenre> Item)
			{
				return SNew(STextBlock).Text(UEnum::GetDisplayValueAsText(*Item));
			})
			.OnSelectionChanged(this, &SProceduralMapGeneratorWidget::OnGenreSelectionChanged)
			[
				SNew(STextBlock).Text(this, &SProceduralMapGeneratorWidget::GetSelectedGenreText)
			]
		]
	];
}

TSharedRef<SWidget> SProceduralMapGeneratorWidget::BuildActionsSection()
{
	return SNew(SBorder)
	.Padding(8.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT(PMG_NS, "ActionsTitle", "Acciones"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SUniformGridPanel).SlotPadding(FMargin(2.f))
			+ SUniformGridPanel::Slot(0, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(NSLOCTEXT(PMG_NS, "ValidateAssets", "Validate Assets"))
				.OnClicked(this, &SProceduralMapGeneratorWidget::OnValidateAssetsClicked)
			]
			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(NSLOCTEXT(PMG_NS, "PreviewAbstract", "Preview Abstract"))
				.OnClicked(this, &SProceduralMapGeneratorWidget::OnPreviewAbstractClicked)
			]
			+ SUniformGridPanel::Slot(0, 1)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(NSLOCTEXT(PMG_NS, "GenerateMap", "Generate Map"))
				.OnClicked(this, &SProceduralMapGeneratorWidget::OnGenerateMapClicked)
			]
			+ SUniformGridPanel::Slot(1, 1)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(NSLOCTEXT(PMG_NS, "SaveConfig", "Save Config"))
				.OnClicked(this, &SProceduralMapGeneratorWidget::OnSaveConfigClicked)
			]
		]
	];
}

// FASE 1
TSharedRef<SWidget> SProceduralMapGeneratorWidget::BuildPreviewSection()
{
	return SNew(SBorder)
	.Padding(8.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT(PMG_NS, "PreviewSectionTitle", "Preview"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SBox)
			.MaxDesiredHeight(240.f)
			[
				SNew(SScrollBox)
				.Orientation(Orient_Vertical)
				+ SScrollBox::Slot()
				[
					SNew(SScrollBox)
					.Orientation(Orient_Horizontal)
					+ SScrollBox::Slot()
					[
						SAssignNew(PreviewWidget, SMapPreviewWidget)
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(STextBlock)
			.Text(this, &SProceduralMapGeneratorWidget::GetPreviewStatusText)
			.ColorAndOpacity(this, &SProceduralMapGeneratorWidget::GetPreviewStatusColor)
			.AutoWrapText(true)
		]
	];
}

UProceduralMapGeneratorSubsystem* SProceduralMapGeneratorWidget::GetSubsystem() const
{
	return GEditor ? GEditor->GetEditorSubsystem<UProceduralMapGeneratorSubsystem>() : nullptr;
}

// FASE 1
UMapTemplate* SProceduralMapGeneratorWidget::GetSelectedMapTemplate() const
{
	UProceduralMapGeneratorSubsystem* Subsystem = GetSubsystem();
	if (!Subsystem || !SelectedTemplateOption.IsValid())
	{
		return nullptr;
	}

	for (UMapTemplate* Template : Subsystem->GetAvailableTemplates())
	{
		if (Template && Template->GetName() == *SelectedTemplateOption)
		{
			return Template;
		}
	}

	return nullptr;
}

// FASE 1
bool SProceduralMapGeneratorWidget::RunDungeonGeneration(TSharedPtr<FDungeonGrid>& OutGrid, FString& OutSummary)
{
	OutGrid.Reset();
	OutSummary.Reset();

	UProceduralMapGeneratorSubsystem* Subsystem = GetSubsystem();
	UMapTemplate* Template = GetSelectedMapTemplate();

	if (!Subsystem || !Template)
	{
		OutSummary = TEXT("Selecciona un template valido antes de continuar.");
		return false;
	}

	FString GenerationError;
	OutGrid = Subsystem->GeneratePreview(Template, Seed, GenerationError);

	if (!OutGrid.IsValid())
	{
		OutSummary = GenerationError;
		return false;
	}

	// Se re-ejecutan los checks individuales (en vez de solo el agregado) para poder mostrar
	// un checklist detallado, tal como pide la seccion 6 del spec para el flujo de validacion.
	UMapValidator* Validator = NewObject<UMapValidator>();

	FString StartEndError, ConnectivityError, RoomSizeError;
	const bool bStartEndOk = Validator->ValidateStartEndExist(OutGrid.Get(), StartEndError);
	const bool bConnectivityOk = bStartEndOk && Validator->ValidateConnectivity(OutGrid.Get(), ConnectivityError);
	const bool bRoomSizeOk = Validator->ValidateRoomSizes(OutGrid.Get(), Template, RoomSizeError);

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("%s Start/End existen"), bStartEndOk ? TEXT("[OK]") : TEXT("[FALLO]")));
	Lines.Add(FString::Printf(TEXT("%s Conectividad"), bConnectivityOk ? TEXT("[OK]") : TEXT("[FALLO]")));
	Lines.Add(FString::Printf(TEXT("%s Tamano de rooms"), bRoomSizeOk ? TEXT("[OK]") : TEXT("[FALLO]")));
	Lines.Add(FString::Printf(TEXT("Rooms: %d | Celdas: %d x %d"), OutGrid->Rooms.Num(), OutGrid->Width, OutGrid->Height));

	if (!bStartEndOk)
	{
		Lines.Add(StartEndError);
	}
	if (bStartEndOk && !bConnectivityOk)
	{
		Lines.Add(ConnectivityError);
	}
	if (!bRoomSizeOk)
	{
		Lines.Add(RoomSizeError);
	}

	OutSummary = FString::Join(Lines, TEXT("\n"));

	return bStartEndOk && bConnectivityOk && bRoomSizeOk;
}

void SProceduralMapGeneratorWidget::RefreshTemplateOptions()
{
	TemplateOptions.Reset();

	if (UProceduralMapGeneratorSubsystem* Subsystem = GetSubsystem())
	{
		for (UMapTemplate* Template : Subsystem->GetAvailableTemplates())
		{
			if (Template)
			{
				TemplateOptions.Add(MakeShared<FString>(Template->GetName()));
			}
		}
	}

	if (TemplateOptions.Num() > 0 && !SelectedTemplateOption.IsValid())
	{
		SelectedTemplateOption = TemplateOptions[0];
	}

	if (TemplateComboBox.IsValid())
	{
		TemplateComboBox->RefreshOptions();
	}
}

TSharedRef<SWidget> SProceduralMapGeneratorWidget::OnGenerateTemplateComboWidget(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
}

void SProceduralMapGeneratorWidget::OnTemplateSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedTemplateOption = NewSelection;
}

FText SProceduralMapGeneratorWidget::GetSelectedTemplateText() const
{
	return SelectedTemplateOption.IsValid() ? FText::FromString(*SelectedTemplateOption) : NSLOCTEXT(PMG_NS, "NoTemplateSelected", "Ningun template seleccionado");
}

FReply SProceduralMapGeneratorWidget::OnCreateNewTemplateClicked()
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UMapTemplate::StaticClass();

	UObject* NewAsset = AssetToolsModule.Get().CreateAssetWithDialog(TEXT("NewMapTemplate"), TEXT("/Game/ProceduralMapGenerator/Templates"), UMapTemplate::StaticClass(), Factory);

	if (NewAsset)
	{
		if (UProceduralMapGeneratorSubsystem* Subsystem = GetSubsystem())
		{
			Subsystem->RefreshTemplateCache();
		}
		RefreshTemplateOptions();
		StatusText = NSLOCTEXT(PMG_NS, "TemplateCreated", "Template creado correctamente.");
	}

	return FReply::Handled();
}

FReply SProceduralMapGeneratorWidget::OnEditTemplateClicked()
{
	// FASE 1: usa el helper GetSelectedMapTemplate() en vez de repetir la busqueda por nombre.
	if (UMapTemplate* Template = GetSelectedMapTemplate())
	{
		if (GEditor)
		{
			if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
			{
				AssetEditorSubsystem->OpenEditorForAsset(Template);
			}
		}
	}

	return FReply::Handled();
}

FReply SProceduralMapGeneratorWidget::OnAddSourcePathClicked()
{
	if (NewSourcePathTextBox.IsValid())
	{
		const FString NewPath = NewSourcePathTextBox->GetText().ToString();
		if (!NewPath.IsEmpty())
		{
			SourcePaths.Add(MakeShared<FString>(NewPath));
			NewSourcePathTextBox->SetText(FText::GetEmpty());

			if (SourcePathListView.IsValid())
			{
				SourcePathListView->RequestListRefresh();
			}
		}
	}

	return FReply::Handled();
}

FReply SProceduralMapGeneratorWidget::OnRemoveSourcePathClicked(TSharedPtr<FString> PathToRemove)
{
	SourcePaths.RemoveAll([&PathToRemove](const TSharedPtr<FString>& Existing) { return Existing == PathToRemove; });

	if (SourcePathListView.IsValid())
	{
		SourcePathListView->RequestListRefresh();
	}

	return FReply::Handled();
}

TSharedRef<ITableRow> SProceduralMapGeneratorWidget::OnGenerateSourcePathRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.Text(NSLOCTEXT(PMG_NS, "RemovePath", "X"))
			.OnClicked(this, &SProceduralMapGeneratorWidget::OnRemoveSourcePathClicked, Item)
		]
	];
}

TSharedRef<ITableRow> SProceduralMapGeneratorWidget::OnGenerateAssetRow(TSharedPtr<FAssetEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FString TagsCombined = FString::Join(Item->FunctionTags, TEXT(", "));
	if (Item->StyleTags.Num() > 0)
	{
		TagsCombined += TEXT(" | ") + FString::Join(Item->StyleTags, TEXT(", "));
	}

	return SNew(STableRow<TSharedPtr<FAssetEntry>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.4f)
		[
			SNew(STextBlock).Text(FText::FromString(Item->AssetName))
		]
		+ SHorizontalBox::Slot().FillWidth(0.6f)
		[
			SNew(STextBlock).Text(FText::FromString(TagsCombined))
		]
	];
}

FReply SProceduralMapGeneratorWidget::OnScanAssetsClicked()
{
	TArray<FString> Paths;
	for (const TSharedPtr<FString>& Path : SourcePaths)
	{
		if (Path.IsValid())
		{
			Paths.Add(*Path);
		}
	}

	if (Paths.Num() == 0)
	{
		StatusText = NSLOCTEXT(PMG_NS, "NoPaths", "Agrega al menos una carpeta antes de escanear.");
		return FReply::Handled();
	}

	UProceduralMapGeneratorSubsystem* Subsystem = GetSubsystem();
	if (!Subsystem)
	{
		StatusText = NSLOCTEXT(PMG_NS, "NoSubsystem", "No se pudo acceder al subsystem del editor.");
		return FReply::Handled();
	}

	FScopedSlowTask SlowTask(1.f, NSLOCTEXT(PMG_NS, "Scanning", "Escaneando assets..."));
	SlowTask.MakeDialog();

	Subsystem->ScanAssets(Paths);
	SlowTask.EnterProgressFrame(1.f);

	DisplayedAssets.Reset();
	if (UAssetCatalog* Catalog = Subsystem->GetAssetCatalog())
	{
		for (const FAssetEntry& Entry : Catalog->Assets)
		{
			DisplayedAssets.Add(MakeShared<FAssetEntry>(Entry));
		}
	}

	if (AssetListView.IsValid())
	{
		AssetListView->RequestListRefresh();
	}

	StatusText = FText::Format(NSLOCTEXT(PMG_NS, "ScanComplete", "Escaneo completo: {0} assets encontrados."), FText::AsNumber(DisplayedAssets.Num()));

	return FReply::Handled();
}

FText SProceduralMapGeneratorWidget::GetAssetCountText() const
{
	return FText::Format(NSLOCTEXT(PMG_NS, "AssetCount", "Assets encontrados: {0}"), FText::AsNumber(DisplayedAssets.Num()));
}

int32 SProceduralMapGeneratorWidget::GetSeed() const { return Seed; }
void SProceduralMapGeneratorWidget::OnSeedChanged(int32 NewSeed) { Seed = NewSeed; }

int32 SProceduralMapGeneratorWidget::GetMapSizeX() const { return MapSizeX; }
void SProceduralMapGeneratorWidget::OnMapSizeXChanged(int32 NewValue) { MapSizeX = NewValue; }

int32 SProceduralMapGeneratorWidget::GetMapSizeY() const { return MapSizeY; }
void SProceduralMapGeneratorWidget::OnMapSizeYChanged(int32 NewValue) { MapSizeY = NewValue; }

void SProceduralMapGeneratorWidget::OnBiomeSelectionChanged(TSharedPtr<EBiomeType> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedBiome = NewSelection;
}

FText SProceduralMapGeneratorWidget::GetSelectedBiomeText() const
{
	return SelectedBiome.IsValid() ? UEnum::GetDisplayValueAsText(*SelectedBiome) : FText::GetEmpty();
}

void SProceduralMapGeneratorWidget::OnGenreSelectionChanged(TSharedPtr<EGameGenre> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedGenre = NewSelection;
}

FText SProceduralMapGeneratorWidget::GetSelectedGenreText() const
{
	return SelectedGenre.IsValid() ? UEnum::GetDisplayValueAsText(*SelectedGenre) : FText::GetEmpty();
}

FReply SProceduralMapGeneratorWidget::OnValidateAssetsClicked()
{
	UProceduralMapGeneratorSubsystem* Subsystem = GetSubsystem();
	if (!Subsystem || !Subsystem->GetAssetCatalog())
	{
		StatusText = NSLOCTEXT(PMG_NS, "NoSubsystem", "No se pudo acceder al subsystem del editor.");
		return FReply::Handled();
	}

	TArray<FString> Missing, Duplicates, Untagged;
	Subsystem->GetAssetCatalog()->ValidateCatalog(Missing, Duplicates, Untagged);

	StatusText = FText::Format(
		NSLOCTEXT(PMG_NS, "ValidateResult", "Validacion: {0} faltantes, {1} duplicados, {2} sin tags."),
		FText::AsNumber(Missing.Num()), FText::AsNumber(Duplicates.Num()), FText::AsNumber(Untagged.Num()));

	return FReply::Handled();
}

// FASE 1
FReply SProceduralMapGeneratorWidget::OnPreviewAbstractClicked()
{
	TSharedPtr<FDungeonGrid> Grid;
	FString Summary;
	const bool bValid = RunDungeonGeneration(Grid, Summary);

	CurrentGrid = Grid;
	if (PreviewWidget.IsValid())
	{
		PreviewWidget->SetGrid(Grid);
	}

	PreviewStatusText = FText::FromString(Summary);
	PreviewStatusColor = bValid ? FSlateColor(FLinearColor(0.2f, 0.9f, 0.2f)) : FSlateColor(FLinearColor(1.f, 0.3f, 0.3f));

	StatusText = bValid
		? NSLOCTEXT(PMG_NS, "PreviewOk", "Preview generado correctamente.")
		: NSLOCTEXT(PMG_NS, "PreviewFailed", "Preview generado con errores de validacion (ver detalle abajo).");

	return FReply::Handled();
}

// FASE 1
FReply SProceduralMapGeneratorWidget::OnGenerateMapClicked()
{
	TSharedPtr<FDungeonGrid> Grid;
	FString Summary;
	const bool bValid = RunDungeonGeneration(Grid, Summary);

	CurrentGrid = Grid;
	if (PreviewWidget.IsValid())
	{
		PreviewWidget->SetGrid(Grid);
	}

	PreviewStatusText = FText::FromString(Summary);
	PreviewStatusColor = bValid ? FSlateColor(FLinearColor(0.2f, 0.9f, 0.2f)) : FSlateColor(FLinearColor(1.f, 0.3f, 0.3f));

	if (!bValid)
	{
		StatusText = NSLOCTEXT(PMG_NS, "GenerateValidationFailed", "El dungeon generado no paso la validacion; revisa el detalle del preview.");
		return FReply::Handled();
	}

	const EAppReturnType::Type Confirmation = FMessageDialog::Open(EAppMsgType::YesNo,
		NSLOCTEXT(PMG_NS, "ConfirmBake", "Esto instanciara meshes en el nivel actual. ¿Continuar?"));

	if (Confirmation != EAppReturnType::Yes)
	{
		StatusText = NSLOCTEXT(PMG_NS, "BakeCancelled", "Bake cancelado por el usuario.");
		return FReply::Handled();
	}

	UProceduralMapGeneratorSubsystem* Subsystem = GetSubsystem();
	UMapTemplate* Template = GetSelectedMapTemplate();

	if (!Subsystem || !Template || !Grid.IsValid())
	{
		StatusText = NSLOCTEXT(PMG_NS, "BakeFailedNoData", "No se pudo hacer bake: faltan datos (template/subsystem/grid).");
		return FReply::Handled();
	}

	const bool bBakeSuccess = Subsystem->BakeMap(Grid.Get(), Template);

	if (bBakeSuccess)
	{
		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports();
		}

		StatusText = FText::Format(
			NSLOCTEXT(PMG_NS, "BakeSuccess", "Mapa generado: {0} rooms, {1} celdas, {2} actors instanciados."),
			FText::AsNumber(Grid->Rooms.Num()),
			FText::AsNumber(Grid->Width * Grid->Height),
			FText::AsNumber(Subsystem->GetLastBakedActorCount()));
	}
	else
	{
		StatusText = NSLOCTEXT(PMG_NS, "BakeFailed", "El bake fallo. Revisa el Output Log para mas detalles.");
	}

	return FReply::Handled();
}

FReply SProceduralMapGeneratorWidget::OnSaveConfigClicked()
{
	// FASE 1: usa el helper GetSelectedMapTemplate() en vez de repetir la busqueda por nombre.
	UMapTemplate* TargetTemplate = GetSelectedMapTemplate();

	if (!TargetTemplate)
	{
		StatusText = NSLOCTEXT(PMG_NS, "NoTemplateForSave", "Selecciona o crea un template antes de guardar.");
		return FReply::Handled();
	}

	TargetTemplate->Modify();
	TargetTemplate->DefaultSeed = Seed;
	TargetTemplate->MapSize = FVector2D(MapSizeX, MapSizeY);
	if (SelectedBiome.IsValid())
	{
		TargetTemplate->Biome = *SelectedBiome;
	}
	if (SelectedGenre.IsValid())
	{
		TargetTemplate->Genre = *SelectedGenre;
	}

	TargetTemplate->GetPackage()->MarkPackageDirty();

	StatusText = NSLOCTEXT(PMG_NS, "ConfigSaved", "Configuracion guardada en el template.");

	return FReply::Handled();
}

#undef PMG_NS
