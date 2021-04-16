// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AssetTypeActions_MotionDataAsset.h"
#include "CustomAssets/MotionDataAsset.h"
#include "MotionPreProcessToolkit.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//FAssetTypeActions_MotionDataAsset::FAssetTypeActions_MotionDataAsset(const TSharedRef<ISlateStyle>& InStyle)
//{
//	Style = InStyle;
//}

FText FAssetTypeActions_MotionDataAsset::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MotionData", "Motion Data");
}

FColor FAssetTypeActions_MotionDataAsset::GetTypeColor() const
{
	return FColor::Blue;
}

UClass * FAssetTypeActions_MotionDataAsset::GetSupportedClass() const
{
	return UMotionDataAsset::StaticClass();
}

void FAssetTypeActions_MotionDataAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
		? EToolkitMode::WorldCentric
		: EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto PreProcessAsset = Cast<UMotionDataAsset>(*ObjIt);

		if (PreProcessAsset != nullptr)
		{
			TSharedRef<FMotionPreProcessToolkit> EditorToolkit = MakeShareable(new FMotionPreProcessToolkit());
			EditorToolkit->Initialize(PreProcessAsset, Mode, EditWithinLevelEditor);
		}
	}
}

uint32 FAssetTypeActions_MotionDataAsset::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

void FAssetTypeActions_MotionDataAsset::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder & MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto MotionPreProcessors = GetTypedWeakObjectPtrs<UMotionDataAsset>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MotionDataAsset_RunPreProcess", "Run Pre-Process"),
		LOCTEXT("MotionDataAsset_RunPreProcessToolTip", "Runs the pre-processing algorithm on the data in this pre-processor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=] {

				}),
			FCanExecuteAction::CreateLambda([=] {
					return true;
				})
					)
	);
}

bool FAssetTypeActions_MotionDataAsset::HasActions(const TArray<UObject*>& InObjects) const
{
	//return true;
	return false;
}

bool FAssetTypeActions_MotionDataAsset::CanFilter()
{
	return true;
}

#undef LOCTEXT_NAMESPACE