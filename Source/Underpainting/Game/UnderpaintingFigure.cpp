#include "UnderpaintingFigure.h"

#include "Underpainting.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

bool FUnderpaintingFigure::Setup(USkeletalMeshComponent* Component, FName Id)
{
	if (!Component || Id.IsNone())
	{
		return false;
	}
	const FString Path = FString::Printf(TEXT("/Game/Folded/%s/SK_%s.SK_%s"), *Id.ToString(), *Id.ToString(), *Id.ToString());
	USkeletalMesh* Model = LoadObject<USkeletalMesh>(nullptr, *Path, nullptr, LOAD_NoWarn);
	if (!Model)
	{
		UE_LOG(LogUnderpainting, Warning, TEXT("No model for '%s' yet (%s). Run /make-character %s."), *Id.ToString(), *Path, *Id.ToString());
		return false;
	}
	FigureId = Id;
	Mesh = Component;
	Mesh->SetSkeletalMesh(Model);
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(true);
	Mesh->bCastDynamicShadow = true;
	Mesh->SetVisibility(true);
	Playing = NAME_None;
	return true;
}

UAnimSequence* FUnderpaintingFigure::Find(FName Name)
{
	if (TObjectPtr<UAnimSequence>* Known = Loaded.Find(Name))
	{
		return Known->Get();
	}
	const FString Asset = FString::Printf(TEXT("%s_%s"), *FigureId.ToString(), *Name.ToString());
	const FString Path = FString::Printf(TEXT("/Game/Folded/%s/Anims/%s.%s"), *FigureId.ToString(), *Asset, *Asset);
	UAnimSequence* Found = LoadObject<UAnimSequence>(nullptr, *Path, nullptr, LOAD_NoWarn);
	Loaded.Add(Name, Found);
	return Found;
}

bool FUnderpaintingFigure::Play(FName Name, bool bLoop, float Rate, bool bRestart)
{
	if (!Mesh)
	{
		return false;
	}
	if (Playing == Name && !bRestart)
	{
		Mesh->SetPlayRate(Rate);
		return true;
	}
	UAnimSequence* Movement = Find(Name);
	if (!Movement)
	{
		return false;
	}
	Mesh->PlayAnimation(Movement, bLoop);
	Mesh->SetPlayRate(Rate);
	Playing = Name;
	return true;
}

float FUnderpaintingFigure::Length(FName Name)
{
	const UAnimSequence* Movement = Find(Name);
	return Movement ? Movement->GetPlayLength() : 0.0f;
}
