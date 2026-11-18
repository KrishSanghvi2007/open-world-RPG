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
	Progress->SetTables(Data->GetPigments(), Data->GetItems(), Data->GetRecipes(), Data->GetAttunements(), Data->GetQuests(), Data->GetDialogue());

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

bool UUnderpaintingProgressSubsystem::SaveGame(APawn* Player)
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
		Progress->OnNotice.Broadcast(TEXT("Saved."));
	}
	UE_LOG(LogUnderpainting, Log, TEXT("Save %s"), bSaved ? TEXT("written") : TEXT("FAILED"));
	return bSaved;
}

bool UUnderpaintingProgressSubsystem::LoadGame()
{
	UUnderpaintingSaveGame* Save = HasSave() ? Cast<UUnderpaintingSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)) : nullptr;
	if (!Save)
	{
		return false;
	}

	LoadedSave = Save;
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
