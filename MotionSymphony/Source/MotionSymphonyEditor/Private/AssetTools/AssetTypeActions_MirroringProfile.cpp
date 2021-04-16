// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AssetTypeActions_MirroringProfile.h"
#include "CustomAssets/MirroringProfile.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_MirroringProfile::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MirroringProfile", "Mirroring Profile");
}

FColor FAssetTypeActions_MirroringProfile::GetTypeColor() const
{
	return FColor::Blue;
}

UClass * FAssetTypeActions_MirroringProfile::GetSupportedClass() const
{
	return UMirroringProfile::StaticClass();
}

uint32 FAssetTypeActions_MirroringProfile::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

void FAssetTypeActions_MirroringProfile::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder & MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto MirroringProfiles = GetTypedWeakObjectPtrs<UMirroringProfile>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MirroringProfile_AutoMap", "Auto Map Bones"),
		LOCTEXT("MirroringProfile_AutoMapTooltip", "Attempt to automatically map bone mirroring using left or right prefixes and suffixes"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=] 
			{
				for (auto MirrorProfilePtr : MirroringProfiles)
				{
					MirrorProfilePtr.Get()->AutoMap();
				}
			}),
			FCanExecuteAction::CreateLambda([=] 
			{
				for (auto MirrorProfilePtr : MirroringProfiles)
				{
					if (MirrorProfilePtr.Get()->SourceSkeleton == nullptr)
					{
						return false;
					}
				}

				return true;
			})
		)
	);
}

bool FAssetTypeActions_MirroringProfile::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

bool FAssetTypeActions_MirroringProfile::CanFilter()
{
	return true;
}

#undef LOCTEXT_NAMESPACE