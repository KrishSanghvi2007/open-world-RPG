#include "UnderpaintingProgressSubsystem.h"

#include "Underpainting.h"
#include "UnderpaintingDataSubsystem.h"
#include "UnderpaintingVitalsComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

void UUnderpaintingProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UUnderpaintingDataSubsystem* Data = Collection.InitializeDependency<UUnderpaintingDataSubsystem>();
	Progress = NewObject<UUnderpaintingProgress>(this);
	Progress->SetTables(Data->GetPigments(), Data->GetItems(), Data->GetRecipes(), Data->GetAttunements(), Data->GetQuests(), Data->GetDialogue(), Data->GetLore());

	if (!LoadGame())
	{
		Progress->NewGame();
	}
}

UUnderpaintingProgress* UUnderpaintingProgressSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UUnderpaintingProgressSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UUnderpaintingProgressSubsystem>() : nullptr;
	return Subsystem ? Subsystem->GetProgress() : nullptr;
}

bool UUnderpaintingProgressSubsystem::HasSave() const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName(), 0);
}

bool UUnderpaintingProgressSubsystem::SaveGame(APawn* Player, bool bQuiet)
{
	UUnderpaintingSaveGame* Save = Cast<UUnderpaintingSaveGame>(UGameplayStatics::CreateSaveGameObject(UUnderpaintingSaveGame::StaticClass()));
	Save->Progress = Progress->GetData();
	if (Player)
	{
		Save->LevelName = FName(*UGameplayStatics::GetCurrentLevelName(Player, true));
		Save->PlayerTransform = Player->GetActorTransform();
		if (const UUnderpaintingVitalsComponent* Vitals = Player->FindComponentByClass<UUnderpaintingVitalsComponent>())
		{
			Save->Health = Vitals->GetHealth();
			Save->Glim = Vitals->GetGlim();
		}
	}

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SlotName(), 0);
	if (bSaved)
	{
		LoadedSave = Save;
		if (!bQuiet)
		{
			Progress->OnNotice.Broadcast(TEXT("Saved."));
		}
	}
	UE_LOG(LogUnderpainting, Log, TEXT("Save %s"), bSaved ? TEXT("written") : TEXT("FAILED"));
	return bSaved;
}

bool UUnderpaintingProgressSubsystem::SaveProgressOnly()
{
	if (!LoadedSave)
	{
		return false;
	}
	LoadedSave->Progress = Progress->GetData();
	return UGameplayStatics::SaveGameToSlot(LoadedSave, SlotName(), 0);
}

bool UUnderpaintingProgressSubsystem::LoadGame()
{
	UUnderpaintingSaveGame* Save = HasSave() ? Cast<UUnderpaintingSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)) : nullptr;
	if (!Save)
	{
		return false;
	}

	LoadedSave = Save;
	bRestorePending = true;
	Progress->SetData(Save->Progress);
	Progress->OnNotice.Broadcast(TEXT("Loaded."));
	UE_LOG(LogUnderpainting, Log, TEXT("Save loaded"));
	return true;
}

void UUnderpaintingProgressSubsystem::NewGame()
{
	UGameplayStatics::DeleteGameInSlot(SlotName(), 0);
	LoadedSave = nullptr;
	Progress->NewGame();
	Progress->OnNotice.Broadcast(TEXT("New game."));
}

bool UUnderpaintingProgressSubsystem::TravelToZone(const UObject* WorldContext, FName TargetZone)
{
	const UUnderpaintingDataSubsystem* Data = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
	const FRegionRow* Target = Data ? Data->FindRegion(TargetZone) : nullptr;
	const FName Level = GetLevelForZone(TargetZone);
	if (!Target || Level.IsNone())
	{
		UE_LOG(LogUnderpainting, Warning, TEXT("No level for zone '%s'"), *TargetZone.ToString());
		return false;
	}

	ArrivingFromZone = Data->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(WorldContext, true));
	StashVitals(UGameplayStatics::GetPlayerPawn(WorldContext, 0));
	bRestorePending = false;
	UGameplayStatics::OpenLevel(WorldContext, FName(*FString::Printf(TEXT("/Game/Maps/Matriarch/%s"), *Level.ToString())));
	return true;
}

FName UUnderpaintingProgressSubsystem::ConsumeArrival()
{
	const FName Result = ArrivingFromZone;
	ArrivingFromZone = NAME_None;
	return Result;
}

bool UUnderpaintingProgressSubsystem::ConsumeRestore()
{
	const bool bResult = bRestorePending;
	bRestorePending = false;
	return bResult;
}

FName UUnderpaintingProgressSubsystem::GetLevelForZone(FName Zone) const
{
	const UUnderpaintingDataSubsystem* Data = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
	const FRegionRow* Row = Data ? Data->FindRegion(Zone) : nullptr;
	if (!Row)
	{
		return NAME_None;
	}
	return Progress->HasTurned() && !Row->LevelAfter.IsNone() ? Row->LevelAfter : Row->LevelBefore;
}

bool UUnderpaintingProgressSubsystem::CompleteTurning(APawn* Player)
{
	const UUnderpaintingDataSubsystem* Data = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
	if (!Player || !Data || Progress->HasTurned())
	{
		return false;
	}
	const FName Zone = Data->FindZoneForLevel(UGameplayStatics::GetCurrentLevelName(Player, true));
	Progress->SetFlag(UUnderpaintingProgress::TurnedFlag);
	const FName Level = GetLevelForZone(Zone);
	if (Level.IsNone())
	{
		return false;        // not standing in a zone (a test room): the world is turned, but there is nothing to reload
	}

	StashVitals(Player);
	KeptTransform = Player->GetActorTransform();
	bHasKeptTransform = true;
	bJustTurned = true;
	bRestorePending = false;
	ArrivingFromZone = NAME_None;
	UGameplayStatics::OpenLevel(Player, FName(*FString::Printf(TEXT("/Game/Maps/Matriarch/%s"), *Level.ToString())));
	return true;
}

bool UUnderpaintingProgressSubsystem::ConsumeKeptTransform(FTransform& OutTransform)
{
	if (!bHasKeptTransform)
	{
		return false;
	}
	OutTransform = KeptTransform;
	bHasKeptTransform = false;
	return true;
}

void UUnderpaintingProgressSubsystem::StashVitals(const APawn* Player)
{
	const UUnderpaintingVitalsComponent* Vitals = Player ? Player->FindComponentByClass<UUnderpaintingVitalsComponent>() : nullptr;
	if (Vitals && !Vitals->IsDead())
	{
		CarriedHealth = Vitals->GetHealth();
		CarriedGlim = Vitals->GetGlim();
	}
}

bool UUnderpaintingProgressSubsystem::ConsumeVitals(float& OutHealth, float& OutGlim)
{
	if (CarriedHealth < 0.0f)
	{
		return false;
	}
	OutHealth = CarriedHealth;
	OutGlim = CarriedGlim;
	CarriedHealth = -1.0f;
	return true;
}

FName UUnderpaintingProgressSubsystem::GetSavedLevel() const
{
	const UUnderpaintingDataSubsystem* Data = GetGameInstance()->GetSubsystem<UUnderpaintingDataSubsystem>();
	if (!LoadedSave || LoadedSave->LevelName.IsNone() || !Data)
	{
		return NAME_None;
	}
	const FName Zone = Data->FindZoneForLevel(LoadedSave->LevelName.ToString());
	return Zone.IsNone() ? LoadedSave->LevelName : GetLevelForZone(Zone);
}
