#include "US_GameMode.h"
#include "US_GameInstance.h"
#include "US_GameState.h"
#include "SnakePawn.h"
#include "PickUp.h"
#include "Grid.h"
#include "GridMovement.h"
#include "SnakeGameTypes.h"
#include "Hazard.h"
#include "GameFramework/PlayerController.h"
#include "SnakeStageConfig.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

void AUS_GameMode::BeginPlay()
{
	Super::BeginPlay();
	
	CacheGridManager(); 
	
	if (UUS_GameInstance* GI = GetGameInstance<UUS_GameInstance>())
	{
		ActiveGameMode = GI->SelectedGameMode;
		Slot0Type = GI->Slot0Type;
		Slot1Type = GI->Slot1Type;
	}
	
	StartPlayingRun();
	StartMusic();
}

void AUS_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopMusic();
	Super::EndPlay(EndPlayReason);
}

void AUS_GameMode::CacheGridManager()
{
	GridManager = Cast<AGrid>(UGameplayStatics::GetActorOfClass(this, AGrid::StaticClass()));
}

AUS_GameState* AUS_GameMode::GetUSGameState()
{
	return GetGameState<AUS_GameState>();
}

void AUS_GameMode::StartPlayingRun()
{
	BattleResult = EResult::None;

	if (AUS_GameState* GS = GetUSGameState())
	{
		GS->Score = 0;
		GS->SetMatchPhase(ESnakeMatchPhase::Playing);
	}

	LoadStage(0);
}

void AUS_GameMode::SpawnSnake()
{
	if (!SnakePawnClass || !GridManager)
	{
		return;
	}

	if (SpawnedSnakePawn)
	{
		SpawnedSnakePawn->Destroy();
		SpawnedSnakePawn = nullptr;
	}

	const FIntPoint SnakeSpawnCell(GridManager->GridDimensions.X/2, GridManager->GridDimensions.Y/2);
	const FVector SnakeSpawnLocation = GridManager->GridToWorld(SnakeSpawnCell);

	SpawnedSnakePawn = GetWorld()->SpawnActor<ASnakePawn>(
		SnakePawnClass, SnakeSpawnLocation, FRotator::ZeroRotator);

	if (!SpawnedSnakePawn)
	{
		return;
	}
	
	SpawnedSnakePawn->OnFoodConsumed.AddDynamic(this, &AUS_GameMode::HandleFoodConsumed);
	SpawnedSnakePawn->OnSnakeDied.AddDynamic(this, &AUS_GameMode::HandleSnakeDied);
	
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->Possess(SpawnedSnakePawn);
		UE_LOG(LogTemp, Warning, TEXT("Player Controller possessed SnakePawn."));
	}
}

void AUS_GameMode::SpawnFood()
{
	if (!PickUpClass || !GridManager)
	{
		return;
	}

	if (PickUp)
	{
		PickUp->Destroy();
		PickUp = nullptr;
	}

	PickUp = GetWorld()->SpawnActor<APickUp>(
		PickUpClass, FVector::ZeroVector, FRotator::ZeroRotator);
}

void AUS_GameMode::MoveFoodToRandomFreeCell()
{
	if (!GridManager || !PickUp)
	{
		return;
	}

	const TArray<FIntPoint> ForbiddenCells = GetAllSnakeOccupiedCells();

	FIntPoint NewFoodCell;
	if (GridManager->TryGetRandomFreeCell(NewFoodCell, ForbiddenCells))
	{
		PickUp->RespawnFood(NewFoodCell, GridManager->GridToWorld(NewFoodCell));
	}
	else
	{
		PickUp->DeactivateFood();
		UE_LOG(LogTemp, Warning, TEXT("No free cell available for food."));
	}
}

TArray<FIntPoint> AUS_GameMode::GetValidatedPatrolCells(
	const FHazardSpawnData& HazardData,
	const AHazard* SpawnedHazard) const
{
	TArray<FIntPoint> ValidPatrolCells;

	for (const FIntPoint& PatrolCell : HazardData.PatrolCells)
	{
		if (!GridManager->IsInsidePlayableArea(PatrolCell))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Hazard %s ignores patrol cell outside playable area: (%d, %d)."),
				SpawnedHazard ? *SpawnedHazard->GetName() : TEXT("Unknown"),
				PatrolCell.X,
				PatrolCell.Y);
			continue;
		}

		ValidPatrolCells.Add(PatrolCell);
	}

	return ValidPatrolCells;
}

void AUS_GameMode::HandleFoodConsumed(ASnakePawn* EatingSnake, int32 ScoreValue)
{
	PlayEatSound(EatingSnake);

	if (AUS_GameState* GS = GetUSGameState())
	{
		GS->AddScore(ScoreValue);
	}
	
	FoodEatenThiStage++;

	if (AUS_GameState* GS = GetUSGameState())
	{
		GS->FoodEatenThisStage = FoodEatenThiStage;
	}
	
	if (ActiveGameMode != EGameModeType::Battle)
	{
		const FSnakeStageConfig& Stage = Stages[CurrentStageIndex];
	
		if (FoodEatenThiStage >= Stage.FoodToClear)
		{
			AdvanceStage();
			return;
		}
	}

	MoveFoodToRandomFreeCell();
}

void AUS_GameMode::HandleSnakeDied(ASnakePawn* DeadSnake)
{
	PlayCollisionSound(DeadSnake);

	if (ActiveGameMode != EGameModeType::Battle)
	{
		ChangePhase(ESnakeMatchPhase::Outro);
		return;
	}

	const int32 DeadIndex = SpawnedSnakes.IndexOfByKey(DeadSnake);

	if (DeadIndex == 0)
	{
		BattleResult = EResult::Player0Won;
	}
	else if (DeadIndex == 1)
	{
		BattleResult = EResult::Player1Won;
	}
	else
	{
		BattleResult = EResult::None;
	}

	if (AUS_GameState* GS = GetUSGameState())
	{
		GS->BattleResult = BattleResult;
	}

	ChangePhase(ESnakeMatchPhase::Outro);
}

void AUS_GameMode::ChangePhase(const ESnakeMatchPhase NewPhase)
{
	if (AUS_GameState* GS = GetUSGameState())
	{
		GS->SetMatchPhase(NewPhase);
	}
}

void AUS_GameMode::PlayEatSound(ASnakePawn* EatingSnake) const
{
	if (!EatingSnake)
	{
		return;
	}

	if (EatingSnake->IsPlayerControlled())
	{
		if (EatSound)
		{
			UGameplayStatics::PlaySound2D(this, EatSound);
		}
		return;
	}
}

void AUS_GameMode::PlayCollisionSound(ASnakePawn* DeadSnake) const
{
	if (!DeadSnake || !CollisionSound)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(
		this,
		CollisionSound,
		1.f,
		1.f,
		0.f,
		nullptr,
		DeadSnake,
		true);
}

void AUS_GameMode::StartMusic()
{
	if (!AmbientMusic || AmbientMusicComponent)
	{
		return;
	}

	AmbientMusicComponent = UGameplayStatics::SpawnSound2D(
		this,
		AmbientMusic,
		1.f,
		1.f,
		0.f,
		nullptr,
		true,
		false);

	if (AmbientMusicComponent)
	{
		AmbientMusicComponent->OnAudioFinished.AddDynamic(
			this,
			&AUS_GameMode::HandleMusicFinished);
	}
}

void AUS_GameMode::StopMusic()
{
	if (!AmbientMusicComponent)
	{
		return;
	}

	AmbientMusicComponent->OnAudioFinished.RemoveAll(this);
	AmbientMusicComponent->Stop();
	AmbientMusicComponent = nullptr;
}

void AUS_GameMode::HandleMusicFinished()
{
	if (AmbientMusicComponent && AmbientMusic)
	{
		AmbientMusicComponent->Play(0.f);
	}
}

void AUS_GameMode::RestartRun()
{
	BattleResult = EResult::None;
	FoodEatenThiStage = 0;

	if (AUS_GameState* GS = GetUSGameState())
	{
		GS->Score = 0;
		GS->SetMatchPhase(ESnakeMatchPhase::Playing);
	}

	LoadStage(0);
}

void AUS_GameMode::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("MainMenu")));
}

void AUS_GameMode::LoadStage(int32 StageIndex)
{

	if (!Stages.IsValidIndex(StageIndex) || !GridManager)
	{
		return;
	}
	
	CurrentStageIndex = StageIndex;
	FoodEatenThiStage = 0;
		
	const FSnakeStageConfig& Stage = Stages[StageIndex];
		
	GridManager->ApplyStage(Stage);
	
	DestroySpawnedSnakes();
	
	if (ActiveGameMode == EGameModeType::NormalSinglePlayer)
	{
		SpawnSinglePlayerSnake();
	}
	else if (ActiveGameMode == EGameModeType::Battle)
	{
		SpawnBattleSnakes();
	}
	else if (ActiveGameMode == EGameModeType::Coop)
	{
		SpawnCoopSnakes();
	}
	
	SpawnFood();
	MoveFoodToRandomFreeCell();

	if (AUS_GameState* GS = GetUSGameState())
	{
		GS->CurrentStageIndex = CurrentStageIndex;
		GS->FoodEatenThisStage = FoodEatenThiStage;
		GS->FoodToClearStage = Stage.FoodToClear;
	}
}

void AUS_GameMode::AdvanceStage()
{
	const int32 NextStage = CurrentStageIndex + 1;
	
	if (!Stages.IsValidIndex(NextStage))
	{
		ChangePhase(ESnakeMatchPhase::Outro);
		return;
	}
	
	LoadStage(NextStage);
}

ASnakePawn* AUS_GameMode::SpawnSnakeForSlot(
	int32 SlotIndex,
	const FIntPoint& SpawnCell,
	ESnakePlayerSlotType SlotType)
{
	if (!SnakePawnClass || !GridManager)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot spawn snake: missing SnakePawnClass or GridManager."));
		return nullptr;
	}

	const FSnakeStageConfig* Stage = Stages.IsValidIndex(CurrentStageIndex)
		? &Stages[CurrentStageIndex]
		: nullptr;

	const FVector SpawnLocation = GridManager->GridToWorld(SpawnCell);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASnakePawn* NewSnake = GetWorld()->SpawnActor<ASnakePawn>(
		SnakePawnClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (!NewSnake)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn snake for slot %d."), SlotIndex);
		return nullptr;
	}

	NewSnake->OnFoodConsumed.AddDynamic(this, &AUS_GameMode::HandleFoodConsumed);
	NewSnake->OnSnakeDied.AddDynamic(this, &AUS_GameMode::HandleSnakeDied);

	if (Stage)
	{
		NewSnake->ApplyStageSettings(
			Stage->CellSize,
			Stage->GridDimensions,
			Stage->MoveStepTime);
	}

	NewSnake->SetStartGridPosition(SpawnCell);

	if (SlotIndex == 0)
	{
		NewSnake->SetInitialDirection(ESnakeDirection::Right);
	}
	else
	{
		NewSnake->SetInitialDirection(ESnakeDirection::Left);
	}

	NewSnake->ResetSnake();

	if (SlotType == ESnakePlayerSlotType::Human)
	{
		APlayerController* PC = GetPlayerController(SlotIndex);
		
		if (PC)
		{
			NewSnake->SetPlayerSlotIndex(SlotIndex);
			PC->Possess(NewSnake);

			UE_LOG(LogTemp, Warning,
				TEXT("Human player slot %d possessed snake %s with controller %s."),
				SlotIndex,
				*NewSnake->GetName(),
				*PC->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("Could not get/create PlayerController for slot %d."),
				SlotIndex);
		}
	}
	else
	{
		if (!SnakeAIControllerClass)
		{
			UE_LOG(LogTemp, Error, TEXT("SnakeAIControllerClass is not set."));
		}
		else
		{
			FActorSpawnParameters AIParams;
			AIParams.Owner = this;
			AIParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AController* AIController = GetWorld()->SpawnActor<AController>(
				SnakeAIControllerClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				AIParams);

			if (AIController)
			{
				AIController->Possess(NewSnake);
				UE_LOG(LogTemp, Warning, TEXT("AI possessed snake for slot %d."), SlotIndex);
			}
		}
	}

	SpawnedSnakes.Add(NewSnake);
	return NewSnake;
}

void AUS_GameMode::SpawnBattleSnakes()
{
	if (!GridManager)
	{
		return;
	}

	const int32 MidY = GridManager->GridDimensions.Y / 2;

	const FIntPoint PlayerSpawnCell(2, MidY);
	const FIntPoint OpponentSpawnCell(GridManager->GridDimensions.X - 3, MidY);

	ASnakePawn* PlayerSnake = SpawnSnakeForSlot(0, PlayerSpawnCell, Slot0Type);
	ASnakePawn* OpponentSnake = SpawnSnakeForSlot(1, OpponentSpawnCell, Slot1Type);
	//fix splitscreen
	if (UGameViewportClient* VP = GetWorld()->GetGameViewport())
	{
		VP->SetForceDisableSplitscreen(false);
		VP->UpdateActiveSplitscreenType();
	}
}

TArray<FIntPoint> AUS_GameMode::GetAllSnakeOccupiedCells() const
{
	TArray<FIntPoint> Occupied;

	for (const ASnakePawn* Snake : SpawnedSnakes)
	{
		if (!Snake)
		{
			continue;
		}

		Occupied.Append(Snake->GetFoodSpawnForbiddenGridCells());
	}

	return Occupied;
}

bool AUS_GameMode::IsCellOccupiedByOtherSnake(
	const ASnakePawn* AskingSnake,
	const FIntPoint& Cell) const
{
	for (const ASnakePawn* Snake : SpawnedSnakes)
	{
		if (!Snake || Snake == AskingSnake)
		{
			continue;
		}

		if (Snake->GetAllOccupiedGridCells().Contains(Cell))
		{
			return true;
		}
	}

	return false;
}

bool AUS_GameMode::IsCellReachableByOtherSnakeHead(
	const ASnakePawn* AskingSnake,
	const FIntPoint& Cell) const
{
	for (const ASnakePawn* Snake : SpawnedSnakes)
	{
		if (!Snake || Snake == AskingSnake || Snake->IsDead())
		{
			continue;
		}

		for (ESnakeDirection Direction : {
			ESnakeDirection::Up,
			ESnakeDirection::Down,
			ESnakeDirection::Left,
			ESnakeDirection::Right
		})
		{
			if (Snake->CanMoveInDirection(Direction) &&
				Snake->GetNextCellForDirection(Direction) == Cell)
			{
				return true;
			}
		}
	}

	return false;
}

void AUS_GameMode::DestroySpawnedSnakes()
{
	for (ASnakePawn* Snake : SpawnedSnakes)
	{
		if (Snake)
		{
			Snake->Destroy();
		}
	}

	SpawnedSnakes.Empty();
	SpawnedSnakePawn = nullptr;
}

void AUS_GameMode::SpawnSinglePlayerSnake()
{
	if (!GridManager)
	{
		return;
	}

	const FIntPoint SpawnCell(
		GridManager->GridDimensions.X / 2,
		GridManager->GridDimensions.Y / 2);

	ASnakePawn* Snake = SpawnSnakeForSlot(0, SpawnCell, ESnakePlayerSlotType::Human);
	//Fix splitscreen persisting in singlepalyer after playing multiplayer
	if (UGameViewportClient* VP = GetWorld()->GetGameViewport())
	{
		VP->SetForceDisableSplitscreen(true);
		VP->UpdateActiveSplitscreenType();
	}
	
	SpawnedSnakePawn = Snake;
}

void AUS_GameMode::SpawnCoopSnakes()
{
	if (!GridManager)
	{
		return;
	}

	const int32 MidY = GridManager->GridDimensions.Y / 2;

	const FIntPoint Player1SpawnCell(2, MidY);
	const FIntPoint Player2SpawnCell(GridManager->GridDimensions.X - 3, MidY);

	SpawnSnakeForSlot(0, Player1SpawnCell, Slot0Type);
	SpawnSnakeForSlot(1, Player2SpawnCell, Slot1Type);
	if (UGameViewportClient* VP = GetWorld()->GetGameViewport())
	{
		VP->SetForceDisableSplitscreen(false);
		VP->UpdateActiveSplitscreenType();
	}
}

APlayerController* AUS_GameMode::GetPlayerController(int32 SlotIndex)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, SlotIndex);

	if (!PC && SlotIndex > 0)
	{
		PC = UGameplayStatics::CreatePlayer(this, SlotIndex, true);

		UE_LOG(LogTemp, Warning,
			TEXT("Created local player %d. Controller: %s"),
			SlotIndex,
			PC ? *PC->GetName() : TEXT("None"));
	}
	return PC;
}
