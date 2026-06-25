
#include "Grid.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "SnakeStageConfig.h"
#include "Components/SceneComponent.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	FloorInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>("FloorInstances");
	FloorInstances->SetupAttachment(RootComponent);
	FloorInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WallInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>("WallInstances");
	WallInstances->SetupAttachment(RootComponent);
	WallInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AGrid::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	RebuildGrid();
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();
	
	RebuildGrid();
}

void AGrid::RebuildGrid()
{
	RebuildBlockedCells();
	RebuildVisualInstances();
}

void AGrid::RebuildBlockedCells()
{
	BlockedCells.Empty();

	if (!bGenerateBorderWalls)
	{
		return;
	}

	for (int32 X = 0; X < GridDimensions.X; ++X)
	{
		BlockedCells.Add(FIntPoint(X, 0));
		BlockedCells.Add(FIntPoint(X, GridDimensions.Y - 1));
	}

	for (int32 Y = 0; Y < GridDimensions.Y; ++Y)
	{
		BlockedCells.Add(FIntPoint(0, Y));
		BlockedCells.Add(FIntPoint(GridDimensions.X - 1, Y));
	}
}

void AGrid::ClearVisualInstances()
{
		FloorInstances->ClearInstances();
		WallInstances->ClearInstances();
}

void AGrid::RebuildVisualInstances()
{
	ClearVisualInstances();

	for (int32 Y = 0; Y < GridDimensions.Y; ++Y)
	{
		for (int32 X = 0; X < GridDimensions.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			const FVector WorldLocation = GridToWorld(Cell);
			
			FloorInstances->AddInstance(FTransform(WorldLocation));

			if (BlockedCells.Contains(Cell))
			{
				const FVector WallLocation = WorldLocation + FVector(0.f, 0.f, WallZOffset);
				WallInstances->AddInstance(FTransform(WallLocation));
			}
		}
	}
}

void AGrid::GenerateBorderWalls()
{
	BlockedCells.Empty();
	
	for (int32 X = 0; X < GridDimensions.X; ++X)
	{
		BlockedCells.Add(FIntPoint(X, 0));
		BlockedCells.Add(FIntPoint(X, GridDimensions.Y - 1));
	}
	
	for (int32 Y = 1; Y < GridDimensions.Y - 1; ++Y)
	{
		BlockedCells.Add(FIntPoint(0, Y));
		BlockedCells.Add(FIntPoint(GridDimensions.X - 1, Y));
	}
}

FVector AGrid::GridToWorld(const FIntPoint& GridCell) const
{
	return GridOrigin + FVector(GridCell.X * CellSize, GridCell.Y * CellSize, 0.f);
}

bool AGrid::IsInsidePlayableArea(const FIntPoint& GridCell) const
{
	return GridCell.X > 0 && GridCell.X < GridDimensions.X - 1
		&& GridCell.Y > 0 && GridCell.Y < GridDimensions.Y - 1;

}

bool AGrid::IsBlockedCell(const FIntPoint& GridCell) const
{
	return BlockedCells.Contains(GridCell);
}

bool AGrid::TryGetRandomFreeCell(FIntPoint& OutCell, const TArray<FIntPoint>& ForbiddenCells, int32 MaxAttempts) const
{
	TSet<FIntPoint> ForbiddenSet(ForbiddenCells);

	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		const FIntPoint Candidate(
			FMath::RandRange(1, GridDimensions.X - 2),
			FMath::RandRange(1, GridDimensions.Y - 2));

		if (BlockedCells.Contains(Candidate) || ForbiddenSet.Contains(Candidate))
		{
			continue;
		}

		OutCell = Candidate;
		return true;
	}

	for (int32 Y = 1; Y < GridDimensions.Y - 1; ++Y)
	{
		for (int32 X = 1; X < GridDimensions.X - 1; ++X)
		{
			const FIntPoint Candidate(X, Y);
			if (!BlockedCells.Contains(Candidate) && !ForbiddenSet.Contains(Candidate))
			{
				OutCell = Candidate;
				return true;
			}
		}
	}

	return false;
}

void AGrid::ApplyStage(const FSnakeStageConfig& Stage)
{
	GridDimensions = Stage.GridDimensions;
	CellSize = Stage.CellSize;
	
	ClearVisualInstances();
	BlockedCells.Empty();
	GenerateBorderWalls();
	
	if (Stage.Pattern == 1) GenerateBorderWalls();
	if (Stage.Pattern == 2) GenerateBorderWalls();
	
	RebuildVisualInstances();
}
