// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MotorcycleAITask : ModuleRules
{
    public MotorcycleAITask(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "StateTreeModule", "GameplayStateTreeModule", "AIModule", "GameplayTags", "NavigationSystem", "Niagara" });
    }
}
