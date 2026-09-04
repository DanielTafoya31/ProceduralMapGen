// Copyright Epic Games, Inc. All Rights Reserved.

#include "MapBaker.h"
#include "Core/MapAbstractData.h"
#include "MapTemplate.h"
#include "AssetCatalog.h"
#include "ProceduralMapGenerator.h"

#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "ScopedTransaction.h"

bool UMapBaker::BakeToWorld(const FDungeonGrid* Grid, const UMapTemplate* Template, UAssetCatalog* Catalog, UWorld* World)
{
	LastSpawnedActorCount = 0;

	if (!Grid || !Template || !Catalog || !World)
	{
		UE_LOG(LogProceduralMapGenerator, Warning, TEXT("MapBaker: parametros invalidos, se aborta el bake."));
		return false;
	}

	BakeRNG = FRandomStream(Grid->Seed);

	FScopedTransaction Transaction(NSLOCTEXT("ProceduralMapGeneratorEditor", "BakeTransaction", "Bake Procedural Map"));

	AActor* Container = World->SpawnActor<AActor>();
	if (!Container)
	{
		return false;
	}

	Container->SetActorLabel(TEXT("ProceduralMap"));
	Container->SetFlags(RF_Transactional);

	BakeGroundAndWalls(*Grid, *Template, Catalog, World, Container);
	BakeRoomMarkers(*Grid, *Template, Catalog, World, Container);

	if (Template->MapType == EMapType::Interior || Template->MapType == EMapType::Hybrid)
	{
		BakeCeilings(*Grid, *Template, Catalog, World, Container);
	}

	return true;
}

const FAssetEntry* UMapBaker::FindBestAsset(UAssetCatalog* Catalog, const FString& FunctionTag, const FString& StyleTag)
{
	if (!Catalog)
	{
		return nullptr;
	}

	TArray<const FAssetEntry*> StyleMatches;
	TArray<const FAssetEntry*> FunctionOnlyMatches;

	for (const FAssetEntry& Entry : Catalog->Assets)
	{
		if (!Entry.FunctionTags.Contains(FunctionTag))
		{
			continue;
		}

		FunctionOnlyMatches.Add(&Entry);

		if (!StyleTag.IsEmpty() && Entry.StyleTags.Contains(StyleTag))
		{
			StyleMatches.Add(&Entry);
		}
	}

	// Preferir coincidencia de estilo (bioma); si no hay ninguna, cualquier asset con la funcion basta.
	const TArray<const FAssetEntry*>& Candidates = StyleMatches.Num() > 0 ? StyleMatches : FunctionOnlyMatches;
	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	float TotalWeight = 0.f;
	for (const FAssetEntry* Entry : Candidates)
	{
		TotalWeight += FMath::Max(0.01f, Entry->Weight);
	}

	float Roll = BakeRNG.FRandRange(0.f, TotalWeight);
	for (const FAssetEntry* Entry : Candidates)
	{
		Roll -= FMath::Max(0.01f, Entry->Weight);
		if (Roll <= 0.f)
		{
			return Entry;
		}
	}

	return Candidates.Last();
}

void UMapBaker::BakeGroundAndWalls(const FDungeonGrid& Grid, const UMapTemplate& Template, UAssetCatalog* Catalog, UWorld* World, AActor* Container)
{
	const FString StyleTag = UEnum::GetDisplayValueAsText(Template.Biome).ToString();
	const float CellSize = static_cast<float>(Grid.CellSize);
	const float WallHeight = FMath::Max(1.f, Template.SpaceMetrics.MaxCeilingHeight);

	for (int32 Y = 0; Y < Grid.Height; ++Y)
	{
		for (int32 X = 0; X < Grid.Width; ++X)
		{
			const FMapCell* Cell = Grid.GetCell(X, Y);
			if (!Cell || Cell->bIsEmpty)
			{
				continue;
			}

			const FVector CellOrigin(X * CellSize, Y * CellSize, 0.f);

			if (Cell->CellType == EAssetFunction::Ground)
			{
				const FAssetEntry* GroundAsset = FindBestAsset(Catalog, TEXT("Ground"), StyleTag);
				if (!GroundAsset)
				{
					UE_LOG(LogProceduralMapGenerator, Warning, TEXT("MapBaker: no se encontro asset con tag 'Ground' en el catalogo; celda (%d,%d) omitida."), X, Y);
					continue;
				}

				if (UStaticMesh* Mesh = GroundAsset->Mesh.LoadSynchronous())
				{
					FTransform Transform;
					Transform.SetLocation(CellOrigin);

					const FVector MeshBounds = GroundAsset->Bounds.IsNearlyZero() ? FVector(CellSize) : GroundAsset->Bounds;
					Transform.SetScale3D(FVector(CellSize / FMath::Max(1.f, MeshBounds.X), CellSize / FMath::Max(1.f, MeshBounds.Y), 1.f));

					SpawnMeshActor(World, Container, Mesh, Transform);
				}
			}
			else if (Cell->CellType == EAssetFunction::Wall)
			{
				const FAssetEntry* WallAsset = FindBestAsset(Catalog, TEXT("Wall"), StyleTag);
				if (!WallAsset)
				{
					UE_LOG(LogProceduralMapGenerator, Warning, TEXT("MapBaker: no se encontro asset con tag 'Wall' en el catalogo; celda (%d,%d) omitida."), X, Y);
					continue;
				}

				if (UStaticMesh* Mesh = WallAsset->Mesh.LoadSynchronous())
				{
					FTransform Transform;
					Transform.SetLocation(CellOrigin + FVector(0.f, 0.f, WallHeight * 0.5f));

					// Fase 1: aproximacion simple - escalar el mesh de pared para cubrir la celda y la altura del techo.
					const FVector MeshBounds = WallAsset->Bounds.IsNearlyZero() ? FVector(CellSize, CellSize, WallHeight) : WallAsset->Bounds;
					Transform.SetScale3D(FVector(CellSize / FMath::Max(1.f, MeshBounds.X), CellSize / FMath::Max(1.f, MeshBounds.Y), WallHeight / FMath::Max(1.f, MeshBounds.Z)));

					SpawnMeshActor(World, Container, Mesh, Transform);
				}
			}
		}
	}
}

void UMapBaker::BakeRoomMarkers(const FDungeonGrid& Grid, const UMapTemplate& Template, UAssetCatalog* Catalog, UWorld* World, AActor* Container)
{
	const FString StyleTag = UEnum::GetDisplayValueAsText(Template.Biome).ToString();
	const float CellSize = static_cast<float>(Grid.CellSize);

	for (const FRoomData& Room : Grid.Rooms)
	{
		const FIntPoint GridCenter = Room.GridCenter();
		const FVector RoomCenterWorld(GridCenter.X * CellSize, GridCenter.Y * CellSize, 0.f);

		const FAssetEntry* MarkerAsset = nullptr;
		switch (Room.Type)
		{
		case ERoomType::Start:
			MarkerAsset = FindBestAsset(Catalog, TEXT("SpawnPoint"), StyleTag);
			break;
		case ERoomType::End:
			MarkerAsset = FindBestAsset(Catalog, TEXT("Checkpoint"), StyleTag);
			break;
		default:
			break;
		}

		if (MarkerAsset)
		{
			if (UStaticMesh* Mesh = MarkerAsset->Mesh.LoadSynchronous())
			{
				FTransform Transform;
				Transform.SetLocation(RoomCenterWorld + FVector(0.f, 0.f, 10.f));
				SpawnMeshActor(World, Container, Mesh, Transform);
			}
		}
		else if (Room.Type == ERoomType::Start || Room.Type == ERoomType::End)
		{
			UE_LOG(LogProceduralMapGenerator, Warning, TEXT("MapBaker: no se encontro asset marcador para la room %s."), Room.Type == ERoomType::Start ? TEXT("Start") : TEXT("End"));
		}

		// Props/decoracion en rooms Boss y Loot, con densidad segun SpaceMetrics::POIDensity.
		if (Room.Type == ERoomType::Boss || Room.Type == ERoomType::Loot)
		{
			const FString PropTag = Room.Type == ERoomType::Boss ? TEXT("Decoration") : TEXT("Prop");
			const float Density = Template.SpaceMetrics.POIDensity > 0.f ? Template.SpaceMetrics.POIDensity : 0.05f;
			const int32 PropCount = FMath::Max(1, FMath::RoundToInt(Room.Area() * Density));

			for (int32 i = 0; i < PropCount; ++i)
			{
				const FAssetEntry* PropAsset = FindBestAsset(Catalog, PropTag, StyleTag);
				if (!PropAsset)
				{
					continue;
				}

				if (UStaticMesh* Mesh = PropAsset->Mesh.LoadSynchronous())
				{
					const float LocalX = BakeRNG.FRandRange(static_cast<float>(Room.Bounds.Min.X) + 1.f, static_cast<float>(Room.Bounds.Max.X) - 1.f);
					const float LocalY = BakeRNG.FRandRange(static_cast<float>(Room.Bounds.Min.Y) + 1.f, static_cast<float>(Room.Bounds.Max.Y) - 1.f);

					FTransform Transform;
					Transform.SetLocation(FVector(LocalX * CellSize, LocalY * CellSize, 0.f));

					if (PropAsset->bCanRotate)
					{
						Transform.SetRotation(FQuat(FRotator(0.f, BakeRNG.FRandRange(0.f, 360.f), 0.f)));
					}

					SpawnMeshActor(World, Container, Mesh, Transform);
				}
			}
		}
	}
}

void UMapBaker::BakeCeilings(const FDungeonGrid& Grid, const UMapTemplate& Template, UAssetCatalog* Catalog, UWorld* World, AActor* Container)
{
	const FString StyleTag = UEnum::GetDisplayValueAsText(Template.Biome).ToString();
	const float CellSize = static_cast<float>(Grid.CellSize);
	const float CeilingHeight = Template.SpaceMetrics.MaxCeilingHeight;

	const FAssetEntry* CeilingAsset = FindBestAsset(Catalog, TEXT("Ceiling"), StyleTag);
	if (!CeilingAsset)
	{
		UE_LOG(LogProceduralMapGenerator, Warning, TEXT("MapBaker: no se encontro asset con tag 'Ceiling'; se omiten los techos."));
		return;
	}

	UStaticMesh* Mesh = CeilingAsset->Mesh.LoadSynchronous();
	if (!Mesh)
	{
		return;
	}

	const FVector MeshBounds = CeilingAsset->Bounds.IsNearlyZero() ? FVector(CellSize) : CeilingAsset->Bounds;
	const FVector Scale(CellSize / FMath::Max(1.f, MeshBounds.X), CellSize / FMath::Max(1.f, MeshBounds.Y), 1.f);

	for (int32 Y = 0; Y < Grid.Height; ++Y)
	{
		for (int32 X = 0; X < Grid.Width; ++X)
		{
			const FMapCell* Cell = Grid.GetCell(X, Y);
			if (!Cell || !Cell->bIsRoom)
			{
				continue;
			}

			FTransform Transform;
			Transform.SetLocation(FVector(X * CellSize, Y * CellSize, CeilingHeight));
			Transform.SetScale3D(Scale);

			SpawnMeshActor(World, Container, Mesh, Transform);
		}
	}
}

AActor* UMapBaker::SpawnMeshActor(UWorld* World, AActor* AttachParent, UStaticMesh* Mesh, const FTransform& Transform)
{
	if (!World || !Mesh)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform, SpawnParams);
	if (!MeshActor)
	{
		return nullptr;
	}

	MeshActor->SetActorLabel(Mesh->GetName());
	MeshActor->SetFlags(RF_Transactional);

	if (UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent())
	{
		MeshComponent->SetStaticMesh(Mesh);
		MeshComponent->SetMobility(EComponentMobility::Static);
	}

	if (AttachParent)
	{
		MeshActor->AttachToActor(AttachParent, FAttachmentTransformRules::KeepWorldTransform);
	}

	++LastSpawnedActorCount;

	return MeshActor;
}
