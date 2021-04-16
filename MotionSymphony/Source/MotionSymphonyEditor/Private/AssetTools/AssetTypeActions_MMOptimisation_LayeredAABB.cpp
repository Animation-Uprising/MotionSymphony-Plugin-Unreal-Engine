// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AssetTypeActions_MMOptimisation_LayeredAABB.h"
#include "CustomAssets/MMOptimisation_LayeredAABB.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_MMOptimisation_LayeredAABB::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MMOptimisation_LayeredAABB", "MMOpimisation LayeredAABB");
}

FColor FAssetTypeActions_MMOptimisation_LayeredAABB::GetTypeColor() const
{
	return FColor::Blue;
}

UClass* FAssetTypeActions_MMOptimisation_LayeredAABB::GetSupportedClass() const
{
	return UMMOptimisation_LayeredAABB::StaticClass();
}

uint32 FAssetTypeActions_MMOptimisation_LayeredAABB::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

void FAssetTypeActions_MMOptimisation_LayeredAABB::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{

}

bool FAssetTypeActions_MMOptimisation_LayeredAABB::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MMOptimisation_LayeredAABB::CanFilter()
{
	return true;
}

#undef LOCTEXT_NAMESPACE