#include "Hazard.h"
#include "HazardTarget.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

AHazard::AHazard()
{
	PrimaryActorTick.bCanEverTick = false;

	HazardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardMesh"));
	HazardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HazardMesh->SetGenerateOverlapEvents(false);
}

void AHazard::EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex)
{
	EliminateTargetActor(TargetActor);
}

bool AHazard::HasAssignedVisualMesh() const
{
	return HazardMesh && HazardMesh->GetStaticMesh();
}

void AHazard::SetHazardActive(bool bNewActive)
{
	bHazardActive = bNewActive;
	SetActorHiddenInGame(!bHazardActive);
	SetActorEnableCollision(bHazardActive);
}

void AHazard::BeginPlay()
{
	Super::BeginPlay();
}

void AHazard::ConfigureHazardCollision(UPrimitiveComponent* CollisionComponent) const
{
	if (!CollisionComponent)
	{
		return;
	}

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AHazard::EliminateTargetActor(AActor* TargetActor)
{
	if (TargetActor && TargetActor->GetClass()->ImplementsInterface(UHazardTarget::StaticClass()))
	{
		IHazardTarget::Execute_EliminateByHazard(TargetActor, this);
	}
}

void AHazard::TrimTargetTail(AActor* TargetActor, int32 HitSegmentIndex)
{
	if (TargetActor && TargetActor->GetClass()->ImplementsInterface(UHazardTarget::StaticClass()))
	{
		IHazardTarget::Execute_TrimTailByHazard(TargetActor, this, HitSegmentIndex);
	}
}

