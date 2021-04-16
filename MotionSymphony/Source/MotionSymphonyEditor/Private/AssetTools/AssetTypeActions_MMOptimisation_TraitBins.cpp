// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AssetTypeActions_MMOptimisation_TraitBins.h"
#include "CustomAssets/MMOptimisation_TraitBins.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_MMOptimisation_TraitBins::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MMOptimisation_TraitBins", "MMOpimisation TraitBins");
}

FColor FAssetTypeActions_MMOptimisation_TraitBins::GetTypeColor() const
{
	return FColor::Blue;
}

UClass* FAssetTypeActions_MMOptimisation_TraitBins::GetSupportedClass() const
{
	return UMMOptimisation_TraitBins::StaticClass();
}

uint32 FAssetTypeActions_MMOptimisation_TraitBins::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

void FAssetTypeActions_MMOptimisation_TraitBins::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{

}

bool FAssetTypeActions_MMOptimisation_TraitBins::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MMOptimisation_TraitBins::CanFilter()
{
	return true;
}

#undef LOCTEXT_NAMESPACE