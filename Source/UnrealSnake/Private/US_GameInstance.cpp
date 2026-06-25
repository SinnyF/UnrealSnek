#include "US_GameInstance.h"

void UUS_GameInstance::ConfigureNormalSinglePlayer(EDifficulty Difficulty)
{
	SelectedGameMode = EGameModeType::NormalSinglePlayer;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::AI;
	SelectedDifficulty = Difficulty;
}

void UUS_GameInstance::ConfigureBattleLocal(EDifficulty Difficulty)
{
	SelectedGameMode = EGameModeType::Battle;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::Human;
	SelectedDifficulty = Difficulty;
}

void UUS_GameInstance::ConfigureCoopLocal(EDifficulty Difficulty)
{
	SelectedGameMode = EGameModeType::Coop;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::Human;
	SelectedDifficulty = Difficulty;
}