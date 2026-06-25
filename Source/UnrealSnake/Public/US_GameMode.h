#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SnakeGameTypes.h"
#include "US_GameMode.generated.h"

class ASnakePawn;
class APickUp;
class AHazard;
class AGrid;
class AUS_GameState;
class UAudioComponent;
class USoundBase;
struct FHazardSpawnData;
struct FSnakeStageConfig;

UCLASS()
class UNREALSNAKE_API AUS_GameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Snake")
	void StartPlayingRun();

	UFUNCTION(BlueprintCallable, Category = "Snake")
	void ReturnToMainMenu();
	
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void RestartRun();
	
	ASnakePawn* SpawnSnakeForSlot(int32 SlotIndex, const FIntPoint& SpawnCell, ESnakePlayerSlotType SlotType);
	void SpawnBattleSnakes();
	TArray<FIntPoint> GetAllSnakeOccupiedCells() const;
	
	bool IsCellOccupiedByOtherSnake(const ASnakePawn* AskingSnake, const FIntPoint& Cell) const;
	bool IsCellReachableByOtherSnakeHead(const ASnakePawn* AskingSnake, const FIntPoint& Cell) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Snake")
	TSubclassOf<ASnakePawn> SnakePawnClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Snake")
	TSubclassOf<APickUp> PickUpClass;

	UPROPERTY()
	TObjectPtr<ASnakePawn> SpawnedSnakePawn;
	
	UPROPERTY()
	TArray<TObjectPtr<ASnakePawn>> SpawnedSnakes;
	
	UPROPERTY(EditDefaultsOnly, Category = "Snake|AI")
	TSubclassOf<AController> SnakeAIControllerClass;
	
	UPROPERTY(EditAnywhere, Category = "Snake|Mode")
	bool bUseBattleMode = true;
	
	UPROPERTY(EditAnywhere, Category = "Snake|Battle")
	ESnakePlayerSlotType Slot0Type = ESnakePlayerSlotType::Human;

	UPROPERTY(EditAnywhere, Category = "Snake|Battle")
	ESnakePlayerSlotType Slot1Type = ESnakePlayerSlotType::AI;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Mode", meta = (AllowPrivateAccess = "true"))
	EGameModeType ActiveGameMode = EGameModeType::NormalSinglePlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Battle", meta = (AllowPrivateAccess = "true"))
	EResult BattleResult = EResult::None;
	
	UPROPERTY()
	TObjectPtr<APickUp> PickUp;
	
	UPROPERTY()
	TObjectPtr<AGrid> GridManager;
	
	UPROPERTY(EditAnywhere, Category = "Stages")
	TArray<FSnakeStageConfig> Stages;

	UPROPERTY(EditDefaultsOnly, Category = "Snake|Audio")
	TObjectPtr<USoundBase> EatSound;

	UPROPERTY(EditDefaultsOnly, Category = "Snake|Audio")
	TObjectPtr<USoundBase> CollisionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Snake|Audio")
	TObjectPtr<USoundBase> AmbientMusic;

	UPROPERTY()
	TObjectPtr<UAudioComponent> AmbientMusicComponent;
	
	int32 CurrentStageIndex = 0;
	int32 FoodEatenThiStage = 0;

	void CacheGridManager();
	void SpawnSnake();
	void SpawnFood();
	void MoveFoodToRandomFreeCell();
	TArray<FIntPoint> GetValidatedPatrolCells(const FHazardSpawnData& HazardData, const AHazard* SpawnedHazard) const;
	void LoadStage(int32 StageIndex);
	void AdvanceStage();
	void DestroySpawnedSnakes();
	void SpawnSinglePlayerSnake();
	void SpawnCoopSnakes();

	UFUNCTION()
	void HandleFoodConsumed(ASnakePawn* EatingSnake, int32 ScoreValue);

	UFUNCTION()
	void HandleSnakeDied(ASnakePawn* DeadSnake);

	UFUNCTION()
	void ChangePhase(const ESnakeMatchPhase NewPhase);

	UFUNCTION()
	void HandleMusicFinished();

	void PlayEatSound(ASnakePawn* EatingSnake) const;
	void PlayCollisionSound(ASnakePawn* DeadSnake) const;
	void StartMusic();
	void StopMusic();
	
	AUS_GameState* GetUSGameState();
	
	APlayerController* GetPlayerController(int32 SlotIndex);
};
