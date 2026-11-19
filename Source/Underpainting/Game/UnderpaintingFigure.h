// A drawn character's model and movements: one of the Folded (docs/story/CAST_IN_GAME.md).
// Made by the /make-character pipeline, which always puts things in the same places:
//   mesh        /Game/Folded/<Id>/SK_<Id>
//   movements   /Game/Folded/<Id>/Anims/<Id>_<Name>      (Idle, Walk, ...: whatever tools/characters/anims/<Id>.py makes)
// This helper finds them by name and plays them one at a time. No animation blueprint: the game's rules already know
// what the character is doing (waiting, winding up, struck), and each of those is one movement.

#pragma once

#include "CoreMinimal.h"
#include "UnderpaintingFigure.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;

USTRUCT()
struct FUnderpaintingFigure
{
	GENERATED_BODY()

	/** Puts the model for Id on the component. Returns false (and changes nothing) if it has not been made yet. */
	bool Setup(USkeletalMeshComponent* Component, FName Id);

	bool IsValid() const { return Mesh != nullptr; }

	/** Plays a movement if it is not already the one playing. Rate 1 is as drawn. Returns false if there is no such movement. */
	bool Play(FName Name, bool bLoop = true, float Rate = 1.0f, bool bRestart = false);

	/** Seconds the movement lasts at rate 1, or 0 if there is none. */
	float Length(FName Name);

	FName GetPlaying() const { return Playing; }

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Mesh;

private:

	UAnimSequence* Find(FName Name);

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UAnimSequence>> Loaded;

	FName FigureId;
	FName Playing;
};
