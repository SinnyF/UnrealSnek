
#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "PickUp.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class ASnakePawn;

UCLASS()
class APickUp : public AActor
{
	GENERATED_BODY()
	
public:	
	APickUp();
	
	UFUNCTION(BlueprintPure, Category = "PickUp")
	FIntPoint GetFoodGridPosition() const { return GridPosition; }
	
	void DeactivateFood();
	
	UFUNCTION(BlueprintCallable, Category = "PickUp")
	void RespawnFood(const FIntPoint& NewGridPosition, const FVector& NewWorldLocation);
	
	UFUNCTION(BlueprintCallable, Category = "PickUp")
	float GetScoreValue() const { return ScoreValue; }	
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleFoodOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherCamp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

public:	
	virtual void Tick(float DeltaTime) override;

private:
	bool bIsActive = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	float ScoreValue = 10.f;	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	TObjectPtr<UPointLightComponent> Light;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PickUp", meta = (AllowPrivateAccess))
	FIntPoint GridPosition = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	float CollisionRadius = 45.f;
};
