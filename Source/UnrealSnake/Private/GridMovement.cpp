

#include "GridMovement.h"

#include "Grid.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"


UGridMovement::UGridMovement()
{
	PrimaryComponentTick.bCanEverTick = true;
	GridSize = 100.f;
	MovementSpeed = GridSize;
	CurrentTargetIndex = 0;
}

void UGridMovement::BeginPlay()
{
	Super::BeginPlay();
	CurrentTargetLocation = GetOwner()->GetActorLocation();

	CacheGridManager();
	SnapToCurrentPatrolPoint();

	if (bAutoStartPatrol)
	{
		StartPatrol();
	}
}

void UGridMovement::CacheGridManager()
{
	if (GridManager)
	{
		return;
	}

	TArray<AActor*> FoundManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), FoundManagers);
	if (FoundManagers.Num() > 0)
	{
		GridManager = Cast<AGrid>(FoundManagers[0]);
		if (GridManager)
		{
			GridSize = GridManager->GetCellSize();
		}
	}
}

void UGridMovement::SnapToCurrentPatrolPoint()
{
	if (GridManager && PatrolPoints.Num() > 0)
	{
		CurrentTargetIndex = FMath::Clamp(CurrentTargetIndex, 0, PatrolPoints.Num() - 1);
		GetOwner()->SetActorLocation(GridManager->GridToWorld(PatrolPoints[0]));
		CurrentTargetLocation = GetOwner()->GetActorLocation();
		bIsMovingToTarget = false;
	}
}

void UGridMovement::MoveInGridDirection(const FVector& Direction)
{
	if (AActor* Owner = GetOwner())
	{
		FVector MoveDelta = Direction * GridSize;
		Owner->AddActorWorldOffset(MoveDelta, true);
	}
}

void UGridMovement::MoveToNextPatrolPoint()
{
	if (!GridManager || PatrolPoints.Num() < 2 || bIsMovingToTarget) return;
	
	CurrentTargetIndex = (CurrentTargetIndex + 1) % PatrolPoints.Num();
	CurrentTargetLocation = GridManager->GridToWorld(PatrolPoints[CurrentTargetIndex]);
	bIsMovingToTarget = true;
	CurrentDirection = (CurrentTargetLocation - GetOwner()->GetActorLocation()).GetSafeNormal();
}

void UGridMovement::SetPatrolPoints(const TArray<FIntPoint>& GridPoints)
{
	PatrolPoints = GridPoints;
	CurrentTargetIndex = 0;
	CacheGridManager();
	SnapToCurrentPatrolPoint();
}

void UGridMovement::ConfigurePatrol(
	const TArray<FIntPoint>& GridPoints,
	float NewMovementSpeed,
	float NewPatrolWaitSeconds,
	bool bShouldAutoStart)
{
	StopPatrol();
	SetMovementSpeed(NewMovementSpeed);
	PatrolWaitSeconds = FMath::Max(NewPatrolWaitSeconds, 0.f);
	bAutoStartPatrol = bShouldAutoStart;
	SetPatrolPoints(GridPoints);

	if (bAutoStartPatrol)
	{
		StartPatrol();
	}
}

void UGridMovement::StartPatrol()
{
	if (PatrolPoints.Num() < 2)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PatrolTimerHandle);
	}

	bPatrolActive = true;
	ScheduleNextPatrolMove();
}

void UGridMovement::StopPatrol()
{
	bPatrolActive = false;
	bIsMovingToTarget = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PatrolTimerHandle);
	}
}

void UGridMovement::SetTargetLocation(FVector NewLocation)
{
	CurrentTargetLocation = NewLocation;
	bIsMovingToTarget = true;
	CurrentDirection = (CurrentTargetLocation - GetOwner()->GetActorLocation()).GetSafeNormal();
}

void UGridMovement::SetMovementSpeed(float NewMovementSpeed)
{
	MovementSpeed = FMath::Max(NewMovementSpeed, 1.f);
}

void UGridMovement::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsMovingToTarget)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FVector CurrentLocation = Owner->GetActorLocation();
	const FVector NewLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		CurrentTargetLocation,
		DeltaTime,
		MovementSpeed);

	Owner->SetActorLocation(NewLocation, true);

	if (FVector::DistSquared(NewLocation, CurrentTargetLocation) <= FMath::Square(AcceptanceDistance))
	{
		Owner->SetActorLocation(CurrentTargetLocation, true);
		bIsMovingToTarget = false;
		CurrentDirection = FVector::ZeroVector;
		ScheduleNextPatrolMove();
	}
}

void UGridMovement::ScheduleNextPatrolMove()
{
	if (!bPatrolActive || bIsMovingToTarget || PatrolPoints.Num() < 2)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (PatrolWaitSeconds <= 0.f)
	{
		MoveToNextPatrolPoint();
		return;
	}

	World->GetTimerManager().SetTimer(
		PatrolTimerHandle,
		this,
		&UGridMovement::MoveToNextPatrolPoint,
		PatrolWaitSeconds,
		false);
}

