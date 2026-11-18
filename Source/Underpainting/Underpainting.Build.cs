// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Underpainting : ModuleRules
{
	public Underpainting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Underpainting",
			"Underpainting/Data",
			"Underpainting/Combat",
			"Underpainting/Variant_Combat",
			"Underpainting/Variant_Combat/AI",
			"Underpainting/Variant_Combat/Animation",
			"Underpainting/Variant_Combat/Gameplay",
			"Underpainting/Variant_Combat/Interfaces",
			"Underpainting/Variant_Combat/UI",
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
