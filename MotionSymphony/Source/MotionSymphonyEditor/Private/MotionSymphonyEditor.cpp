// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "MotionSymphonyEditor.h"
#include "Templates/SharedPointer.h"
#include "MotionSymphonyStyle.h"

#define LOCTEXT_NAMESPACE "FMotionSymphonyEditorModule"

void FMotionSymphonyEditorModule::StartupModule()
{
	PreProcessToolkit_ToolbarExtManager = MakeShareable(new FExtensibilityManager);
	FMotionSymphonyStyle::Initialize();

	RegisterAssetTools();
	RegisterMenuExtensions();
}

void FMotionSymphonyEditorModule::ShutdownModule()
{
	PreProcessToolkit_ToolbarExtManager.Reset();
	FMotionSymphonyStyle::Shutdown();

	UnRegisterAssetTools();
	UnRegisterMenuExtensions();
}

void FMotionSymphonyEditorModule::RegisterAssetTools()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	RegisterMotionDataAssetTypeActions(AssetTools, MakeShareable(new FAssetTypeActions_MotionDataAsset()));
	RegisterMotionMatchConfigAssetTypeActions(AssetTools, MakeShareable(new FAssetTypeActions_MotionMatchConfig()));
}

void FMotionSymphonyEditorModule::RegisterMenuExtensions()
{
	
}

void FMotionSymphonyEditorModule::RegisterMotionDataAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MotionDataAsset> TypeActions)
{
	AssetTools.RegisterAssetTypeActions(TypeActions);
	RegisteredAssetTypeActions.Add(TypeActions);
}

void FMotionSymphonyEditorModule::RegisterMotionMatchConfigAssetTypeActions(IAssetTools & AssetTools, TSharedRef<FAssetTypeActions_MotionMatchConfig> TypeActions)
{
	AssetTools.RegisterAssetTypeActions(TypeActions);
	RegisteredAssetTypeActions.Add(TypeActions);
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

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMotionSymphonyEditorModule, MotionSymphonyEditor)