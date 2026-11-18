#include "UnderpaintingSettings.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* SettingsSlot = TEXT("UnderpaintingSettings");
}

const TArray<FUnderpaintingAction>& UUnderpaintingSettingsSave::Actions()
{
	static const TArray<FUnderpaintingAction> List = {
		{ TEXT("Use"), TEXT("Use / talk / next"), EKeys::E },
		{ TEXT("LightAttack"), TEXT("Light attack"), EKeys::LeftMouseButton },
		{ TEXT("HeavyAttack"), TEXT("Heavy attack (hold: Stroke)"), EKeys::RightMouseButton },
		{ TEXT("Dodge"), TEXT("Dodge"), EKeys::LeftControl },
		{ TEXT("Sprint"), TEXT("Sprint"), EKeys::LeftShift },
		{ TEXT("LockOn"), TEXT("Lock on / close menu"), EKeys::Q },
		{ TEXT("SwapWell"), TEXT("Swap palette well"), EKeys::Z },
		{ TEXT("Flare"), TEXT("Flare the lantern"), EKeys::F },
		{ TEXT("Flask"), TEXT("Throw Fixative Flask"), EKeys::G },
		{ TEXT("Poultice"), TEXT("Moss Poultice"), EKeys::H },
		{ TEXT("LampOil"), TEXT("Lamp Oil"), EKeys::J },
		{ TEXT("Draught"), TEXT("Sanguine Draught"), EKeys::K },
		{ TEXT("Tincture"), TEXT("Blue Tincture"), EKeys::L },
		{ TEXT("Pack"), TEXT("Open the satchel"), EKeys::Tab },
	};
	return List;
}

FKey UUnderpaintingSettingsSave::GetKey(FName Action) const
{
	if (const FKey* Chosen = Keys.Find(Action))
	{
		if (Chosen->IsValid())
		{
			return *Chosen;
		}
	}
	for (const FUnderpaintingAction& Entry : Actions())
	{
		if (Entry.Id == Action)
		{
			return Entry.DefaultKey;
		}
	}
	return EKeys::Invalid;
}

void UUnderpaintingSettingsSave::SetKey(FName Action, FKey Key)
{
	if (!Key.IsValid())
	{
		return;
	}
	const FKey Old = GetKey(Action);
	for (const FUnderpaintingAction& Entry : Actions())
	{
		if (Entry.Id != Action && GetKey(Entry.Id) == Key)
		{
			Keys.Add(Entry.Id, Old);        // the other action takes the key this one is giving up
		}
	}
	Keys.Add(Action, Key);
}

FString UUnderpaintingSettingsSave::DescribeKey(FName Action) const
{
	FString Name = GetKey(Action).GetDisplayName(false).ToString();
	Name.ReplaceInline(TEXT("Mouse Button"), TEXT("Mouse"));
	return Name;
}

float UUnderpaintingSettingsSave::GetUIScale() const
{
	static const float Steps[] = { 0.85f, 1.0f, 1.15f, 1.3f };
	return Steps[FMath::Clamp(TextSizeStep, 0, 3)];
}

void UUnderpaintingSettings::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Save = Cast<UUnderpaintingSettingsSave>(UGameplayStatics::DoesSaveGameExist(SettingsSlot, 0) ? UGameplayStatics::LoadGameFromSlot(SettingsSlot, 0) : nullptr);
	if (!Save)
	{
		Save = NewObject<UUnderpaintingSettingsSave>(this);
	}
}

UUnderpaintingSettings* UUnderpaintingSettings::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UUnderpaintingSettings>() : nullptr;
}

void UUnderpaintingSettings::Write()
{
	UGameplayStatics::SaveGameToSlot(Save, SettingsSlot, 0);
}

void UUnderpaintingSettings::SetKey(FName Action, FKey Key)
{
	Save->SetKey(Action, Key);
	Write();
	OnKeysChanged.Broadcast();
}

void UUnderpaintingSettings::ResetKeys()
{
	Save->ResetKeys();
	Write();
	OnKeysChanged.Broadcast();
}

void UUnderpaintingSettings::CycleTextSize()
{
	Save->TextSizeStep = (Save->TextSizeStep + 1) % 4;
	Write();
}

void UUnderpaintingSettings::ToggleKeyHints()
{
	Save->bKeyHints = !Save->bKeyHints;
	Write();
}

void UUnderpaintingSettings::ToggleTeaching()
{
	Save->bTeaching = !Save->bTeaching;
	Write();
}

void UUnderpaintingSettings::ToggleObjectiveMarker()
{
	Save->bObjectiveMarker = !Save->bObjectiveMarker;
	Write();
}
