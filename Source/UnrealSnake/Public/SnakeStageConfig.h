#pragma once

#include "CoreMinimal.h"
#include "SnakeStageConfig.generated.h"


USTRUCT(BlueprintType)
struct FHazardSpawnData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	FIntPoint SpawnCell = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	FRotator Rotation = FRotator::ZeroRotator;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	TArray<FIntPoint> PatrolCells;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "1.0"))
	float MovementSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float PatrolWaitSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bAutoStartPatrol = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser", meta = (ClampMin = "0.0"))
	float ToggleRateSeconds = 4.f;
};

USTRUCT(BlueprintType)
struct FSnakeStageConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint GridDimensions = FIntPoint(20, 20);
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CellSize = 100.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MoveStepTime = 0.2f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 FoodToClear = 3;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FHazardSpawnData> Hazards;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Pattern = 0;
};
