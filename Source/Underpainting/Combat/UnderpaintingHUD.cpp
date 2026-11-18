#include "UnderpaintingHUD.h"

#include "UnderpaintingEnemy.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingVitalsComponent.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"

namespace
{
	// Palette from docs/GDD.md: madder rose, terre verte, lantern amber, and violet shadow for the bar backs.
	const FLinearColor HealthColour(0.55f, 0.10f, 0.07f);
	const FLinearColor StaminaColour(0.15f, 0.27f, 0.17f);
	const FLinearColor GlimColour(0.80f, 0.29f, 0.04f);
	const FLinearColor BackColour(0.07f, 0.04f, 0.10f, 0.75f);
	const FLinearColor TextColour(0.90f, 0.78f, 0.53f);
}

void AUnderpaintingHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill, const FString& Label)
{
	DrawRect(BackColour, X - 2.0f, Y - 2.0f, Width + 4.0f, Height + 4.0f);
	DrawRect(Fill, X, Y, Width * FMath::Clamp(Fraction, 0.0f, 1.0f), Height);
	if (!Label.IsEmpty())
	{
		DrawText(Label, TextColour, X + Width + 10.0f, Y - 2.0f, nullptr, 1.0f);
	}
}

void AUnderpaintingHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const AUnderpaintingPlayerCharacter* Player = Cast<AUnderpaintingPlayerCharacter>(GetOwningPawn());
	if (const UUnderpaintingVitalsComponent* Vitals = Player ? Player->GetVitals() : nullptr)
	{
		const float X = 40.0f;
		const float Y = Canvas->ClipY - 110.0f;
		DrawBar(X, Y, 300.0f, 16.0f, Vitals->GetHealthFraction(), HealthColour, FString::Printf(TEXT("Health %d"), FMath::CeilToInt(Vitals->GetHealth())));
		DrawBar(X, Y + 26.0f, 300.0f, 12.0f, Vitals->GetStaminaFraction(), StaminaColour, FString::Printf(TEXT("Stamina %d"), FMath::CeilToInt(Vitals->GetStamina())));
		DrawBar(X, Y + 48.0f, 300.0f, 12.0f, Vitals->GetGlimFraction(), GlimColour, FString::Printf(TEXT("Glim %d"), FMath::CeilToInt(Vitals->GetGlim())));

		if (Vitals->IsDead())
		{
			DrawText(TEXT("You were rubbed out."), TextColour, Canvas->ClipX * 0.5f - 110.0f, Canvas->ClipY * 0.4f, nullptr, 1.6f);
		}
		else
		{
			DrawText(TEXT("Left mouse: light   Right mouse: heavy   Left Ctrl: dodge   Left Shift: sprint"), TextColour, X, Canvas->ClipY - 30.0f, nullptr, 0.9f);
		}
	}

	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		const AUnderpaintingEnemy* Enemy = *It;
		const UUnderpaintingVitalsComponent* Vitals = Enemy->GetVitals();
		if (!Vitals || Vitals->IsDead())
		{
			continue;
		}

		const FVector Screen = Project(Enemy->GetActorLocation() + FVector(0.0f, 0.0f, 150.0f));
		if (Screen.Z <= 0.0f)
		{
			continue;
		}

		const float Width = 90.0f;
		DrawBar(Screen.X - Width * 0.5f, Screen.Y, Width, 8.0f, Vitals->GetHealthFraction(), HealthColour, FString());
		DrawBar(Screen.X - Width * 0.5f, Screen.Y + 12.0f, Width, 4.0f, Enemy->GetPoiseFraction(), TextColour, FString());

		FString Status = Enemy->GetDisplayName();
		if (Enemy->GetState() == EEnemyState::Staggered)
		{
			Status += TEXT("  (staggered)");
		}
		else if (!Enemy->GetCurrentAttackName().IsNone())
		{
			Status += TEXT("  ") + Enemy->GetCurrentAttackName().ToString();
		}
		DrawText(Status, TextColour, Screen.X - Width * 0.5f, Screen.Y - 20.0f, nullptr, 0.9f);
	}
}
