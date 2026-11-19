// The Limner: movement from the template character, plus Stamina, sprint, dodge and light and heavy attacks.
// Rules and numbers: docs/GDD.md section 5. Weapon and armour numbers: data/items.json.

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingCharacter.h"
#include "UnderpaintingCombatTypes.h"
#include "UnderpaintingPlayerCharacter.generated.h"

class UUnderpaintingVitalsComponent;
class UAnimMontage;
class UAnimSequenceBase;
class UUnderpaintingAutopilot;

UENUM(BlueprintType)
enum class EPlayerAction : uint8
{
	None,
	LightAttack,
	HeavyAttack,
	Stroke,
	Dodge,
	/** Knocked out of what you were doing by a hit. Short, and a dodge can cut it shorter. */
	Flinch,
	Dead
};

UCLASS(abstract)
class AUnderpaintingPlayerCharacter : public AUnderpaintingCharacter
{
	GENERATED_BODY()

	friend class UUnderpaintingAutopilot;

public:

	AUnderpaintingPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnJumped_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartLightAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartHeavyAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartDodge();

	/** A dodge in a chosen direction (world space), for the autopilot and for anything that knows where it wants to go. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartDodgeInDirection(FVector Direction);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetSprinting(bool bWantsSprint);

	/** Development: a mouse click at a point on the screen (0..1 across, 0..1 down), for testing menus without a hand on the mouse. */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	void DevClickAt(float U, float V);

	/** Development: play from a script (see UnderpaintingAutopilot.h for the verbs). */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	void DevAutopilot(const FString& Script);

	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	FString DevAutopilotStatus() const;

	/** Development: save frames of the game view for a clip (Saved/FightClips/<Name>/). */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	void DevRecord(const FString& Name, float Seconds, float Fps);

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
	virtual void DoLook(float Yaw, float Pitch) override;

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

	/** Puts the player back on firm ground: after a long fall (a small cost), or by choice from the settings page (to where they last woke). */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Dev")
	void ReturnToSafeGround(bool bByChoice);

	virtual void FellOutOfWorld(const class UDamageType& DamageType) override;

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

	/** Held by something (a web): movement is slowed to this fraction for this long. */
	void Snare(float Seconds, float SpeedFraction);

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

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UUnderpaintingAutopilot> Autopilot;

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

	// --- Feel: how it answers the hands. None of these change what a hit does. ---

	/** A press made while busy is kept this long and carried out the moment it becomes possible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float InputBufferTime = 0.30f;

	/** You can still jump this long after walking off an edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float LedgeGraceTime = 0.15f;

	/** How far through a light swing's recovery a dodge may cut in (0 = at once, 1 = never). Weighty: you commit to the hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float DodgeCancelLight = 0.30f;

	/** The same for a heavy swing or a Stroke: you commit for longer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float DodgeCancelHeavy = 0.55f;

	/** How far through a light swing's recovery the next light may begin, so three lights flow as a combo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float LightChainAt = 0.50f;

	/** Seconds you are knocked out of an action by a light hit and by a heavy one (22 damage or more). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float FlinchLight = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float FlinchHeavy = 0.50f;

	/** Without lock-on, a swing turns to the nearest enemy within this distance (cm) and this angle (degrees) of where you are pushing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float AimAssistRange = 380.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Feel")
	float AimAssistAngle = 70.0f;

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

	/** A Flare with this long or less left can be lit again (it gutters as a cue). */
	UPROPERTY(EditAnywhere, Category = "Underpainting|Tuning")
	float FlareRenewWindow = 3.0f;

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
	FVector ForcedDodgeDirection = FVector::ZeroVector;
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
	void LightPressed();
	void MenuClickPressed();
	void TabLeftPressed();
	void TabRightPressed();
	void CursorUpPressed();
	void CursorDownPressed();
	void BindGameKeys();
	FDelegateHandle KeysChangedHandle;
	void LampOilPressed() { UseLampOil(); }
	void TickLockOn(float DeltaTime);
	void ApplyHit(AActor* Target, float Damage, float StaggerDamage, const TCHAR* What, int32 ContrastSign = 0);

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

	// Feel.
	enum class EBuffered : uint8 { None, Light, Heavy, Stroke, Dodge };
	void Buffer(EBuffered What);
	void TryBuffered();
	bool CanCancelInto(EBuffered What) const;
	void PlayAction(UAnimSequenceBase* Animation, float Duration);
	void StopActionAnimation();
	void LoadAnimations();
	void Deny(bool bGlim);
	void TickFeel(float DeltaTime);

	// Teaching prompts (data/hints.json) and the things that keep a playthrough from getting stuck.
	void TickHints(float DeltaTime);
	void HintDone(FName HintName);
	void TickSafeGround(float DeltaTime);
	void Checkpoint();
	void NoteProgressChanged() { bProgressDirty = true; }

	FName CurrentHint;
	float HintClock = 0.0f;
	float LastHintTime = -100.0f;
	float WalkedTotal = 0.0f;
	bool bMenuSeen = false;
	float SnareRemaining = 0.0f;
	float SnareFraction = 1.0f;
	TSet<FName> LearnedByDoing;
	bool bArrivedThroughExit = false;
	bool bInZoneLevel = false;
	bool bHurtOnce = false;
	TArray<FVector> SafeTrail;
	float SafeSampleClock = 0.0f;
	float FallingTime = 0.0f;
	bool bProgressDirty = false;
	float AutosaveClock = 0.0f;
	FDelegateHandle ProgressDirtyHandle;
	class AUnderpaintingEnemy* FindAimTarget() const;

	UFUNCTION()
	void HandleDamaged(float Damage, float StaggerDamage, AActor* DamageCauser);

	EBuffered Buffered = EBuffered::None;
	float BufferedAt = -10.0f;
	int32 ComboIndex = 0;
	float LastSwingEndTime = -10.0f;
	bool bSwingSounded = false;
	TWeakObjectPtr<class AUnderpaintingEnemy> AimTarget;
	float FlinchRemaining = 0.0f;
	float FlinchTotal = 0.0f;
	float LastGroundedTime = 0.0f;
	bool bJumpedSinceGround = false;
	float StepDistance = 0.0f;
	FVector LastStepLocation = FVector::ZeroVector;
	float LastDenyTime = -10.0f;
	FName FootstepSound = TEXT("footstep");

	UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequenceBase>> LightAnims;
	UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequenceBase>> HitAnimsLight;
	UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequenceBase>> HitAnimsMedium;
	UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequenceBase>> DeathAnims;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> HeavyAnim;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> DashAnim;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> HitAnimHeavy;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> HitAnimBack;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> DeathBackAnim;

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
