// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

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
				"MotionSymphony/Public",
                "MotionSymphony/Public/AnimGraph",
                "MotionSymphony/Public/Components",
                "MotionSymphony/Public/CustomAssets",
                "MotionSymphony/Public/Data",
                "MotionSymphony/Public/MotionMatchingUtil",
				"MotionSymphony/Public/Enumerations",
				"MotionSymphony/Public/Objects",
				"MotionSymphony/Public/Objects/Tags",

                "MotionSymphony/Private",
                "MotionSymphony/Private/AnimGraph",
                "MotionSymphony/Private/Components",
                "MotionSymphony/Private/CustomAssets",
                "MotionSymphony/Private/Data",
                "MotionSymphony/Private/MotionMatchingUtil",
				"MotionSymphony/Private/Objects",
				"MotionSymphony/Private/Objects/Tags"
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine", 
				"AnimGraphRuntime"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
                "Engine"
                // ... add private dependencies that you statically link with here ...	
			}
			);

		//Add editor only modules
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("AnimationModifiers");
        }

        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
    }
}
