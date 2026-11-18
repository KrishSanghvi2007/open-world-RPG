// Copyright Epic Games, Inc. All Rights Reserved.


#include "UnderpaintingPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Underpainting.h"
#include "UnderpaintingHUD.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AUnderpaintingPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogUnderpainting, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AUnderpaintingPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AUnderpaintingPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

bool AUnderpaintingPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	AUnderpaintingHUD* HUD = Cast<AUnderpaintingHUD>(GetHUD());
	if (HUD && HUD->IsCapturingKey() && !Params.Key.IsAnalog())
	{
		if (Params.Event == IE_Pressed)
		{
			HUD->CaptureKey(Params.Key);
		}
		return true;
	}
	return Super::InputKey(Params);
}
