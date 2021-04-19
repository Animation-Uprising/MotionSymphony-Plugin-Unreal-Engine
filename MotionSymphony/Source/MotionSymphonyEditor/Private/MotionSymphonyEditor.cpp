// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MotionSymphonyEditor.h"
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

	RegisterMotionDataAssetTypeActions(AssetTools, MakeShareable(new FAssetTypeActions_MotionDataAsset()));
	RegisterMotionMatchConfigAssetTypeActions(AssetTools, MakeShareable(new FAssetTypeActions_MotionMatchConfig()));
	RegisterMirroringProfileAssetTypeActions(AssetTools, MakeShareable(new FAssetTypeActions_MirroringProfile()));
	RegisterMotionCalibrationAssetTypeActions(AssetTools, MakeShareable(new FAssetTypeActions_MotionCalibration()));
	RegisterMMOptimisationTraitBinsAssetTypeActions(AssetTools, MakeShareable(new FAssetTypeActions_MMOptimisation_TraitBins()));
	RegisterMMOptimisationMultiClusteringAssetTypeActions(AssetTools, MakeShareable(new FAssetTypeActions_MMOptimisation_MultiClustering()));
}

void FMotionSymphonyEditorModule::RegisterMenuExtensions()
{
	
}

void FMotionSymphonyEditorModule::RegisterMotionDataAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MotionDataAsset> TypeActions)
{
	AssetTools.RegisterAssetTypeActions(TypeActions);
	RegisteredAssetTypeActions.Add(TypeActions);
}

void FMotionSymphonyEditorModule::RegisterMotionMatchConfigAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MotionMatchConfig> TypeActions)
{
	AssetTools.RegisterAssetTypeActions(TypeActions);
	RegisteredAssetTypeActions.Add(TypeActions);
}

void FMotionSymphonyEditorModule::RegisterMotionCalibrationAssetTypeActions(IAssetTools & AssetTools, TSharedRef<FAssetTypeActions_MotionCalibration> TypeActions)
{
	AssetTools.RegisterAssetTypeActions(TypeActions);
	RegisteredAssetTypeActions.Add(TypeActions);
}

void FMotionSymphonyEditorModule::RegisterMirroringProfileAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MirroringProfile> TypeActions)
{
	AssetTools.RegisterAssetTypeActions(TypeActions);
	RegisteredAssetTypeActions.Add(TypeActions);
}

void FMotionSymphonyEditorModule::RegisterMMOptimisationTraitBinsAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MMOptimisation_TraitBins> TypeActions)
{
	AssetTools.RegisterAssetTypeActions(TypeActions);
	RegisteredAssetTypeActions.Add(TypeActions);
}

void FMotionSymphonyEditorModule::RegisterMMOptimisationMultiClusteringAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MMOptimisation_MultiClustering> TypeActions)
{
	AssetTools.RegisterAssetTypeActions(TypeActions);
	RegisteredAssetTypeActions.Add(TypeActions);
}

//void FMotionSymphonyEditorModule::RegisterMMOptimisationLayeredAABBAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MirroringProfile> TypeActions)
//{
//	AssetTools.RegisterAssetTypeActions(TypeActions);
//	RegisteredAssetTypeActions.Add(TypeActions);
//}

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
	
	bool ResaveSettings = false;
	if (Settings->TraitNames.Num() > 64)
	{
		int32 Removecount = Settings->TraitNames.Num() - 64;
		for (int32 i = 0; i < Removecount; ++i)
		{
			Settings->TraitNames.RemoveAt(Settings->TraitNames.Num() - 1);
		}

		ResaveSettings = true;
	}

	if (ResaveSettings)
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