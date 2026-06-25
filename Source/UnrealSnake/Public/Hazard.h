// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridMovement.h"
#include "Hazard.generated.h"

UENUM(BlueprintType)
enum class EHazardType : uint8 
{
	MovingWall,
	LaserWall,
	MovingLaserBeam
};

UCLASS(Abstract)
class UNREALSNAKE_API AHazard : public AActor
{
	GENERATED_BODY()

public:
	AHazard();

	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
	EHazardType HazardType;
	
	UFUNCTION(BlueprintCallable, Category = "Hazard")
	virtual void EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex);

	UFUNCTION(BlueprintCallable, Category = "Hazard")
	bool IsHazardActive() const { return bHazardActive; }

	UFUNCTION(BlueprintCallable, Category = "Hazard")
	bool HasAssignedVisualMesh() const;

	UFUNCTION(BlueprintCallable, Category = "Hazard")
	virtual void SetHazardActive(bool bNewActive);

	static constexpr int32 HeadHitIndex = -1;

protected:
	void ConfigureHazardCollision(class UPrimitiveComponent* CollisionComponent) const;
	void EliminateTargetActor(AActor* TargetActor);
	void TrimTargetTail(AActor* TargetActor, int32 HitSegmentIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> HazardMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard")
	bool bHazardActive = true;
};