// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CompassKitProject : ModuleRules
{
	public CompassKitProject(ReadOnlyTargetRules Target) : base(Target)
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
			"CompassKitProject",
			"CompassKitProject/Variant_Platforming",
			"CompassKitProject/Variant_Platforming/Animation",
			"CompassKitProject/Variant_Combat",
			"CompassKitProject/Variant_Combat/AI",
			"CompassKitProject/Variant_Combat/Animation",
			"CompassKitProject/Variant_Combat/Gameplay",
			"CompassKitProject/Variant_Combat/Interfaces",
			"CompassKitProject/Variant_Combat/UI",
			"CompassKitProject/Variant_SideScrolling",
			"CompassKitProject/Variant_SideScrolling/AI",
			"CompassKitProject/Variant_SideScrolling/Gameplay",
			"CompassKitProject/Variant_SideScrolling/Interfaces",
			"CompassKitProject/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
