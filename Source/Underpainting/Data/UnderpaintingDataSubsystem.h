// Loads the game content in data/*.json into DataTables when the game starts.
// The JSON files stay the single source of truth: there are no imported table assets to keep in sync.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UnderpaintingDataTypes.h"
#include "UnderpaintingDataSubsystem.generated.h"

class UDataTable;

UCLASS()
class UUnderpaintingDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Folder that holds the JSON files (<project>/data). */
	static FString GetDataDirectory();

	/**
	 * Reads one JSON file into a new DataTable.
	 * @param FileStem		File name without extension, e.g. "pigments".
	 * @param RowStruct		Row struct the file maps to.
	 * @param OutProblems	Anything the importer complained about. Empty means a clean load.
	 * @return The table, or null if the file could not be read.
	 */
	static UDataTable* LoadTableFromJson(UObject* Outer, const FString& FileStem, UScriptStruct* RowStruct, TArray<FString>& OutProblems);

	/** Reads every data file again. Returns the number of problems found (0 is a clean load). */
	UFUNCTION(BlueprintCallable, Category = "Underpainting|Data")
	int32 ReloadAll();

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetPigments() const { return Pigments; }

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetRecipes() const { return Recipes; }

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetAttunements() const { return Attunements; }

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetEnemies() const { return Enemies; }

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetQuests() const { return Quests; }

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetRegions() const { return Regions; }

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetDialogue() const { return Dialogue; }

	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	UDataTable* GetLore() const { return Lore; }

	/** The zone row whose level is the one given (for example "L_Matriarch_Vault_A"), or None. */
	FName FindZoneForLevel(const FString& LevelName) const;

	/** Typed row lookups for C++. Return null if the row does not exist. */
	const FPigmentRow* FindPigment(FName RowName) const;
	const FItemRow* FindItem(FName RowName) const;
	const FRecipeRow* FindRecipe(FName RowName) const;
	const FAttunementRow* FindAttunement(FName RowName) const;
	const FEnemyRow* FindEnemy(FName RowName) const;
	const FQuestRow* FindQuest(FName RowName) const;
	const FRegionRow* FindRegion(FName RowName) const;

	/** Converts "#RRGGBB" from the data files into a colour. Returns magenta if the text is not a valid hex colour. */
	UFUNCTION(BlueprintPure, Category = "Underpainting|Data")
	static FLinearColor HexToColour(const FString& Hex);

private:

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Pigments;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Items;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Recipes;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Attunements;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Enemies;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Quests;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Regions;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Dialogue;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> Lore;
};
