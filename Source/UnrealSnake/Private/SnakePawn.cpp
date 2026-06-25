#include "SnakePawn.h"
#include "PickUp.h"
#include "Hazard.h"
#include "SnakeBodyComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "US_GameMode.h"
#include "US_GameState.h"
#include "Kismet/GameplayStatics.h"

ASnakePawn::ASnakePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(CollisionSphereRadius);
	CollisionSphere->SetCollisionProfileName(TEXT("Pawn"));
	CollisionSphere->SetGenerateOverlapEvents(true);

	SnakeBodyComponent = CreateDefaultSubobject<USnakeBodyComponent>(TEXT("SnakeBodyComponent"));

	SnakeHeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SnakeHeadMesh"));
	SnakeHeadMesh->SetupAttachment(RootComponent);
	SnakeHeadMesh->SetRelativeLocation(FVector::ZeroVector);
	SnakeHeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SnakeHeadMesh->SetSimulatePhysics(false);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 800.f;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	bUseControllerRotationYaw = false;
}

void ASnakePawn::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this,
		&ASnakePawn::HandleHeadOverlap);

	if (!GridManager)
	{
		CacheGridManager();
		if (!GridManager)
		{
			UE_LOG(LogTemp, Error, TEXT("SnakePawn could not find GridManager in world."))
		}
		else
		{
			GridDimensions = GridManager->GridDimensions;
		}
	}

	RequestedDirection = CurrentDirection;
	UpdateDirection(CurrentDirection);

	CurrentGridPosition = GetClampedStartGridPosition();
	PendingNextGridPosition = CurrentGridPosition;

	const FVector StartLocation = GridToWorldLocation(CurrentGridPosition);
	SetActorLocation(StartLocation);

	StepStartWorldLocation = StartLocation;
	StepTargetWorldLocation = StartLocation;
	MoveInterpolationProgress = 0.f;
	bIsMovingToTarget = false;

	if (SnakeBodyComponent)
	{
		SnakeBodyComponent->SetGridContext(GridManager, CellSize, GridOrigin);
		SnakeBodyComponent->ResetBody(
			InitialBodyLength,
			CurrentGridPosition,
			DirectionToGridOffset(CurrentDirection) * -1,
			0.f);
	}
}

void ASnakePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	TickGridMovement(DeltaTime);

	if (bShowDebugCollision)
	{
		DrawDebugInfo();
	}
}

void ASnakePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	SetupEnhancedInput();
}

void ASnakePawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	SetupEnhancedInput();
}

void ASnakePawn::SetupEnhancedInput()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!Subsystem)
	{
		return;
	}

	if (KeyboardInputMapping)
	{
		Subsystem->RemoveMappingContext(KeyboardInputMapping);
	}

	if (GamepadInputMapping)
	{
		Subsystem->RemoveMappingContext(GamepadInputMapping);
	}

	const int32 LocalPlayerIndex = LocalPlayer->GetControllerId();

	UInputMappingContext* MappingToUse = nullptr;

	if (PlayerSlotIndex == 0)
	{
		MappingToUse = KeyboardInputMapping;
	}
	else if (PlayerSlotIndex == 1)
	{
		MappingToUse = GamepadInputMapping;
	}

	if (MappingToUse)
	{
		Subsystem->AddMappingContext(MappingToUse, 0);

		UE_LOG(LogTemp, Warning,
		       TEXT("Snake slot %d added input mapping %s for LocalPlayer ControllerId %d."),
		       PlayerSlotIndex,
		       *MappingToUse->GetName(),
		       LocalPlayerIndex);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("Snake slot %d has no input mapping assigned."),
		       PlayerSlotIndex);
	}
}

void ASnakePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent)
	{
		return;
	}

	if (TurnKeyboardAction)
	{
		EnhancedInputComponent->BindAction(
			TurnKeyboardAction,
			ETriggerEvent::Triggered,
			this,
			&ASnakePawn::Input_TurnKeyboard);
	}

	if (TurnGamepadAction)
	{
		EnhancedInputComponent->BindAction(
			TurnGamepadAction,
			ETriggerEvent::Triggered,
			this,
			&ASnakePawn::Input_TurnGamepad);

		EnhancedInputComponent->BindAction(
			TurnGamepadAction,
			ETriggerEvent::Completed,
			this,
			&ASnakePawn::Input_TurnGamepadCompleted);
	}
}

void ASnakePawn::Input_TurnKeyboard(const FInputActionValue& Value)
{
	HandleTurnVector(Value.Get<FVector2D>());
}

void ASnakePawn::Input_TurnGamepad(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	constexpr float DeadZone = 0.5f;

	if (Input.SizeSquared() < DeadZone * DeadZone)
	{
		return;
	}

	if (!bGamepadStickTurnReady)
	{
		return;
	}

	if (HandleTurnVector(Input))
	{
		bGamepadStickTurnReady = false;
	}
}

void ASnakePawn::Input_TurnGamepadCompleted(const FInputActionValue& Value)
{
	bGamepadStickTurnReady = true;
}

void ASnakePawn::Input_TryTurnUp(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		RequestDirection(ESnakeDirection::Up);
	}
}

void ASnakePawn::Input_TryTurnDown(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		RequestDirection(ESnakeDirection::Down);
	}
}

void ASnakePawn::Input_TryTurnLeft(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		RequestDirection(ESnakeDirection::Left);
	}
}

void ASnakePawn::Input_TryTurnRight(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		RequestDirection(ESnakeDirection::Right);
	}
}

bool ASnakePawn::HandleTurnVector(const FVector2D& Input)
{
	if (Input.IsNearlyZero())
	{
		return false;
	}

	ESnakeDirection DesiredDirection;

	if (FMath::Abs(Input.X) > FMath::Abs(Input.Y))
	{
		DesiredDirection = Input.X > 0.f
			                   ? ESnakeDirection::Right
			                   : ESnakeDirection::Left;
	}
	else
	{
		DesiredDirection = Input.Y > 0.f
			                   ? ESnakeDirection::Up
			                   : ESnakeDirection::Down;
	}

	return RequestDirection(DesiredDirection);
}

FVector ASnakePawn::GetVectorFromDirection(ESnakeDirection Direction) const
{
	switch (Direction)
	{
	case ESnakeDirection::Up: return FVector::ForwardVector; // +X
	case ESnakeDirection::Down: return -FVector::ForwardVector; // -X
	case ESnakeDirection::Left: return -FVector::RightVector; // -Y
	case ESnakeDirection::Right: return FVector::RightVector; // +Y
	default: return FVector::ZeroVector;
	}
}

void ASnakePawn::TickGridMovement(float DeltaTime)
{
	if (MoveStepTime <= 0.f)
	{
		return;
	}

	if (!bIsMovingToTarget)
	{
		StartNewMoveStep();
	}

	MoveInterpolationProgress += DeltaTime / (MoveStepTime / 2);
	const float Alpha = FMath::Clamp(MoveInterpolationProgress, 0.f, 1.f);
	MoveToTimer += DeltaTime;

	const FVector NewHeadLocation = FMath::Lerp(StepStartWorldLocation, StepTargetWorldLocation, Alpha);
	SetActorLocation(NewHeadLocation, false);

	if (SnakeBodyComponent)
	{
		SnakeBodyComponent->UpdateSegments(Alpha);
	}

	if (MoveToTimer >= MoveStepTime)
	{
		MoveToTimer = 0.f;
		const FIntPoint OldHeadGridPosition = CurrentGridPosition;
		CurrentGridPosition = PendingNextGridPosition;

		if (SnakeBodyComponent)
		{
			SnakeBodyComponent->FinishMoveStep(OldHeadGridPosition);
		}
		SetActorLocation(StepTargetWorldLocation, false);
		bIsMovingToTarget = false;
	}
}

void ASnakePawn::StartNewMoveStep()
{
	HandleDirectionChange();

	const FIntPoint GridOffset = DirectionToGridOffset(CurrentDirection);
	PendingNextGridPosition = CurrentGridPosition + GridOffset;

	FString UnsafeMoveReason;
	if (TryGetUnsafeMoveReason(PendingNextGridPosition, UnsafeMoveReason))
	{
		HandleSnakeDeath(UnsafeMoveReason);
		return;
	}

	if (SnakeBodyComponent)
	{
		SnakeBodyComponent->PrepareMoveTargets(CurrentGridPosition);
	}

	StepStartWorldLocation = GridToWorldLocation(CurrentGridPosition);
	StepTargetWorldLocation = GridToWorldLocation(PendingNextGridPosition);
	MoveInterpolationProgress = 0.f;
	bIsMovingToTarget = true;
}

bool ASnakePawn::IsValidTurn(ESnakeDirection NewDirection) const
{
	if (NewDirection == CurrentDirection)
	{
		return false;
	}
	switch (CurrentDirection)
	{
	case ESnakeDirection::Up:
		return NewDirection != ESnakeDirection::Down;
	case ESnakeDirection::Down:
		return NewDirection != ESnakeDirection::Up;
	case ESnakeDirection::Left:
		return NewDirection != ESnakeDirection::Right;
	case ESnakeDirection::Right:
		return NewDirection != ESnakeDirection::Left;
	default:
		return false; // Should never happen, but we return false just in case
	}
}

void ASnakePawn::UpdateDirection(ESnakeDirection NewDirection)
{
	switch (NewDirection)
	{
	case ESnakeDirection::Up: SetActorRotation(FRotator(0.f, 0.f, 0.f));
		break;
	case ESnakeDirection::Down: SetActorRotation(FRotator(0.f, 180.f, 0.f));
		break;
	case ESnakeDirection::Left: SetActorRotation(FRotator(0.f, -90.f, 0.f));
		break;
	case ESnakeDirection::Right: SetActorRotation(FRotator(0.f, 90.f, 0.f));
		break;
	}
}

void ASnakePawn::HandleDirectionChange()
{
	if (CurrentDirection != RequestedDirection && IsValidTurn(RequestedDirection))
	{
		CurrentDirection = RequestedDirection;
		UpdateDirection(CurrentDirection);
		UE_LOG(LogTemp, Verbose, TEXT("Direction changed to: %s"), *UEnum::GetValueAsString(CurrentDirection));
	}
}

void ASnakePawn::DrawDebugInfo()
{
	if (!bShowDebugCollision)
	{
		return;
	}

	if (CollisionSphere)
	{
		DrawDebugSphere(
			GetWorld(),
			CollisionSphere->GetComponentLocation(),
			CollisionSphere->GetScaledSphereRadius(),
			16,
			FColor::Green,
			false,
			-1.f,
			0,
			2.0f
		);
	}

	FVector ForwardStart = GetActorLocation();
	FVector ForwardEnd = ForwardStart + GetActorForwardVector() * 100.f; // 100 units in front
	DrawDebugDirectionalArrow(GetWorld(), ForwardStart, ForwardEnd, 50.f, FColor::Blue, false, -1.f, 0, 3.0f);

	FVector RightStart = GetActorLocation();
	FVector RightEnd = RightStart + GetActorRightVector() * 100.f; // 100 units to the right
	DrawDebugDirectionalArrow(GetWorld(), RightStart, RightEnd, 50.f, FColor::Red, false, -1.f, 0, 3.0f);

	if (SnakeBodyComponent)
	{
		SnakeBodyComponent->DrawDebugBody();
	}
}

FIntPoint ASnakePawn::DirectionToGridOffset(ESnakeDirection Direction)
{
	switch (Direction)
	{
	case ESnakeDirection::Up: return FIntPoint(1, 0); // +X
	case ESnakeDirection::Down: return FIntPoint(-1, 0); // -X
	case ESnakeDirection::Left: return FIntPoint(0, -1); // -Y
	case ESnakeDirection::Right: return FIntPoint(0, 1); // +Y
	default: return FIntPoint(0, 0);
	}
}

FVector ASnakePawn::GridToWorldLocation(const FIntPoint& GridPosition) const
{
	if (GridManager)
	{
		return GridManager->GridToWorld(GridPosition);
	}
	return GridOrigin + FVector(GridPosition.X * CellSize, GridPosition.Y * CellSize, 0.f);
}

bool ASnakePawn::WouldHitWall(const FIntPoint& NextCell) const
{
	if (!GridManager)
	{
		return false;
	}

	return !GridManager->IsInsidePlayableArea(NextCell)
		|| GridManager->IsBlockedCell(NextCell);
}

bool ASnakePawn::WouldHitSelf(const FIntPoint& NextCell) const
{
	return SnakeBodyComponent && SnakeBodyComponent->WouldHitSelf(NextCell);
}

bool ASnakePawn::TryGetUnsafeMoveReason(const FIntPoint& NextCell, FString& OutReason) const
{
	if (GridManager)
	{
		if (!GridManager->IsInsidePlayableArea(NextCell))
		{
			OutReason = TEXT("Outside playable area");
			return true;
		}

		if (GridManager->IsBlockedCell(NextCell))
		{
			OutReason = TEXT("Blocked wall cell");
			return true;
		}
	}

	if (WouldHitSelf(NextCell))
	{
		OutReason = TEXT("Self collision");
		return true;
	}

	const AUS_GameMode* SnakeGM = GetWorld()->GetAuthGameMode<AUS_GameMode>();
	if (SnakeGM && SnakeGM->IsCellOccupiedByOtherSnake(this, NextCell))
	{
		OutReason = TEXT("Other snake collision");
		return true;
	}

	OutReason.Reset();
	return false;
}

void ASnakePawn::GrowSnake(int32 Amount)
{
	if (SnakeBodyComponent)
	{
		SnakeBodyComponent->Grow(Amount);
	}
}

void ASnakePawn::HandleSnakeDeath(const FString& DeathReason)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	const int32 PendingGrowth = SnakeBodyComponent
		                            ? SnakeBodyComponent->GetPendingGrowth()
		                            : 0;

	UE_LOG(LogTemp, Warning,
	       TEXT(
		       "Snake died: %s. Snake=%s Slot=%d CurrentCell=(%d,%d) NextCell=(%d,%d) Direction=%s Requested=%s PendingGrowth=%d"
	       ),
	       *DeathReason,
	       *GetName(),
	       PlayerSlotIndex,
	       CurrentGridPosition.X,
	       CurrentGridPosition.Y,
	       PendingNextGridPosition.X,
	       PendingNextGridPosition.Y,
	       *UEnum::GetValueAsString(CurrentDirection),
	       *UEnum::GetValueAsString(RequestedDirection),
	       PendingGrowth);

	OnSnakeDied.Broadcast(this);
}

void ASnakePawn::Eliminate()
{
	HandleSnakeDeath();
}

void ASnakePawn::EliminateByHazard_Implementation(AHazard* Hazard)
{
	HandleSnakeDeath(Hazard
		                 ? FString::Printf(TEXT("Hazard collision: %s"), *Hazard->GetName())
		                 : FString(TEXT("Hazard collision")));
}

void ASnakePawn::HandleHeadOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bIsDead || !OtherActor || OtherActor == this)
	{
		return;
	}

	if (AHazard* Hazard = Cast<AHazard>(OtherActor))
	{
		Hazard->EliminateTarget(this, AHazard::HeadHitIndex);
	}
}

void ASnakePawn::ResetSnake()
{
	const FIntPoint SpawnCell = GetClampedStartGridPosition();
	CurrentGridPosition = SpawnCell;
	PendingNextGridPosition = SpawnCell;

	CurrentDirection = StartDirection;
	RequestedDirection = StartDirection;
	UpdateDirection(CurrentDirection);

	const FVector ResetLocation = GridToWorldLocation(CurrentGridPosition);
	SetActorLocation(ResetLocation);

	StepStartWorldLocation = ResetLocation;
	StepTargetWorldLocation = ResetLocation;
	MoveInterpolationProgress = 0.f;
	bIsMovingToTarget = false;
	bIsDead = false;

	if (SnakeBodyComponent)
	{
		SnakeBodyComponent->SetGridContext(GridManager, CellSize, GridOrigin);
		SnakeBodyComponent->ResetBody(
			InitialBodyLength,
			CurrentGridPosition,
			DirectionToGridOffset(CurrentDirection) * -1,
			1.f);
	}
}

FIntPoint ASnakePawn::GetClampedStartGridPosition() const
{
	return FIntPoint(
		FMath::Clamp(StartGridPosition.X, 1, GridDimensions.X - 2),
		FMath::Clamp(StartGridPosition.Y, 1, GridDimensions.Y - 2)
	);
}

void ASnakePawn::HandleFoodOverlap(APickUp* FoodActor)
{
	if (bIsDead || !FoodActor)
	{
		return;
	}

	GrowSnake(1);
	FoodActor->DeactivateFood();

	OnFoodConsumed.Broadcast(this, 10);
}

TArray<FIntPoint> ASnakePawn::GetAllOccupiedGridCells() const
{
	TArray<FIntPoint> Occupied = SnakeBodyComponent
		                             ? SnakeBodyComponent->GetBodyGridPositions()
		                             : TArray<FIntPoint>();
	Occupied.Insert(CurrentGridPosition, 0);
	return Occupied;
}

TArray<FIntPoint> ASnakePawn::GetFoodSpawnForbiddenGridCells() const
{
	TArray<FIntPoint> ForbiddenCells = GetAllOccupiedGridCells();

	if (bIsMovingToTarget)
	{
		ForbiddenCells.AddUnique(PendingNextGridPosition);
	}

	return ForbiddenCells;
}

bool ASnakePawn::IsNextGridCellSafe(const FIntPoint& NextCell, bool bCheckOtherSnakes) const
{
	if (WouldHitWall(NextCell) || WouldHitSelf(NextCell))
	{
		return false;
	}

	if (bCheckOtherSnakes)
	{
		const AUS_GameMode* SnakeGM = GetWorld()->GetAuthGameMode<AUS_GameMode>();
		if (SnakeGM && SnakeGM->IsCellOccupiedByOtherSnake(this, NextCell))
		{
			return false;
		}
	}

	return true;
}

void ASnakePawn::GetProjectedOccupiedGridCellsAfterMove(
	const FIntPoint& NextCell,
	TSet<FIntPoint>& OutOccupiedCells) const
{
	if (!SnakeBodyComponent)
	{
		OutOccupiedCells.Reset();
		OutOccupiedCells.Add(NextCell);
		return;
	}

	SnakeBodyComponent->GetProjectedOccupiedCellsAfterMove(
		CurrentGridPosition,
		NextCell,
		OutOccupiedCells);
}

void ASnakePawn::CacheGridManager()
{
	GridManager = Cast<AGrid>(UGameplayStatics::GetActorOfClass(GetWorld(), AGrid::StaticClass()));
}

void ASnakePawn::ApplyStageSettings(float NewCellSize, FIntPoint NewGridDimensions, float NewMoveStepTime)
{
	CellSize = NewCellSize;
	GridDimensions = NewGridDimensions;
	MoveStepTime = NewMoveStepTime;

	if (SnakeBodyComponent)
	{
		SnakeBodyComponent->SetGridContext(GridManager, CellSize, GridOrigin);
	}
}

void ASnakePawn::SetStartGridPosition(FIntPoint NewStartGridPosition)
{
	StartGridPosition = NewStartGridPosition;
}

bool ASnakePawn::RequestDirection(ESnakeDirection NewDirection)
{
	if (NewDirection == CurrentDirection)
	{
		return false;
	}

	if (!CanRequestDirection(NewDirection))
	{
		return false;
	}

	RequestedDirection = NewDirection;
	return true;
}

bool ASnakePawn::CanRequestDirection(ESnakeDirection NewDirection) const
{
	if (bIsDead)
	{
		return false;
	}

	if (NewDirection == CurrentDirection)
	{
		return false;
	}

	return IsValidTurn(NewDirection);
}

bool ASnakePawn::CanMoveInDirection(ESnakeDirection Direction) const
{
	if (bIsDead)
	{
		return false;
	}
	if (Direction == CurrentDirection)
	{
		return true;
	}

	return IsValidTurn(Direction);
}

FIntPoint ASnakePawn::GetNextCellForDirection(ESnakeDirection Direction) const
{
	return CurrentGridPosition + DirectionToGridOffset(Direction);
}

void ASnakePawn::SetInitialDirection(ESnakeDirection NewDirection)
{
	StartDirection = NewDirection;
	CurrentDirection = NewDirection;
	RequestedDirection = NewDirection;
	UpdateDirection(NewDirection);
}

void ASnakePawn::SetPlayerSlotIndex(int32 NewPlayerSlotIndex)
{
	PlayerSlotIndex = NewPlayerSlotIndex;
}
