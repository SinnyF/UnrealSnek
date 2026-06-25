#pragma once

#include "CoreMinimal.h"
#include "SnakeGameTypes.generated.h"

UENUM(BlueprintType)
enum class EGameModeType : uint8
{
	NormalSinglePlayer,
	Coop,
	Battle
};

UENUM(BlueprintType)
enum class ESnakePlayerSlotType : uint8
{
	Human,
	AI
};

UENUM(BlueprintType)
enum class EResult : uint8
{
	None,
	Player0Won,
	Player1Won,
	Draw
};
