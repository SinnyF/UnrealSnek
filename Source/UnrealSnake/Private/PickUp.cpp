
#include "PickUp.h"
#include "SnakePawn.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

APickUp::APickUp()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetSimulatePhysics(false);
	
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);
}

void APickUp::BeginPlay()
{
	Super::BeginPlay();
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &APickUp::HandleFoodOverlap);
}

void APickUp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickUp::HandleFoodOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherCamp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bIsActive) return;
	
	if (ASnakePawn* SnakePawn = Cast<ASnakePawn>(OtherActor))
	{
		bIsActive = false;
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SnakePawn->HandleFoodOverlap(this);
	}
}

void APickUp::DeactivateFood()
{
	bIsActive = false;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APickUp::RespawnFood(const FIntPoint& NewGridPosition, const FVector& NewWorldLocation)
{
	GridPosition = NewGridPosition;
	SetActorLocation(NewWorldLocation, false, nullptr, ETeleportType::TeleportPhysics);

	bIsActive = true;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->UpdateOverlaps();
}
