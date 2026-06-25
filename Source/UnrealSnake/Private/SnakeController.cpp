
#include "SnakeController.h"
#include "US_GameInstance.h"
#include "US_GameState.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void ASnakeController::BeginPlay()
{
	Super::BeginPlay();
	
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	if (AUS_GameState* GS = GetWorld()->GetGameState<AUS_GameState>())
	{
		GS->OnPhaseChanged.AddUniqueDynamic(this, &ASnakeController::HandlePhaseChanged);
		
		if (GS->MatchPhase != ESnakeMatchPhase::None)
		{
			HandlePhaseChanged(GS->MatchPhase);
		}
	}
}

void ASnakeController::RequestStartGame()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("Level")));
}

void ASnakeController::RequestStartNormalSinglePlayer(EDifficulty Difficulty)
{
	if (UUS_GameInstance* GI = GetGameInstance<UUS_GameInstance>())
	{
		GI->ConfigureNormalSinglePlayer(Difficulty);
	}

	UGameplayStatics::OpenLevel(this, FName(TEXT("Level")));
}

void ASnakeController::RequestStartBattleLocal(EDifficulty Difficulty)
{
	if (UUS_GameInstance* GI = GetGameInstance<UUS_GameInstance>())
	{
		GI->ConfigureBattleLocal(Difficulty);
	}

	UGameplayStatics::OpenLevel(this, FName(TEXT("Level")));
}

void ASnakeController::RequestStartCoopLocal(EDifficulty Difficulty)
{
	if (UUS_GameInstance* GI = GetGameInstance<UUS_GameInstance>())
	{
		GI->ConfigureCoopLocal(Difficulty);
	}

	UGameplayStatics::OpenLevel(this, FName(TEXT("Level")));
}

void ASnakeController::HandlePhaseChanged(ESnakeMatchPhase Phase)
{
	switch (Phase)
	{
	case ESnakeMatchPhase::Playing:
		{
			UGameplayStatics::SetGamePaused(GetWorld(), false);
			ShowHUD();
			break;
		}
	case ESnakeMatchPhase::Outro:
		{
			UGameplayStatics::SetGamePaused(GetWorld(), true);
			ShowOutro();
			break;
		}
	case ESnakeMatchPhase::MainMenu:
		{
			UGameplayStatics::SetGamePaused(GetWorld(), false);
			ShowMainMenu();
			break;
		}
	case ESnakeMatchPhase::None:
		break;
	}
}

void ASnakeController::HideAllWidgets()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}
}

void ASnakeController::ShowHUD()
{
	HideAllWidgets();
	if (!HUDWidgetClass) return;
	
	if (UGameplayStatics::GetPlayerController(this, 0) != this)
	{
		return;
	}
	
	ActiveWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport();
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}

void ASnakeController::ShowMainMenu()
{
	HideAllWidgets();
	if (!MainMenuWidgetClass) return;

	ActiveWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport();
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ASnakeController::ShowOutro()
{
	HideAllWidgets();
	if (!OutroWidgetClass) return;

	ActiveWidget = CreateWidget<UUserWidget>(this, OutroWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport();
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

