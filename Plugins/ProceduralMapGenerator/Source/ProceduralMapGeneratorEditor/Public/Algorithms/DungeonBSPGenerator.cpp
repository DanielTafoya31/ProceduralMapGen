// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonBSPGenerator.h"
#include "MapTemplate.h"
#include "Containers/Queue.h"
#include "Containers/Set.h"

TSharedPtr<FDungeonGrid> UDungeonBSPGenerator::Generate(const UMapTemplate* Template, int32 Seed)
{
	if (!Template)
	{
		return nullptr;
	}

	TSharedPtr<FDungeonGrid> GridPtr = MakeShared<FDungeonGrid>();
	FDungeonGrid& Grid = *GridPtr;

	const int32 GridWidth = FMath::Max(1, FMath::RoundToInt(Template->MapSize.X));
	const int32 GridHeight = FMath::Max(1, FMath::RoundToInt(Template->MapSize.Y));

	Grid.Init(GridWidth, GridHeight, 200);
	Grid.Seed = Seed;

	FRandomStream RNG(Seed);

	FBSPNode Root;
	Root.Bounds = FIntRect(0, 0, GridWidth, GridHeight);

	const int32 MinLeafSize = 8; // celdas
	const int32 MaxDepth = 8;
	int32 LeafCounter = 0;

	// Decision de diseno (seccion 11): si el BSP generaria mas rooms que MaxRooms, se detiene la
	// division ANTES de crearlas (presupuesto de hojas), en vez de fusionar rooms despues de
	// crearlas. Es mas simple, evita tener que re-tallar celdas ya escritas en el grid y es
	// determinista de la misma forma (mismo seed -> mismo punto de corte).
	const int32 MaxLeaves = FMath::Max(1, Template->GameplayRules.MaxRooms);

	SplitNode(Root, MinLeafSize, MaxDepth, 0, MaxLeaves, LeafCounter, RNG);

	TArray<FBSPNode*> Leaves;
	CollectLeaves(Root, Leaves);

	CreateRoomsFromLeaves(Leaves, Grid, Template, RNG);

	if (Grid.Rooms.Num() == 0)
	{
		// No se pudo generar ninguna room (mapa demasiado pequeno). Se devuelve el grid vacio;
		// el validador lo marcara como invalido con un mensaje claro.
		return GridPtr;
	}

	ConnectRooms(Grid, Template, RNG);
	AssignRoomTypes(Grid, RNG);
	MarkWalls(Grid);

	return GridPtr;
}

void UDungeonBSPGenerator::SplitNode(FBSPNode& Node, int32 MinLeafSize, int32 MaxDepth, int32 CurrentDepth, int32 MaxLeaves, int32& LeafCounter, FRandomStream& RNG) const
{
	const int32 NodeWidth = Node.Bounds.Width();
	const int32 NodeHeight = Node.Bounds.Height();

	const bool bTooSmall = NodeWidth < MinLeafSize * 2 || NodeHeight < MinLeafSize * 2;
	const bool bMaxDepthReached = CurrentDepth >= MaxDepth;
	const bool bReachedRoomBudget = (LeafCounter + 1) >= MaxLeaves;
	const bool bRandomStop = CurrentDepth > 1 && RNG.FRandRange(0.f, 1.f) < 0.3f;

	if (bTooSmall || bMaxDepthReached || bReachedRoomBudget || bRandomStop)
	{
		++LeafCounter;
		return;
	}

	// Nodo mas ancho que alto -> se divide verticalmente (crea izquierda/derecha), y viceversa;
	// si es aproximadamente cuadrado, se elige al azar. Esto evita hojas muy alargadas.
	const bool bSplitVertical = NodeWidth > NodeHeight ? true : (NodeHeight > NodeWidth ? false : RNG.GetFraction() < 0.5f);

	if (bSplitVertical)
	{
		const int32 SplitX = RNG.RandRange(Node.Bounds.Min.X + MinLeafSize, Node.Bounds.Max.X - MinLeafSize);
		Node.Left = MakeUnique<FBSPNode>();
		Node.Left->Bounds = FIntRect(Node.Bounds.Min.X, Node.Bounds.Min.Y, SplitX, Node.Bounds.Max.Y);
		Node.Right = MakeUnique<FBSPNode>();
		Node.Right->Bounds = FIntRect(SplitX, Node.Bounds.Min.Y, Node.Bounds.Max.X, Node.Bounds.Max.Y);
	}
	else
	{
		const int32 SplitY = RNG.RandRange(Node.Bounds.Min.Y + MinLeafSize, Node.Bounds.Max.Y - MinLeafSize);
		Node.Left = MakeUnique<FBSPNode>();
		Node.Left->Bounds = FIntRect(Node.Bounds.Min.X, Node.Bounds.Min.Y, Node.Bounds.Max.X, SplitY);
		Node.Right = MakeUnique<FBSPNode>();
		Node.Right->Bounds = FIntRect(Node.Bounds.Min.X, SplitY, Node.Bounds.Max.X, Node.Bounds.Max.Y);
	}

	SplitNode(*Node.Left, MinLeafSize, MaxDepth, CurrentDepth + 1, MaxLeaves, LeafCounter, RNG);
	SplitNode(*Node.Right, MinLeafSize, MaxDepth, CurrentDepth + 1, MaxLeaves, LeafCounter, RNG);
}

void UDungeonBSPGenerator::CollectLeaves(FBSPNode& Node, TArray<FBSPNode*>& OutLeaves) const
{
	if (Node.IsLeaf())
	{
		OutLeaves.Add(&Node);
		return;
	}

	if (Node.Left.IsValid())
	{
		CollectLeaves(*Node.Left, OutLeaves);
	}
	if (Node.Right.IsValid())
	{
		CollectLeaves(*Node.Right, OutLeaves);
	}
}

void UDungeonBSPGenerator::CreateRoomsFromLeaves(const TArray<FBSPNode*>& Leaves, FDungeonGrid& Grid, const UMapTemplate* Template, FRandomStream& RNG) const
{
	constexpr int32 LeafMargin = 1; // celdas de margen interior respecto a los limites de la hoja

	for (FBSPNode* Leaf : Leaves)
	{
		if (!Leaf)
		{
			continue;
		}

		const int32 UsableWidth = Leaf->Bounds.Width() - LeafMargin * 2;
		const int32 UsableHeight = Leaf->Bounds.Height() - LeafMargin * 2;

		if (UsableWidth < 3 || UsableHeight < 3)
		{
			continue; // hoja demasiado pequena para alojar una room util
		}

		const float SizeFraction = RNG.FRandRange(0.6f, 0.9f);
		const int32 RoomWidth = FMath::Max(3, FMath::RoundToInt(UsableWidth * SizeFraction));
		const int32 RoomHeight = FMath::Max(3, FMath::RoundToInt(UsableHeight * SizeFraction));

		const int32 MaxOffsetX = FMath::Max(0, UsableWidth - RoomWidth);
		const int32 MaxOffsetY = FMath::Max(0, UsableHeight - RoomHeight);

		const int32 OffsetX = MaxOffsetX > 0 ? RNG.RandRange(0, MaxOffsetX) : 0;
		const int32 OffsetY = MaxOffsetY > 0 ? RNG.RandRange(0, MaxOffsetY) : 0;

		const int32 RoomMinX = Leaf->Bounds.Min.X + LeafMargin + OffsetX;
		const int32 RoomMinY = Leaf->Bounds.Min.Y + LeafMargin + OffsetY;
		const int32 RoomMaxX = RoomMinX + RoomWidth;
		const int32 RoomMaxY = RoomMinY + RoomHeight;

		FRoomData Room;
		Room.Bounds = FIntRect(RoomMinX, RoomMinY, RoomMaxX, RoomMaxY);

		const int32 RoomId = Grid.Rooms.Add(Room);
		Leaf->RoomIndex = RoomId;

		for (int32 Y = RoomMinY; Y < RoomMaxY; ++Y)
		{
			for (int32 X = RoomMinX; X < RoomMaxX; ++X)
			{
				if (FMapCell* Cell = Grid.GetCell(X, Y))
				{
					Cell->CellType = EAssetFunction::Ground;
					Cell->bIsRoom = true;
					Cell->RoomId = RoomId;
					Cell->bIsEmpty = false;
				}
			}
		}
	}
}

void UDungeonBSPGenerator::ConnectRooms(FDungeonGrid& Grid, const UMapTemplate* Template, FRandomStream& RNG) const
{
	const int32 RoomCount = Grid.Rooms.Num();
	if (RoomCount < 2)
	{
		return;
	}

	// --- Minimum Spanning Tree (Prim) sobre distancias euclidianas entre centros de room ---
	TArray<bool> InMST;
	InMST.Init(false, RoomCount);
	InMST[0] = true;

	TArray<FRoomEdge> MSTEdges;

	for (int32 Step = 0; Step < RoomCount - 1; ++Step)
	{
		FRoomEdge BestEdge;
		BestEdge.DistanceSq = TNumericLimits<float>::Max();
		bool bFound = false;

		for (int32 A = 0; A < RoomCount; ++A)
		{
			if (!InMST[A])
			{
				continue;
			}

			const FVector2D CenterA(Grid.Rooms[A].GridCenter());

			for (int32 B = 0; B < RoomCount; ++B)
			{
				if (InMST[B])
				{
					continue;
				}

				const FVector2D CenterB(Grid.Rooms[B].GridCenter());
				const float DistSq = FVector2D::DistSquared(CenterA, CenterB);

				if (DistSq < BestEdge.DistanceSq)
				{
					BestEdge.RoomA = A;
					BestEdge.RoomB = B;
					BestEdge.DistanceSq = DistSq;
					bFound = true;
				}
			}
		}

		if (!bFound)
		{
			break;
		}

		InMST[BestEdge.RoomB] = true;
		MSTEdges.Add(BestEdge);
	}

	// --- Aristas extra para crear loops/caminos alternativos (Paso C) ---
	TArray<FRoomEdge> CandidateExtraEdges;
	for (int32 A = 0; A < RoomCount; ++A)
	{
		for (int32 B = A + 1; B < RoomCount; ++B)
		{
			const bool bAlreadyInMST = MSTEdges.ContainsByPredicate([A, B](const FRoomEdge& Edge)
			{
				return (Edge.RoomA == A && Edge.RoomB == B) || (Edge.RoomA == B && Edge.RoomB == A);
			});

			if (bAlreadyInMST)
			{
				continue;
			}

			FRoomEdge Edge;
			Edge.RoomA = A;
			Edge.RoomB = B;
			Edge.DistanceSq = FVector2D::DistSquared(FVector2D(Grid.Rooms[A].GridCenter()), FVector2D(Grid.Rooms[B].GridCenter()));
			CandidateExtraEdges.Add(Edge);
		}
	}

	CandidateExtraEdges.Sort([](const FRoomEdge& A, const FRoomEdge& B) { return A.DistanceSq < B.DistanceSq; });

	const int32 ExtraEdgeCount = FMath::RoundToInt(MSTEdges.Num() * RNG.FRandRange(0.15f, 0.25f));
	const int32 ActualExtraEdges = FMath::Min(ExtraEdgeCount, CandidateExtraEdges.Num());

	TArray<FRoomEdge> FinalEdges = MSTEdges;
	for (int32 i = 0; i < ActualExtraEdges; ++i)
	{
		FinalEdges.Add(CandidateExtraEdges[i]);
	}

	const float CorridorWidthUU = Template->SpaceMetrics.MinCorridorWidth;
	const int32 CorridorWidthCells = FMath::Max(1, FMath::RoundToInt(CorridorWidthUU / FMath::Max(1, Grid.CellSize)));

	for (const FRoomEdge& Edge : FinalEdges)
	{
		Grid.Rooms[Edge.RoomA].ConnectedRoomIds.AddUnique(Edge.RoomB);
		Grid.Rooms[Edge.RoomB].ConnectedRoomIds.AddUnique(Edge.RoomA);

		CarveCorridor(Grid, Grid.Rooms[Edge.RoomA].GridCenter(), Grid.Rooms[Edge.RoomB].GridCenter(), CorridorWidthCells, RNG);
	}
}

void UDungeonBSPGenerator::CarveCorridor(FDungeonGrid& Grid, const FIntPoint& From, const FIntPoint& To, int32 CorridorWidthCells, FRandomStream& RNG) const
{
	// Corridor en L: primero X luego Y, o al reves, elegido al azar (Paso C).
	const bool bHorizontalFirst = RNG.GetFraction() < 0.5f;
	const FIntPoint Bend = bHorizontalFirst ? FIntPoint(To.X, From.Y) : FIntPoint(From.X, To.Y);

	CarveLine(Grid, From, Bend, CorridorWidthCells);
	CarveLine(Grid, Bend, To, CorridorWidthCells);
}

void UDungeonBSPGenerator::CarveLine(FDungeonGrid& Grid, const FIntPoint& From, const FIntPoint& To, int32 CorridorWidthCells) const
{
	const int32 HalfWidth = FMath::Max(0, (CorridorWidthCells - 1) / 2);

	const int32 MinX = FMath::Min(From.X, To.X) - HalfWidth;
	const int32 MaxX = FMath::Max(From.X, To.X) + HalfWidth;
	const int32 MinY = FMath::Min(From.Y, To.Y) - HalfWidth;
	const int32 MaxY = FMath::Max(From.Y, To.Y) + HalfWidth;

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			if (FMapCell* Cell = Grid.GetCell(X, Y))
			{
				if (!Cell->bIsRoom) // no pisar celdas que ya pertenecen a una room
				{
					Cell->CellType = EAssetFunction::Ground;
					Cell->bIsEmpty = false;
				}
			}
		}
	}
}

void UDungeonBSPGenerator::AssignRoomTypes(FDungeonGrid& Grid, FRandomStream& RNG) const
{
	const int32 RoomCount = Grid.Rooms.Num();
	if (RoomCount == 0)
	{
		return;
	}

	// --- Start: room cuyo centro tiene menor X+Y (esquina superior-izquierda) ---
	int32 StartIndex = 0;
	int32 BestStartScore = TNumericLimits<int32>::Max();
	for (int32 i = 0; i < RoomCount; ++i)
	{
		const FIntPoint Center = Grid.Rooms[i].GridCenter();
		const int32 Score = Center.X + Center.Y;
		if (Score < BestStartScore)
		{
			BestStartScore = Score;
			StartIndex = i;
		}
	}

	// --- BFS de distancias EN SALTOS (grafo de conectividad, no distancia euclidiana) desde Start ---
	TArray<int32> HopDistance;
	HopDistance.Init(-1, RoomCount);
	TArray<int32> PreviousRoom;
	PreviousRoom.Init(-1, RoomCount);

	HopDistance[StartIndex] = 0;
	TQueue<int32> BFSQueue;
	BFSQueue.Enqueue(StartIndex);

	int32 Current = INDEX_NONE;
	while (BFSQueue.Dequeue(Current))
	{
		for (int32 Neighbor : Grid.Rooms[Current].ConnectedRoomIds)
		{
			if (Grid.Rooms.IsValidIndex(Neighbor) && HopDistance[Neighbor] == -1)
			{
				HopDistance[Neighbor] = HopDistance[Current] + 1;
				PreviousRoom[Neighbor] = Current;
				BFSQueue.Enqueue(Neighbor);
			}
		}
	}

	// --- End: la room mas alejada de Start en numero de saltos ---
	int32 EndIndex = StartIndex;
	int32 MaxHops = -1;
	for (int32 i = 0; i < RoomCount; ++i)
	{
		if (HopDistance[i] > MaxHops)
		{
			MaxHops = HopDistance[i];
			EndIndex = i;
		}
	}

	// --- Camino critico Start -> End (para distinguir ramas secundarias) ---
	TSet<int32> CriticalPath;
	for (int32 Node = EndIndex; Node != -1; Node = PreviousRoom[Node])
	{
		CriticalPath.Add(Node);
		if (Node == StartIndex)
		{
			break;
		}
	}

	Grid.Rooms[StartIndex].Type = ERoomType::Start;
	Grid.Rooms[EndIndex].Type = ERoomType::End;
	Grid.StartRoomIndex = StartIndex;
	Grid.EndRoomIndex = EndIndex;

	// --- Boss: la room mas grande conectada directamente a End o a 1-2 saltos de End ---
	int32 BossIndex = INDEX_NONE;
	int32 BossBestArea = -1;
	for (int32 i = 0; i < RoomCount; ++i)
	{
		if (i == StartIndex || i == EndIndex)
		{
			continue;
		}

		const bool bNearEnd = Grid.Rooms[EndIndex].ConnectedRoomIds.Contains(i) || HopDistance[i] >= FMath::Max(0, MaxHops - 2);
		if (bNearEnd && Grid.Rooms[i].Area() > BossBestArea)
		{
			BossBestArea = Grid.Rooms[i].Area();
			BossIndex = i;
		}
	}

	if (BossIndex != INDEX_NONE)
	{
		Grid.Rooms[BossIndex].Type = ERoomType::Boss;
	}

	// --- Loot: rooms pequenas fuera del camino critico. El resto (fuera del camino critico o
	// grandes) se marca como Normal. ---
	TArray<int32> OffPathAreas;
	for (int32 i = 0; i < RoomCount; ++i)
	{
		if (i != StartIndex && i != EndIndex && i != BossIndex && !CriticalPath.Contains(i))
		{
			OffPathAreas.Add(Grid.Rooms[i].Area());
		}
	}
	OffPathAreas.Sort();
	const int32 MedianArea = OffPathAreas.Num() > 0 ? OffPathAreas[OffPathAreas.Num() / 2] : 0;

	for (int32 i = 0; i < RoomCount; ++i)
	{
		if (i == StartIndex || i == EndIndex || i == BossIndex)
		{
			continue;
		}

		if (!CriticalPath.Contains(i) && Grid.Rooms[i].Area() <= MedianArea)
		{
			Grid.Rooms[i].Type = ERoomType::Loot;
		}
		else
		{
			Grid.Rooms[i].Type = ERoomType::Normal;
		}
	}
}

void UDungeonBSPGenerator::MarkWalls(FDungeonGrid& Grid) const
{
	static const int32 OffsetsX[4] = { 1, -1, 0, 0 };
	static const int32 OffsetsY[4] = { 0, 0, 1, -1 };

	for (int32 Y = 0; Y < Grid.Height; ++Y)
	{
		for (int32 X = 0; X < Grid.Width; ++X)
		{
			FMapCell* Cell = Grid.GetCell(X, Y);
			if (!Cell || !Cell->bIsEmpty)
			{
				continue;
			}

			for (int32 Dir = 0; Dir < 4; ++Dir)
			{
				const FMapCell* Neighbor = Grid.GetCell(X + OffsetsX[Dir], Y + OffsetsY[Dir]);
				if (Neighbor && !Neighbor->bIsEmpty)
				{
					Cell->CellType = EAssetFunction::Wall;
					Cell->bIsEmpty = false;
					break;
				}
			}
		}
	}
}
