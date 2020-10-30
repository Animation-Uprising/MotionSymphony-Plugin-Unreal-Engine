// Copyright 2020 Kenneth Claassen. All Rights Reserved.

using UnrealBuildTool;

public class MotionSymphony : ModuleRules
{
	public MotionSymphony(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                "MotionSymphony/Private",
                "MotionSymphony/Private/AnimGraph",
                "MotionSymphony/Private/Components",
                "MotionSymphony/Private/CustomAssets",
                "MotionSymphony/Private/Data",
                "MotionSymphony/Private/MotionMatchingUtil"
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "AnimationModifiers"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Projects",
                "InputCore",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{

				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
