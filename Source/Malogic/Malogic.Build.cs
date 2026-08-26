// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Malogic : ModuleRules
{
	public Malogic(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"GameplayTags",
            "GameplayTasks",
            "GameplayAbilities",
            "InputCore", 
			"Lua",
			"UnLua",
			"ModularGameplay",
			"ModularGameplayActors",
			"NetCore",
			"EnhancedInput",
			"ModelViewViewModel"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		PublicIncludePaths.AddRange(new string[] { 
			"Malogic"
		});
        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
