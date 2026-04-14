// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IntVectorTypes.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

UCLASS()
class UNREALSNEK_API AGridManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGridManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int width;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int height;
	
	class Tile
	{
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FIntVector2 position = FIntVector2();
		UE::Geometry::FVector2i position2 = UE::Geometry::FVector2i();
		FIntPoint tile = FIntPoint();
	};

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
