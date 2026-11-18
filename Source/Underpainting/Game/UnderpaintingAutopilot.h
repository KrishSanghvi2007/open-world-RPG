// A development tool, not part of the game: it plays the Limner from a script, by real movement input and the
// same functions the keys call, and it can save what the screen shows as frames. It is how routes and fights are
// timed and recorded when nobody is at the keyboard (tools/dev/pilot.py).
//
// A script is steps separated by semicolons. Distances are metres. The verbs:
//   walk x y [z] | sprint x y [z]     go to a point (it jumps if it sticks; if it cannot get there it says so and teleports)
//   walkto Kind Id [n] | sprintto ... go to the n-th marker of that kind and data ID (the tags the level builder sets)
//   look Kind Id [n]                  turn the view onto a marker (Use follows the view)
//   use | menu N | close | jump       the Use key, a number key, close the menu, jump
//   talk [r1 r2 ...]                  talk to whoever is in view, pressing these numbers on successive lines, reading each line
//   wait S                            do nothing for S seconds
//   light | heavy | stroke | dodge | lock | flare | swap | poultice     one press
//   follow on [d] | follow off        keep walking up to the nearest enemy (to within d metres) while other steps run
//   fight [skill] [timeout]           fight whatever is near until it is dead: skill 0..1 is how often an attack is dodged
//   mark Label                        note the time under a name
//   god on|off, grant Id N, attune Node, tp x y z, hurt N, fast on|off       test conveniences (fast skips reading time)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnderpaintingAutopilot.generated.h"

class AUnderpaintingEnemy;
class AUnderpaintingPlayerCharacter;

UCLASS()
class UUnderpaintingAutopilot : public UActorComponent
{
	GENERATED_BODY()

public:

	UUnderpaintingAutopilot();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Starts a script, replacing any that is running. */
	void Run(const FString& Script);

	/** Where the script has got to, the times noted, and anything that went wrong. */
	FString Describe() const;

	/** Saves about Fps frames a second of the game view, with the HUD, into Saved/FightClips/<Name>/ for Seconds of real time. */
	void StartRecording(const FString& Name, float Seconds, float Fps);

	bool IsRunning() const { return Steps.IsValidIndex(Index); }

private:

	struct FStep
	{
		FString Verb;
		TArray<FString> Args;
	};

	AUnderpaintingPlayerCharacter* GetPlayer() const;
	bool TickStep(const FStep& Step, float DeltaTime);
	bool TickWalk(const FVector& Target, bool bSprint, float StopDistance, float DeltaTime);
	bool TickFight(float Skill, float Timeout, float DeltaTime);
	bool TickTalk(const FStep& Step, float DeltaTime);
	bool TickTrial(const FStep& Step, float DeltaTime);
	AActor* FindMarker(const FStep& Step, int32 FirstArg) const;
	AUnderpaintingEnemy* NearestEnemy(float Range) const;
	void Note(const FString& Text);
	void CaptureFrame();
	float Arg(const FStep& Step, int32 Index, float Default) const;

	TArray<FStep> Steps;
	int32 Index = INDEX_NONE;
	float StepTime = 0.0f;
	float TotalTime = 0.0f;
	bool bFast = false;
	TArray<FString> Notes;

	// Walking.
	FVector LastProgressLocation = FVector::ZeroVector;
	float SinceProgress = 0.0f;
	float WalkedDistance = 0.0f;

	/** While walking, fight whatever attacks with this skill (below 0: walk on regardless). */
	float GuardSkill = -1.0f;
	float GuardTime = 0.0f;
	float LastDrinkTime = -100.0f;

	// Background follow.
	bool bFollow = false;
	float FollowDistance = 170.0f;

	// Fighting.
	TWeakObjectPtr<AUnderpaintingEnemy> ThreatSeen;
	FName ThreatAttack;
	bool bThreatDecided = false;
	bool bWillDodge = false;
	int32 HitsTaken = 0;
	float LastHealth = 0.0f;
	int32 Dodges = 0;
	float DamageTaken = 0.0f;

	// Trials: the same fight several times over, to measure how hard an enemy is.
	TWeakObjectPtr<AUnderpaintingEnemy> TrialEnemy;
	int32 TrialRound = 0;
	float TrialStartTime = 0.0f;
	float TrialStartDamage = 0.0f;
	int32 TrialStartHits = 0;
	TArray<float> TrialDamages;
	TArray<float> TrialTimes;

	// Talking.
	int32 TalkLine = 0;
	float TalkWait = 0.0f;
	bool bTalkStarted = false;

	// Recording.
	FString RecordFolder;
	float RecordRemaining = 0.0f;
	float RecordInterval = 0.066f;
	float RecordAccumulated = 0.0f;
	int32 RecordFrame = 0;
};
