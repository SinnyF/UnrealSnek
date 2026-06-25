#pragma once

#include "CoreMinimal.h"
#include "US_GameMode.h"
#include "GameFramework/PlayerController.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "UObject/UnrealTypePrivate.h"
#include "SnakeController.generated.h"

class UUserWidget;

UCLASS()
class UNREALSNAKE_API ASnakeController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void ShowHUD();

	UFUNCTION(BlueprintCallable)
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable)
	void ShowOutro();

	UFUNCTION(BlueprintCallable)
	void HideAllWidgets();
	
	UFUNCTION(BlueprintCallable)
	void RequestStartGame();
	
	UFUNCTION(BlueprintCallable)
	void RequestRestartFromUI()
	{
		if (AUS_GameMode* GM = GetWorld()->GetAuthGameMode<AUS_GameMode>())
		{
			GM->RestartRun();
		}
	}
	
	UFUNCTION(BlueprintCallable)
	void RequestReturnToMenuFromUI()
	{
		if (AUS_GameMode* GM = GetWorld()->GetAuthGameMode<AUS_GameMode>())
		{
			GM->ReturnToMainMenu();
		}
	}
	UFUNCTION(BlueprintCallable)
	void RequestStartNormalSinglePlayer();

	UFUNCTION(BlueprintCallable)
	void RequestStartBattleLocal();

	UFUNCTION(BlueprintCallable)
	void RequestStartCoopLocal();

protected:
	UFUNCTION(BlueprintCallable)
	void HandlePhaseChanged(ESnakeMatchPhase Phase);
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> OutroWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveWidget;
};
