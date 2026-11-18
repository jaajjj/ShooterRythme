// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class FinalProject : ModuleRules
{
	public FinalProject(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate",
			"AudioMixer",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "Character"),
			Path.Combine(ModuleDirectory, "Logic"),
			Path.Combine(ModuleDirectory, "Variant_Horror"),
			Path.Combine(ModuleDirectory, "Variant_Horror/UI"),
			Path.Combine(ModuleDirectory, "Logic/Music"),
			Path.Combine(ModuleDirectory, "Variant_Shooter"),
			Path.Combine(ModuleDirectory, "Variant_Shooter/AI"),
			Path.Combine(ModuleDirectory, "Variant_Shooter/UI"),
			Path.Combine(ModuleDirectory, "Variant_Shooter/Weapons"),
			Path.Combine(ModuleDirectory, "Settings"),
			Path.Combine(ModuleDirectory, "UI")
		});
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
