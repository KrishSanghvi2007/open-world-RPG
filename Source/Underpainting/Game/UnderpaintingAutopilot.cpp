#include "UnderpaintingAutopilot.h"

#include "Underpainting.h"
#include "UnderpaintingEnemy.h"
#include "UnderpaintingHUD.h"
#include "UnderpaintingPlayerCharacter.h"
#include "UnderpaintingProgressSubsystem.h"
#include "UnderpaintingVitalsComponent.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

UUnderpaintingAutopilot::UUnderpaintingAutopilot()
{
	PrimaryComponentTick.bCanEverTick = true;
}

AUnderpaintingPlayerCharacter* UUnderpaintingAutopilot::GetPlayer() const
{
	return Cast<AUnderpaintingPlayerCharacter>(GetOwner());
}

float UUnderpaintingAutopilot::Arg(const FStep& Step, int32 ArgIndex, float Default) const
{
	return Step.Args.IsValidIndex(ArgIndex) ? FCString::Atof(*Step.Args[ArgIndex]) : Default;
}

void UUnderpaintingAutopilot::Note(const FString& Text)
{
	Notes.Add(FString::Printf(TEXT("%.1fs %s"), TotalTime, *Text));
	UE_LOG(LogUnderpainting, Log, TEXT("AUTOPILOT %.1fs %s"), TotalTime, *Text);
}

void UUnderpaintingAutopilot::Run(const FString& Script)
{
	Steps.Reset();
	TArray<FString> Parts;
	Script.ParseIntoArray(Parts, TEXT(";"));
	for (FString& Part : Parts)
	{
		Part.TrimStartAndEndInline();
		TArray<FString> Words;
		Part.ParseIntoArrayWS(Words);
		if (Words.Num() > 0)
		{
			FStep Step;
			Step.Verb = Words[0].ToLower();
			Words.RemoveAt(0);
			Step.Args = Words;
			Steps.Add(Step);
		}
	}
	Index = Steps.Num() > 0 ? 0 : INDEX_NONE;
	StepTime = TotalTime = 0.0f;
	Notes.Reset();
	bFollow = false;
	HitsTaken = Dodges = 0;
	WalkedDistance = 0.0f;
	if (const AUnderpaintingPlayerCharacter* Player = GetPlayer())
	{
		LastHealth = Player->GetVitals()->GetHealth();
		LastProgressLocation = Player->GetActorLocation();
	}
}

FString UUnderpaintingAutopilot::Describe() const
{
	FString Text = IsRunning()
		? FString::Printf(TEXT("RUNNING step %d of %d (%s) t=%.1f"), Index + 1, Steps.Num(), *Steps[Index].Verb, TotalTime)
		: FString::Printf(TEXT("DONE t=%.1f"), TotalTime);
	Text += FString::Printf(TEXT(" walked=%.0fm hits_taken=%d dodges=%d recording=%s\n"), WalkedDistance / 100.0f, HitsTaken, Dodges, RecordRemaining > 0.0f ? TEXT("yes") : TEXT("no"));
	for (const FString& Line : Notes)
	{
		Text += Line + TEXT("\n");
	}
	return Text;
}

AActor* UUnderpaintingAutopilot::FindMarker(const FStep& Step, int32 FirstArg) const
{
	if (!Step.Args.IsValidIndex(FirstArg + 1))
	{
		return nullptr;
	}
	const FName Kind(*Step.Args[FirstArg]), Id(*Step.Args[FirstArg + 1]);
	const int32 Wanted = FMath::Max(1, Step.Args.IsValidIndex(FirstArg + 2) ? FCString::Atoi(*Step.Args[FirstArg + 2]) : 1);
	TArray<AActor*> Found;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(Kind) && It->ActorHasTag(Id) && !It->GetClass()->GetName().Contains(TEXT("TextRender")))
		{
			Found.Add(*It);
		}
	}
	Found.Sort([](const AActor& A, const AActor& B) { return A.GetName() < B.GetName(); });
	return Found.IsValidIndex(Wanted - 1) ? Found[Wanted - 1] : nullptr;
}

AUnderpaintingEnemy* UUnderpaintingAutopilot::NearestEnemy(float Range) const
{
	AUnderpaintingEnemy* Best = nullptr;
	float BestDistance = Range;
	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		const float Distance = FVector::Dist(It->GetActorLocation(), GetOwner()->GetActorLocation());
		if (!It->GetVitals()->IsDead() && !It->IsHidden() && Distance < BestDistance)
		{
			Best = *It;
			BestDistance = Distance;
		}
	}
	return Best;
}

void UUnderpaintingAutopilot::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Recording runs on real time, so a clip plays back at the speed it happened (hit-stop included).
	if (RecordRemaining > 0.0f)
	{
		const float Real = FApp::GetDeltaTime();
		RecordRemaining -= Real;
		RecordAccumulated += Real;
		if (RecordAccumulated >= RecordInterval)
		{
			RecordAccumulated = FMath::Fmod(RecordAccumulated, RecordInterval);
			CaptureFrame();
		}
	}

	AUnderpaintingPlayerCharacter* Player = GetPlayer();
	if (!Player || !IsRunning())
	{
		return;
	}
	TotalTime += DeltaTime;
	StepTime += DeltaTime;

	// Count the hits that land, whatever step is running.
	const float Health = Player->GetVitals()->GetHealth();
	if (Health < LastHealth - 0.5f)
	{
		if (LastHealth - Health < 500.0f)        // not a blow: Health being put back to normal after a trial
		{
			++HitsTaken;
			DamageTaken += LastHealth - Health;
		}
	}
	LastHealth = Health;
	if (Player->GetVitals()->IsDead())
	{
		Note(FString::Printf(TEXT("DIED during step %d (%s)"), Index + 1, *Steps[Index].Verb));
		Index = INDEX_NONE;
		return;
	}

	if (bFollow)
	{
		if (const AUnderpaintingEnemy* Enemy = NearestEnemy(3000.0f))
		{
			FVector To = Enemy->GetActorLocation() - Player->GetActorLocation();
			To.Z = 0.0f;
			if (To.Size() > FollowDistance)
			{
				Player->AddMovementInput(To.GetSafeNormal(), 1.0f);
			}
		}
	}

	if (TickStep(Steps[Index], DeltaTime))
	{
		++Index;
		StepTime = 0.0f;
		SinceProgress = 0.0f;
		LastProgressLocation = Player->GetActorLocation();
		bTalkStarted = false;
		bThreatDecided = false;
		if (!IsRunning())
		{
			Player->SetSprinting(false);
			Note(TEXT("script finished"));
		}
	}
}

bool UUnderpaintingAutopilot::TickWalk(const FVector& Target, bool bSprint, float StopDistance, float DeltaTime)
{
	AUnderpaintingPlayerCharacter* Player = GetPlayer();

	// On guard: anything that comes for you on the way is fought first, as a person would.
	if (GuardSkill >= 0.0f)
	{
		const AUnderpaintingEnemy* Threat = NearestEnemy(750.0f);
		if (Threat && Threat->GetState() != EEnemyState::Idle && Threat->GetState() != EEnemyState::Returning)
		{
			Player->SetSprinting(false);
			GuardTime += DeltaTime;
			TickFight(GuardSkill, 100000.0f, DeltaTime);
			return false;
		}
		Player->LockTarget = nullptr;
	}

	FVector To = Target - Player->GetActorLocation();
	To.Z = 0.0f;
	if (To.Size() <= StopDistance)
	{
		Player->SetSprinting(false);
		return true;
	}

	Player->SetSprinting(bSprint);
	Player->AddMovementInput(To.GetSafeNormal(), 1.0f);
	// The view swings round to where it is going, as a player's would.
	if (AController* Controller = Player->GetController())
	{
		if (!Player->GetLockTarget())
		{
			FRotator Wanted = To.Rotation();
			Wanted.Pitch = -10.0f;
			Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), Wanted, DeltaTime, 3.0f));
		}
	}

	// Has it got anywhere? If not: jump, then give up, say so, and carry on from the target.
	const float Moved = FVector::Dist2D(Player->GetActorLocation(), LastProgressLocation);
	SinceProgress += DeltaTime;
	if (Moved > 60.0f)
	{
		WalkedDistance += Moved;
		LastProgressLocation = Player->GetActorLocation();
		SinceProgress = 0.0f;
	}
	else if (SinceProgress > 1.2f && SinceProgress < 1.3f)
	{
		Player->Jump();
	}
	else if (SinceProgress > 5.0f)
	{
		const FVector Here = Player->GetActorLocation();
		Note(FString::Printf(TEXT("STUCK at (%.1f, %.1f, %.1f) walking to (%.1f, %.1f); teleported on"), Here.X / 100.0f, Here.Y / 100.0f, Here.Z / 100.0f, Target.X / 100.0f, Target.Y / 100.0f));
		Player->SetActorLocation(FVector(Target.X, Target.Y, FMath::Max(Target.Z, Here.Z) + 100.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Player->SetSprinting(false);
		return true;
	}
	return false;
}

bool UUnderpaintingAutopilot::TickTalk(const FStep& Step, float DeltaTime)
{
	AUnderpaintingPlayerCharacter* Player = GetPlayer();
	AUnderpaintingHUD* HUD = Player->GetUnderpaintingHUD();
	if (!HUD)
	{
		return true;
	}
	if (!bTalkStarted)
	{
		bTalkStarted = true;
		TalkLine = 0;
		TalkWait = 0.0f;
		Player->Interact();
		if (HUD->GetMenu() != EUnderpaintingMenu::Dialogue)
		{
			Note(TEXT("TALK: nobody answered"));
			return true;
		}
	}
	if (HUD->GetMenu() != EUnderpaintingMenu::Dialogue)
	{
		return true;
	}

	// Read the line (about 17 characters a second, and a beat), then answer or go on.
	if (TalkWait <= 0.0f)
	{
		const UUnderpaintingProgress* Progress = UUnderpaintingProgressSubsystem::Get(this);
		const FDialogueRow* Row = Progress ? Progress->GetDialogueRow(HUD->GetDialogueRow()) : nullptr;
		TalkWait = bFast ? 0.05f : 0.8f + (Row ? Row->Text.Len() / 17.0f : 1.0f);
	}
	TalkWait -= DeltaTime;
	if (TalkWait > 0.0f)
	{
		return false;
	}
	const int32 Reply = Step.Args.Num() > 0 ? FCString::Atoi(*Step.Args[FMath::Min(TalkLine, Step.Args.Num() - 1)]) : 1;
	const FName Before = HUD->GetDialogueRow();
	Player->MenuNumber(Reply);
	if (HUD->GetMenu() == EUnderpaintingMenu::Dialogue && HUD->GetDialogueRow() == Before)
	{
		Player->Interact();
	}
	++TalkLine;
	TalkWait = 0.0f;
	return HUD->GetMenu() != EUnderpaintingMenu::Dialogue || TalkLine > 14;
}

bool UUnderpaintingAutopilot::TickFight(float Skill, float Timeout, float DeltaTime)
{
	AUnderpaintingPlayerCharacter* Player = GetPlayer();
	AUnderpaintingEnemy* Enemy = NearestEnemy(2500.0f);
	if (!Enemy)
	{
		Note(FString::Printf(TEXT("FIGHT won in %.1fs, health %.0f, hits taken so far %d"), StepTime, Player->GetVitals()->GetHealth(), HitsTaken));
		Player->LockTarget = nullptr;
		return true;
	}
	if (StepTime > Timeout)
	{
		Note(FString::Printf(TEXT("FIGHT timed out after %.0fs: %s has %.0f health left"), Timeout, *Enemy->GetDisplayName(), Enemy->GetVitals()->GetHealth()));
		return true;
	}

	Player->LockTarget = Enemy;
	FVector To = Enemy->GetActorLocation() - Player->GetActorLocation();
	To.Z = 0.0f;
	const float Distance = To.Size();
	const FVector Toward = To.GetSafeNormal();
	UUnderpaintingVitalsComponent* Vitals = Player->GetVitals();

	// An attack is coming. Decide once whether this one is read (skill), then dodge late, the way a player does.
	const float UntilHit = Enemy->GetTimeUntilHit();
	if (UntilHit >= 0.0f)
	{
		if (ThreatSeen.Get() != Enemy || ThreatAttack != Enemy->GetCurrentAttackName() || !bThreatDecided)
		{
			ThreatSeen = Enemy;
			ThreatAttack = Enemy->GetCurrentAttackName();
			bThreatDecided = true;
			bWillDodge = FMath::FRand() < Skill;
		}
		if (bWillDodge && UntilHit < 0.16f && Distance < 520.0f && Player->GetCurrentAction() == EPlayerAction::None)
		{
			// Sideways and a little back, alternating sides.
			const FVector Side = FVector::CrossProduct(Toward, FVector::UpVector) * ((Dodges % 2 == 0) ? 1.0f : -1.0f);
			Player->StartDodgeInDirection((Side - Toward * 0.4f).GetSafeNormal());
			if (Player->GetCurrentAction() == EPlayerAction::Dodge)
			{
				++Dodges;
			}
			return false;
		}
	}
	else
	{
		bThreatDecided = false;
	}

	if (Player->GetCurrentAction() != EPlayerAction::None)
	{
		return false;
	}

	// Drink when hurt and there is a gap.
	if (Vitals->GetHealth() < Vitals->MaxHealth * 0.45f && UntilHit < 0.0f && Distance > 250.0f)
	{
		Player->UsePoultice();
	}

	const float Reach = 165.0f;
	if (Distance > Reach)
	{
		Player->AddMovementInput(Toward, 1.0f);
		return false;
	}

	// In reach. Do not start a swing that an attack about to land would interrupt, and keep a dodge's worth of Stamina.
	const bool bEnemyOpen = Enemy->GetState() == EEnemyState::Staggered || Enemy->GetAttackPhase() == EAttackPhase::Recovery || Enemy->GetState() == EEnemyState::Chasing;
	if (UntilHit >= 0.0f && UntilHit < 0.75f && bWillDodge)
	{
		return false;
	}
	if (bEnemyOpen && Vitals->GetStamina() >= 34.0f)
	{
		if (Enemy->GetState() == EEnemyState::Staggered && Vitals->GetStamina() >= 48.0f)
		{
			if (!Player->StartStroke())
			{
				Player->StartHeavyAttack();
			}
		}
		else
		{
			Player->StartLightAttack();
		}
	}
	return false;
}

bool UUnderpaintingAutopilot::TickStep(const FStep& Step, float DeltaTime)
{
	AUnderpaintingPlayerCharacter* Player = GetPlayer();
	const FString& Verb = Step.Verb;

	if (Verb == TEXT("walk") || Verb == TEXT("sprint"))
	{
		const FVector Target(Arg(Step, 0, 0) * 100.0f, Arg(Step, 1, 0) * 100.0f, Arg(Step, 2, Player->GetActorLocation().Z / 100.0f) * 100.0f);
		return TickWalk(Target, Verb == TEXT("sprint"), 70.0f, DeltaTime);
	}
	if (Verb == TEXT("walkto") || Verb == TEXT("sprintto"))
	{
		const AActor* Marker = FindMarker(Step, 0);
		if (!Marker)
		{
			Note(FString::Printf(TEXT("NO MARKER %s"), *FString::Join(Step.Args, TEXT(" "))));
			return true;
		}
		return TickWalk(Marker->GetActorLocation(), Verb == TEXT("sprintto"), 150.0f, DeltaTime);
	}
	if (Verb == TEXT("look"))
	{
		const AActor* Marker = FindMarker(Step, 0);
		AController* Controller = Player->GetController();
		if (!Marker || !Controller)
		{
			return true;
		}
		const FRotator Wanted = (Marker->GetActorLocation() - (Player->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f))).Rotation();
		Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), Wanted, DeltaTime, 10.0f));
		Player->SetActorRotation(FRotator(0.0f, Controller->GetControlRotation().Yaw, 0.0f));
		return StepTime > 0.5f;
	}
	if (Verb == TEXT("use")) { Player->Interact(); return true; }
	if (Verb == TEXT("menu")) { Player->MenuNumber(int32(Arg(Step, 0, 1))); return true; }
	if (Verb == TEXT("close")) { Player->MenuClose(); return true; }
	if (Verb == TEXT("jump")) { Player->Jump(); return true; }
	if (Verb == TEXT("talk")) { return TickTalk(Step, DeltaTime); }
	if (Verb == TEXT("wait")) { return StepTime >= Arg(Step, 0, 1.0f); }
	if (Verb == TEXT("light")) { Player->StartLightAttack(); return true; }
	if (Verb == TEXT("heavy")) { Player->StartHeavyAttack(); return true; }
	if (Verb == TEXT("stroke")) { Player->StartStroke(); return true; }
	if (Verb == TEXT("dodge")) { Player->StartDodge(); return true; }
	if (Verb == TEXT("lock")) { Player->ToggleLockOn(); return true; }
	if (Verb == TEXT("flare")) { Player->UseFlare(); return true; }
	if (Verb == TEXT("swap")) { Player->SwapWell(); return true; }
	if (Verb == TEXT("poultice")) { Player->UsePoultice(); return true; }
	if (Verb == TEXT("follow"))
	{
		bFollow = Step.Args.Num() > 0 && Step.Args[0] == TEXT("on");
		FollowDistance = Arg(Step, 1, 1.7f) * 100.0f;
		return true;
	}
	if (Verb == TEXT("fight")) { return TickFight(Arg(Step, 0, 0.6f), Arg(Step, 1, 180.0f), DeltaTime); }
	if (Verb == TEXT("trial")) { return TickTrial(Step, DeltaTime); }
	if (Verb == TEXT("mark"))
	{
		Note(FString::Printf(TEXT("MARK %s"), Step.Args.Num() > 0 ? *FString::Join(Step.Args, TEXT(" ")) : TEXT("?")));
		return true;
	}
	if (Verb == TEXT("guard")) { GuardSkill = Step.Args.Num() > 0 && Step.Args[0] == TEXT("on") ? Arg(Step, 1, 0.6f) : -1.0f; return true; }
	if (Verb == TEXT("god")) { Player->DevSetInvulnerable(Step.Args.Num() > 0 && Step.Args[0] == TEXT("on")); return true; }
	if (Verb == TEXT("grant") && Step.Args.Num() >= 2) { Player->DevGrant(FName(*Step.Args[0]), FCString::Atoi(*Step.Args[1])); return true; }
	if (Verb == TEXT("attune") && Step.Args.Num() >= 1) { Player->DevAttune(FName(*Step.Args[0])); return true; }
	if (Verb == TEXT("hurt")) { Player->DevHurtEnemies(Arg(Step, 0, 10.0f)); return true; }
	if (Verb == TEXT("fast")) { bFast = Step.Args.Num() > 0 && Step.Args[0] == TEXT("on"); return true; }
	if (Verb == TEXT("tp"))
	{
		Player->SetActorLocation(FVector(Arg(Step, 0, 0) * 100.0f, Arg(Step, 1, 0) * 100.0f, Arg(Step, 2, 0) * 100.0f + 100.0f), false, nullptr, ETeleportType::TeleportPhysics);
		return true;
	}

	Note(FString::Printf(TEXT("UNKNOWN step '%s'"), *Verb));
	return true;
}

bool UUnderpaintingAutopilot::TickTrial(const FStep& Step, float DeltaTime)
{
	// trial <EnemyId> <skill> <rounds>: fights a fresh one that many times and notes the time and the damage taken in each.
	// The player cannot die during a trial (their Health is topped up between hits); the damage is what is measured.
	AUnderpaintingPlayerCharacter* Player = GetPlayer();
	UUnderpaintingVitalsComponent* Vitals = Player->GetVitals();
	const FString EnemyId = Step.Args.Num() > 0 ? Step.Args[0] : TEXT("Blotter");
	const float Skill = Arg(Step, 1, 0.6f);
	const int32 Rounds = FMath::Max(1, int32(Arg(Step, 2, 3.0f)));
	Vitals->SetMaxHealth(100000.0f);
	if (Vitals->GetHealth() < 50000.0f)
	{
		Vitals->SetHealthAndGlim(100000.0f, Vitals->GetGlim());
		LastHealth = 100000.0f;
	}

	AUnderpaintingEnemy* Enemy = TrialEnemy.Get();
	if (Enemy && Enemy->GetActorLocation().Z < Player->GetActorLocation().Z - 1500.0f)
	{
		// It left the room (knocked through a wall). That round does not count.
		Enemy->Destroy();
		TrialEnemy = nullptr;
		Enemy = nullptr;
		--TrialRound;
	}
	if (Enemy && !Enemy->GetVitals()->IsDead() && TotalTime - TrialStartTime < 300.0f)
	{
		TickFight(Skill, 100000.0f, DeltaTime);
		return false;
	}
	if (Enemy)
	{
		const bool bWon = Enemy->GetVitals()->IsDead();
		TrialTimes.Add(TotalTime - TrialStartTime);
		TrialDamages.Add(DamageTaken - TrialStartDamage);
		Note(FString::Printf(TEXT("TRIAL %s skill %.1f round %d: %s in %.1fs, damage taken %.0f in %d hits"), *EnemyId, Skill, TrialRound,
			bWon ? TEXT("won") : TEXT("TIMED OUT"), TrialTimes.Last(), TrialDamages.Last(), HitsTaken - TrialStartHits));
		Enemy->Destroy();
		TrialEnemy = nullptr;
		Player->LockTarget = nullptr;
		Vitals->Refill();
		if (TrialRound >= Rounds)
		{
			float Time = 0.0f, Damage = 0.0f, Worst = 0.0f;
			for (int32 Round = 0; Round < TrialTimes.Num(); ++Round)
			{
				Time += TrialTimes[Round];
				Damage += TrialDamages[Round];
				Worst = FMath::Max(Worst, TrialDamages[Round]);
			}
			Note(FString::Printf(TEXT("TRIALS %s skill %.1f x%d: mean time %.1fs, mean damage %.0f, worst %.0f"), *EnemyId, Skill, TrialTimes.Num(),
				Time / TrialTimes.Num(), Damage / TrialTimes.Num(), Worst));
			TrialRound = 0;
			TrialTimes.Reset();
			TrialDamages.Reset();
			return true;
		}
	}

	// A fresh one, nine metres ahead, and nothing else in the room.
	static const TMap<FString, FString> Classes = { { TEXT("Blotter"), TEXT("BlotterEnemy") }, { TEXT("Madderback"), TEXT("MadderbackEnemy") },
		{ TEXT("Pentiment"), TEXT("PentimentEnemy") }, { TEXT("ThePalimpsest"), TEXT("OldPalimpsest") } };
	const FString* ClassName = Classes.Find(EnemyId);
	UClass* EnemyClass = ClassName ? StaticLoadClass(AUnderpaintingEnemy::StaticClass(), nullptr, *FString::Printf(TEXT("/Script/Underpainting.%s"), **ClassName)) : nullptr;
	if (!EnemyClass)
	{
		Note(FString::Printf(TEXT("TRIAL: no enemy called %s"), *EnemyId));
		return true;
	}
	for (TActorIterator<AUnderpaintingEnemy> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	// Stood on the ground seven metres ahead (found by looking down), and never nudged: a nudge can push it through the floor.
	// Whichever way has open floor: ahead first, then round the compass.
	FVector Where = Player->GetActorLocation();
	FCollisionQueryParams Query;
	Query.AddIgnoredActor(Player);
	for (int32 Turn = 0; Turn < 8; ++Turn)
	{
		const FVector Direction = Player->GetActorForwardVector().RotateAngleAxis(Turn * 45.0f, FVector::UpVector);
		const FVector Spot = Player->GetActorLocation() + Direction * 700.0f;
		FHitResult Ground, Wall;
		const bool bFloor = GetWorld()->LineTraceSingleByChannel(Ground, Spot + FVector(0.0f, 0.0f, 100.0f), Spot - FVector(0.0f, 0.0f, 600.0f), ECC_WorldStatic, Query);
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Wall, Player->GetActorLocation(), Spot + Direction * 150.0f, ECC_WorldStatic, Query);
		if (bFloor && !bBlocked)
		{
			Where = Ground.ImpactPoint;
			break;
		}
	}
	Where.Z += 160.0f;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AUnderpaintingEnemy* Fresh = GetWorld()->SpawnActor<AUnderpaintingEnemy>(EnemyClass, Where, (-Player->GetActorForwardVector()).Rotation(), Params);
	if (!Fresh)
	{
		Note(TEXT("TRIAL: could not spawn"));
		return true;
	}
	if (!Fresh->GetController())
	{
		Fresh->SpawnDefaultController();
	}
	TrialEnemy = Fresh;
	++TrialRound;
	TrialStartTime = TotalTime;
	TrialStartDamage = DamageTaken;
	TrialStartHits = HitsTaken;
	return false;
}

void UUnderpaintingAutopilot::StartRecording(const FString& Name, float Seconds, float Fps)
{
	RecordFolder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("FightClips") / Name);
	IFileManager::Get().DeleteDirectory(*RecordFolder, false, true);
	IFileManager::Get().MakeDirectory(*RecordFolder, true);
	RecordRemaining = Seconds;
	RecordInterval = 1.0f / FMath::Clamp(Fps, 1.0f, 30.0f);
	RecordAccumulated = 0.0f;
	RecordFrame = 0;
}

void UUnderpaintingAutopilot::CaptureFrame()
{
	FViewport* Viewport = GEngine && GEngine->GameViewport ? GEngine->GameViewport->Viewport : nullptr;
	TArray<FColor> Pixels;
	if (!Viewport || !Viewport->ReadPixels(Pixels))
	{
		return;
	}
	const FIntPoint Size = Viewport->GetSizeXY();
	if (Pixels.Num() != Size.X * Size.Y || Size.X < 16)
	{
		return;
	}

	// Halve big views so a clip stays small, then compress and write off the game thread.
	const int32 Skip = Size.X > 1400 ? 2 : 1;
	const int32 Width = Size.X / Skip, Height = Size.Y / Skip;
	TArray<FColor> Small;
	Small.SetNumUninitialized(Width * Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			FColor Pixel = Pixels[(Y * Skip) * Size.X + X * Skip];
			Pixel.A = 255;
			Small[Y * Width + X] = Pixel;
		}
	}
	const FString Path = RecordFolder / FString::Printf(TEXT("f%04d.png"), RecordFrame++);
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [Small = MoveTemp(Small), Width, Height, Path]()
	{
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(Small.GetData(), Small.Num()), Png);
		FFileHelper::SaveArrayToFile(Png, *Path);
	});
}
