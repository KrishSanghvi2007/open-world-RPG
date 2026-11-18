// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderpaintingGameMode.h"
#include "UnderpaintingHUD.h"

AUnderpaintingGameMode::AUnderpaintingGameMode()
{
	// The drawn HUD for the first playable (health, stamina, Glim, enemy bars).
	HUDClass = AUnderpaintingHUD::StaticClass();
}
