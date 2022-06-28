// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MotionSymphonyEditor.h"

#include "AssetTypeActions_MatchFeature.h"
#include "Templates/SharedPointer.h"
#include "MotionSymphonyStyle.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"
#include "MotionSymphonySettings.h"

#define LOCTEXT_NAMESPACE "FMotionSymphonyEditorModule"

void FMotionSymphonyEditorModule::StartupModule()
{
	PreProcessToolkit_ToolbarExtManager = MakeShareable(new FExtensibilityManager);
	FMotionSymphonyStyle::Initialize();

	RegisterSettings();
	RegisterAssetTools();
	RegisterMenuExtensions();
	
}

void FMotionSymphonyEditorModule::ShutdownModule()
{
	PreProcessToolkit_ToolbarExtManager.Reset();
	FMotionSymphonyStyle::Shutdown();

	UnRegisterAssetTools();
	UnRegisterMenuExtensions();

	if (UObjectInitialized())
	{
		UnRegisterSettings();
	}
}

void FMotionSymphonyEditorModule::RegisterAssetTools()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MotionDataAsset()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MotionMatchConfig()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MirroringProfile()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MotionCalibration()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MMOptimisation_TraitBins()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MMOptimisation_MultiClustering()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MatchFeatureBoneLocation()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MatchFeatureBoneVelocity()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MatchFeatureBoneHeight()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MatchFeatureBodyMomentum2D()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MatchFeatureBodyMomentumRot()));
	RegisterAssetTool(AssetTools, MakeShareable(new FAssetTypeActions_MatchFeatureTrajectory2D()));
}

void FMotionSymphonyEditorModule::RegisterMenuExtensions()
{
	
}

void FMotionSymphonyEditorModule::UnRegisterAssetTools()
{
	
}

void FMotionSymphonyEditorModule::UnRegisterMenuExtensions()
{
	
}

void FMotionSymphonyEditorModule::UnRegisterAssetTypeActions()
{
}

bool FMotionSymphonyEditorModule::HandleSettingsSaved()
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();
	
	bool bReSaveSettings = false;
	if (Settings->TraitNames.Num() > 64)
	{
		const int32 RemoveCount = Settings->TraitNames.Num() - 64;
		for (int32 i = 0; i < RemoveCount; ++i)
		{
			Settings->TraitNames.RemoveAt(Settings->TraitNames.Num() - 1);
		}

		bReSaveSettings = true;
	}

	if (bReSaveSettings)
	{
		Settings->SaveConfig();
	}

	return true;
}

void FMotionSymphonyEditorModule::RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		//Create a new category
		ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Project");

		SettingsContainer->DescribeCategory("MotionSymphonySettings",
			LOCTEXT("RuntimeWDCategoryName", "MotionSymphonySettings"),
			LOCTEXT("RuntimeWDCategoryDescription", "Game configuration for motion symphony plugin module"));

		//Register settings
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "MotionSymphony",
			LOCTEXT("RuntimeGeneralSettingsName", "MotionSymphony"),
			LOCTEXT("RuntimeGeneratlSettingsDescription", "Base configuration for motion symphony plugin module"),
			GetMutableDefault<UMotionSymphonySettings>());

		//Register the save handler to your settings for validation checks
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FMotionSymphonyEditorModule::HandleSettingsSaved);
		}
	}
}

void FMotionSymphonyEditorModule::UnRegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "MotionSymphony");
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMotionSymphonyEditorModule, MotionSymphonyEditor)