// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "IAssetTools.h"
#include "AssetTypeActions_MotionDataAsset.h"
#include "AssetTypeActions_MotionMatchConfig.h"

class FMotionSymphonyEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual TSharedPtr<FExtensibilityManager> GetPreProcessToolkitToolbarExtensibilityManager() { return PreProcessToolkit_ToolbarExtManager;}

private:
	TSharedPtr<FExtensibilityManager> PreProcessToolkit_ToolbarExtManager;

	void RegisterAssetTools();
	void RegisterMenuExtensions();
	void RegisterMotionDataAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MotionDataAsset> TypeActions);
	void RegisterMotionMatchConfigAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MotionMatchConfig> TypeActions);

	void UnRegisterAssetTools();
	void UnRegisterMenuExtensions();
	void UnRegisterAssetTypeActions();

	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
};
