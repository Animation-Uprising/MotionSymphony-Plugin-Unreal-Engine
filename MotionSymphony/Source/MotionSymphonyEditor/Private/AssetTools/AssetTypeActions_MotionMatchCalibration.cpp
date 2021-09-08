// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AssetTypeActions_MotionMatchCalibration.h"
#include "CustomAssets/MotionCalibration.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_MotionCalibration::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MotionCalibration", "Motion Calibration");
}

FColor FAssetTypeActions_MotionCalibration::GetTypeColor() const
{
	return FColor::Blue;
}

UClass* FAssetTypeActions_MotionCalibration::GetSupportedClass() const
{
	return UMotionCalibration::StaticClass();
}

uint32 FAssetTypeActions_MotionCalibration::GetCategories()
{
	return EAssetTypeCategories::Animation;
}
//
//void FAssetTypeActions_MotionCalibration::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder & MenuBuilder)
//{
//	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);
//
//	auto MotionPreProcessors = GetTypedWeakObjectPtrs<UMotionDataAsset>(InObjects);
//
//	MenuBuilder.AddMenuEntry(
//		LOCTEXT("MotionDataAsset_RunPreProcess", "Run Pre-Process"),
//		LOCTEXT("MotionDataAsset_RunPreProcessToolTip", "Runs the pre-processing algorithm on the data in this pre-processor."),
//		FSlateIcon(),
//		FUIAction(
//			FExecuteAction::CreateLambda([=] {
//
//			}),
//			FCanExecuteAction::CreateLambda([=] {
//				return true;
//			})
//		)
//	);
//}

bool FAssetTypeActions_MotionCalibration::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MotionCalibration::CanFilter()
{
	return true;
}

#undef LOCTEXT_NAMESPACE