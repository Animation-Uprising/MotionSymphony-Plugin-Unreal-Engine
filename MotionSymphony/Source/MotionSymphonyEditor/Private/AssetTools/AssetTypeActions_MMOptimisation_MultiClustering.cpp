// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AssetTypeActions_MMOptimisation_MultiClustering.h"
#include "CustomAssets/MMOptimisation_MultiClustering.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_MMOptimisation_MultiClustering::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MMOptimisation_MultiClustering", "MMOpimisation MultiClustering");
}

FColor FAssetTypeActions_MMOptimisation_MultiClustering::GetTypeColor() const
{
	return FColor::Blue;
}

UClass* FAssetTypeActions_MMOptimisation_MultiClustering::GetSupportedClass() const
{
	return UMMOptimisation_MultiClustering::StaticClass();
}

uint32 FAssetTypeActions_MMOptimisation_MultiClustering::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

void FAssetTypeActions_MMOptimisation_MultiClustering::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{

}

bool FAssetTypeActions_MMOptimisation_MultiClustering::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MMOptimisation_MultiClustering::CanFilter()
{
	return true;
}

#undef LOCTEXT_NAMESPACE