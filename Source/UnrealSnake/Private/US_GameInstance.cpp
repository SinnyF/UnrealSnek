#include "US_GameInstance.h"

void UUS_GameInstance::ConfigureNormalSinglePlayer()
{
	SelectedGameMode = EGameModeType::NormalSinglePlayer;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::AI;
}

void UUS_GameInstance::ConfigureBattleLocal()
{
	SelectedGameMode = EGameModeType::Battle;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::Human;
}

void UUS_GameInstance::ConfigureCoopLocal()
{
	SelectedGameMode = EGameModeType::Coop;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::Human;
}