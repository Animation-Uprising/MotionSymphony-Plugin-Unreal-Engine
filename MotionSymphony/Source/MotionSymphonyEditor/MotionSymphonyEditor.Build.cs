// Copyright 2020 Kenneth Claassen. All Rights Reserved.

using UnrealBuildTool;

public class MotionSymphonyEditor : ModuleRules
{
	public MotionSymphonyEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                "MotionSymphonyEditor/Private",
                "MotionSymphonyEditor/Private/AssetTools",
               // "MotionSymphonyEditor/Private/CustomAssets",
                "MotionSymphonyEditor/Private/Factories",
                "MotionSymphonyEditor/Private/Toolkits",
               // "MotionSymphonyEditor/Private/Widgets"
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "AnimGraph",
                "AnimGraphRuntime",
                "MotionSymphony"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
                "Json",
                "Slate",
                "SlateCore",
                "Engine",
                "InputCore",
                "UnrealEd", // for FAssetEditorManager
				"KismetWidgets",
                "Kismet",  // for FWorkflowCentricApplication
				"PropertyEditor",
                "RenderCore",
                "ContentBrowser",
                "WorkspaceMenuStructure",
                "EditorStyle",
                "MeshPaint",
                "EditorWidgets",
                "Projects",
                "MotionSymphony",
                "AnimGraph",
                "BlueprintGraph",
                "AssetRegistry",
                "AdvancedPreviewScene",
                "AnimGraphRuntime",
                "ToolMenus"
				// ... add private dependencies that you statically link with here ...	
			}
			);

        PrivateIncludePathModuleNames.AddRange(
           new string[] {
                "Settings",
                "IntroTutorials",
                "AssetTools",
                "LevelEditor",
           }
           );



        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
                "AssetTools"
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
