// The Limner: movement from the template character, plus Stamina, sprint, dodge and light and heavy attacks.
// Rules and numbers: docs/GDD.md section 5. Weapon and armour numbers: data/items.json.

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingCharacter.h"
#include "UnderpaintingCombatTypes.h"
#include "UnderpaintingPlayerCharacter.generated.h"

class UUnderpaintingVitalsComponent;
class UAnimMontage;

UENUM(BlueprintType)
enum class EPlayerAction : uint8
{
	None,
	LightAttack,
	HeavyAttack,
	Stroke,
	Dodge,
	Dead
};

UCLASS(abstract)
class AUnderpaintingPlayerCharacter : public AUnderpaintingCharacter
{
	GENERATED_BODY()

public:

	AUnderpaintingPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartLightAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartHeavyAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartDodge();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetSprinting(bool bWantsSprint);

	/** Swaps the weapon for another row in data/items.json. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipWeapon(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipArmour(FName ItemId);

	UFUNCTION(BlueprintPure, Category = "Combat")
	UUnderpaintingVitalsComponent* GetVitals() const { return Vitals; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	EPlayerAction GetCurrentAction() const { return Action; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	EAttackPhase GetAttackPhase() const { return AttackClock.GetPhase(); }

	UFUNCTION(BlueprintPure, Category = "Combat")
	FName GetWeaponId() const { return WeaponId; }

	/** The thing the player would use by pressing Interact, or null. */
	class AUnderpaintingInteractable* GetFocus() const { return Focus.Get(); }

	/** Called by a Lampstand after resting: refills Health and Stamina and saves. */
	void OnRested();

	virtual void DoMove(float Right, float Forward) override;

	/** Uses whatever is in reach, or advances the open menu. The Interact key calls this; so can scripts and tests. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact();

	/** A number key, 1 to 9, for the open menu. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void MenuNumber(int32 Number);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void MenuClose();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveNow();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void LoadNow();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void StartNewGame();

	/** The Stroke of the active well: the heavy attack, changed by a pigment. Costs heavy Stamina plus Glim. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool StartStroke();

	/** Glim the active well's Stroke would cost here and now. */
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetStrokeGlimCost() const;

	/** Light where the player stands, 0 to 1 (the zone's ambient light, at least 0.5 inside a Flare). */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetLightHere() const;

	/** Something lights the place for a while (the Matriarch's heartbeat): light here counts as at least this. */
	void LendLight(float Light, float Seconds);

	/** Locks the view onto the enemy in front, or lets go. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ToggleLockOn();

	UFUNCTION(BlueprintPure, Category = "Combat")
	class AUnderpaintingEnemy* GetLockTarget() const { return LockTarget.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SwapWell();

	/** Uses a Moss Poultice: 40 Health over 4 seconds. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool UsePoultice();

	/** Pours Lamp Oil into the lantern: +40 Glim. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool UseLampOil();

	/** Lights a Flare: 20 Glim for 8 seconds of light in an 8 m radius (docs/GDD.md 4.5). */
	UFUNCTION(BlueprintCallable, Category = "Exploration")
	bool UseFlare();

	/** Throws a Fixative Flask at the sketched platform being looked at (or the nearest within 10 m). */
	UFUNCTION(BlueprintCallable, Category = "Exploration")
	bool UseFixativeFlask();

	/** True if a Flare is burning and this point is inside its light. */
	UFUNCTION(BlueprintPure, Category = "Exploration")
	bool IsFlareLighting(const FVector& Location) const;

	UFUNCTION(BlueprintPure, Category = "Exploration")
	float GetFlareTimeRemaining() const { return FlareRemaining; }

	/** For test scripts and debugging: puts dry pigment or an item straight into the pack. */
	UFUNCTION(BlueprintCallable, Category = "Dev")
	void DevGrant(FName Id, int32 Count);

	/** For scripted checks: open the satchel on a tab (0 Pack, 1 Journal, 2 Notes, 3 Settings). */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	void DevOpenSatchel(int32 Tab);

	/** For scripted checks: move the highlight in the satchel. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	void DevMoveCursor(int32 Steps);

	/** For scripted checks: the key the settings screen is waiting for. Returns false if it was not waiting. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	bool DevPressKey(FKey Key);

	/** For scripted checks: the key an action is on, as the hints show it. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	FString DevDescribeKey(FName ActionId) const;

	/** Debug (F7): 20 of every pigment, dried, a full pack of materials, and every recipe taught. For testing crafting. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	void DevFill();

	/** Sanguine Draught: +20% damage for 30 seconds. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool UseDraught();

	/** Blue Tincture: you fall slowly and jump further for 20 seconds. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool UseTincture();

	/** Seconds into the Turning sequence, or -1 when it is not playing. */
	float GetTurningTime() const { return TurningTime; }

	/** How long the Turning takes before the changed level loads. */
	static constexpr float TurningLength = 9.0f;

	/** Debug: start the Turning now, whatever the quest says. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	void DevStartTurning();

	float GetDraughtRemaining() const { return DraughtRemaining; }
	float GetTinctureRemaining() const { return TinctureRemaining; }

	/** For test scripts and debugging: buys an attunement node if it can be afforded. */
	UFUNCTION(BlueprintCallable, Category = "Dev")
	bool DevAttune(FName NodeId);

	/** For test scripts and debugging: nothing can hurt the player. */
	UFUNCTION(BlueprintCallable, Category = "Dev")
	void DevSetInvulnerable(bool bOn);

	/** For test scripts and debugging: damages every enemy in the level. Returns the total dealt. */
	UFUNCTION(BlueprintCallable, Category = "Dev")
	float DevHurtEnemies(float Damage);

	/** For test scripts and debugging: one line per enemy. */
	UFUNCTION(BlueprintCallable, Category = "Dev")
	FString DevDescribeEnemies();

	/** For test scripts and debugging: puts a pigment in a well. */
	UFUNCTION(BlueprintCallable, Category = "Dev")
	bool DevSetWell(int32 Index, FName PigmentId);

	/** One line describing what the player would see: prompt, open menu, quest step, pack. For scripts and tests. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FString DescribeState();

protected:

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUnderpaintingVitalsComponent> Vitals;

	/** Rows in data/items.json. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Equipment")
	FName WeaponId = TEXT("LimnersKnife");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Equipment")
	FName ArmourId = TEXT("SizedSmock");

	/** Swing timings for a weapon of speed 1.0; a weapon's AttackSpeed scales them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	FAttackTiming LightTiming = FAttackTiming(0.25f, 0.15f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	FAttackTiming HeavyTiming = FAttackTiming(0.50f, 0.20f, 0.60f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float DodgeStaminaCost = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float DodgeDuration = 0.6f;

	/** Seconds at the start of a dodge during which hits pass through. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float DodgeInvulnerableTime = 0.30f;

	/** Centimetres travelled by one dodge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float DodgeDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float SprintStaminaPerSecond = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float SprintSpeed = 750.0f;

	/** Hold the heavy button this long (seconds) and it becomes the active well's Stroke. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float StrokeHoldTime = 0.28f;

	/** How far away (cm) an enemy can be locked onto, and how far before the lock lets go. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float LockOnRange = 1500.0f;

	/** Movement speed while committed to a swing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Tuning")
	float AttackMoveSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Tuning")
	float FlareGlimCost = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Tuning")
	float FlareDuration = 8.0f;

	/** Centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Tuning")
	float FlareRadius = 800.0f;

	/** How far (cm) the player can reach to use something. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Tuning")
	float UseReach = 320.0f;

	/** How far off the centre of the view (degrees) something may be and still be the thing you are looking at. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Tuning")
	float UseViewAngle = 35.0f;

	/** The Flare: a small light, so it casts no shadow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPointLightComponent> FlareLight;

	/** Optional animations. The rules work without them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> LightAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> HeavyAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

private:

	bool CanStartAction() const;
	void StartAttack(EPlayerAction NewAction);
	void TickAttack(float DeltaTime);
	void TickDodge(float DeltaTime);
	void EndAction();
	void HitTest();

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	void RestartLevel();
	void InteractPressed() { Interact(); }
	void MenuClosePressed() { MenuClose(); }
	void InventoryPressed();
	void SavePressed() { SaveNow(); }
	void LoadPressed() { LoadNow(); }
	void NewGamePressed() { StartNewGame(); }
	template <int32 Number> void NumberPressed() { OnNumber(Number); }
	void OnNumber(int32 Number) { MenuNumber(Number); }
	void UpdateFocus();
	void ApplyAttunements();
	void RestoreFromSave();
	class AUnderpaintingHUD* GetUnderpaintingHUD() const;
	bool IsMenuOpen() const;

	TWeakObjectPtr<class AUnderpaintingInteractable> Focus;
	float FlareRemaining = 0.0f;
	TWeakObjectPtr<class AUnderpaintingEnemy> LockTarget;
	FName StrokePigment;
	FName StrokeName;
	float StrokeDamage = 0.0f;
	int32 StrokeHitsDone = 0;
	float HeavyHeldSince = -1.0f;
	bool bDodgeRewarded = false;
	float ZoneAmbientLight = 1.0f;
	bool bDevInvulnerable = false;
	float BorrowedLight = 0.0f;
	float BorrowedLightUntil = -1.0f;
	void HeavyPressed();
	void HeavyReleased();
	void LockOrClosePressed();
	void SwapWellPressed() { SwapWell(); }
	void PoulticePressed() { UsePoultice(); }
	void DraughtPressed() { UseDraught(); }
	void TincturePressed() { UseTincture(); }
	void DevFillPressed() { DevFill(); }
	void TabLeftPressed();
	void TabRightPressed();
	void CursorUpPressed();
	void CursorDownPressed();
	void BindGameKeys();
	FDelegateHandle KeysChangedHandle;
	void LampOilPressed() { UseLampOil(); }
	void TickLockOn(float DeltaTime);
	void ApplyHit(AActor* Target, float Damage, float StaggerDamage, const TCHAR* What);

	UFUNCTION()
	void HandleDodged(AActor* Attacker);
	void FlarePressed() { UseFlare(); }
	void FlaskPressed() { UseFixativeFlask(); }
	void ArriveFromZone();
	FDelegateHandle ProgressChangedHandle;
	float BaseDodgeDistance = 450.0f;
	float BaseDodgeInvulnerableTime = 0.30f;

	void SprintPressed() { SetSprinting(true); }
	void SprintReleased() { SetSprinting(false); }

	EPlayerAction Action = EPlayerAction::None;
	FAttackClock AttackClock;
	float DodgeElapsed = 0.0f;
	FVector DodgeDirection = FVector::ForwardVector;
	bool bWantsSprint = false;

	// Weapon numbers, read from data/items.json when equipped.
	float WeaponLightDamage = 14.0f;
	float WeaponHeavyDamage = 26.0f;
	float WeaponSpeed = 1.0f;
	float WeaponStaminaLight = 12.0f;
	float WeaponStaminaHeavy = 25.0f;
	float WeaponReach = 160.0f;
	float WeaponStaggerMultiplier = 1.0f;

	// The Turning.
	float TurningTime = -1.0f;
	FRotator TurningBaseRotation = FRotator::ZeroRotator;

	// Timed consumables.
	float DraughtRemaining = 0.0f;
	float DraughtBonus = 0.0f;
	float TinctureRemaining = 0.0f;
	float ArmourStaminaRegenPercent = 0.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HitThisSwing;

	FTimerHandle RestartTimer;
};
