// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "IAssetTools.h"
#include "AssetTypeActions_MotionDataAsset.h"
#include "AssetTypeActions_MotionMatchCalibration.h"
#include "AssetTypeActions_MotionMatchConfig.h"
#include "AssetTypeActions_MirroringProfile.h"
#include "AssetTypeActions_MMOptimisation_TraitBins.h"
#include "AssetTypeActions_MMOptimisation_MultiClustering.h"

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
	void RegisterMotionCalibrationAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MotionCalibration> TypeActions);
	void RegisterMirroringProfileAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MirroringProfile> TypeActions);
	void RegisterMMOptimisationTraitBinsAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MMOptimisation_TraitBins> TypeActions);
	void RegisterMMOptimisationMultiClusteringAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MMOptimisation_MultiClustering> TypeActions);
	//void RegisterMMOptimisationLayeredAABBAssetTypeActions(IAssetTools& AssetTools, TSharedRef<FAssetTypeActions_MMOptimisation_LayeredAABB> TypeActions);

	void UnRegisterAssetTools();
	void UnRegisterMenuExtensions();
	void UnRegisterAssetTypeActions();

	bool HandleSettingsSaved();
	void RegisterSettings();
	void UnRegisterSettings();

	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
};
