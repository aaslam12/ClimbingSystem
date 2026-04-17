// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ClimbingSystem : ModuleRules
{
	public ClimbingSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "Animation"),
			Path.Combine(ModuleDirectory, "Character"),
			Path.Combine(ModuleDirectory, "Core"),
			Path.Combine(ModuleDirectory, "Data"),
			Path.Combine(ModuleDirectory, "Movement"),
			Path.Combine(ModuleDirectory, "Tests"),
			Path.Combine(ModuleDirectory, "Tests", "Helpers")
		});
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"GameplayTasks",
			"AnimGraphRuntime",
			"MotionWarping",
			"NetCore",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
