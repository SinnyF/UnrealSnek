#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SnakeGameTypes.h"
#include "US_GameInstance.generated.h"

UCLASS()
class UNREALSNAKE_API UUS_GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "Snake|Mode")
	EGameModeType SelectedGameMode = EGameModeType::NormalSinglePlayer;

	UPROPERTY(BlueprintReadWrite, Category = "Snake|Mode")
	ESnakePlayerSlotType Slot0Type = ESnakePlayerSlotType::Human;

	UPROPERTY(BlueprintReadWrite, Category = "Snake|Mode")
	ESnakePlayerSlotType Slot1Type = ESnakePlayerSlotType::AI;
	
	UPROPERTY(BlueprintReadWrite, Category = "Snake|Mode")
	EDifficulty SelectedDifficulty = EDifficulty::Easy;

	UFUNCTION(BlueprintCallable, Category = "Snake|Mode")
	void ConfigureNormalSinglePlayer(EDifficulty Difficulty);

	UFUNCTION(BlueprintCallable, Category = "Snake|Mode")
	void ConfigureBattleLocal(EDifficulty Difficulty);

	UFUNCTION(BlueprintCallable, Category = "Snake|Mode")
	void ConfigureCoopLocal(EDifficulty Difficulty);
};
