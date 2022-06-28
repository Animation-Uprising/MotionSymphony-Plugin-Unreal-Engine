// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AssetTypeActions_MatchFeature.h"
#include "DataOriented/MatchFeature_BodyMomentum2D.h"
#include "DataOriented/MatchFeature_BodyMomentumRot.h"
#include "DataOriented/MatchFeature_BoneHeight.h"
#include "DataOriented/MatchFeature_BoneLocation.h"
#include "DataOriented/MatchFeature_BoneVelocity.h"
#include "DataOriented/MatchFeature_Trajectory2D.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

/***********************************************/
/** Begin Match Feature Bone Location Functions*/
/***********************************************/
FText FAssetTypeActions_MatchFeatureBoneLocation::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MatchFeatureBoneLocation", "Match Feature - Bone Location");
}

FColor FAssetTypeActions_MatchFeatureBoneLocation::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FAssetTypeActions_MatchFeatureBoneLocation::GetSupportedClass() const
{
	return UMatchFeature_BoneLocation::StaticClass();
}

uint32 FAssetTypeActions_MatchFeatureBoneLocation::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

bool FAssetTypeActions_MatchFeatureBoneLocation::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MatchFeatureBoneLocation::CanFilter()
{
	return true;
}

/***********************************************/
/** Begin Match Feature Bone Velocity Functions*/
/***********************************************/
FText FAssetTypeActions_MatchFeatureBoneVelocity::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MatchFeatureBoneVelocity",
		"Match Feature - Bone Velocity");
}

FColor FAssetTypeActions_MatchFeatureBoneVelocity::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FAssetTypeActions_MatchFeatureBoneVelocity::GetSupportedClass() const
{
	return UMatchFeature_BoneVelocity::StaticClass();
}

uint32 FAssetTypeActions_MatchFeatureBoneVelocity::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

bool FAssetTypeActions_MatchFeatureBoneVelocity::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MatchFeatureBoneVelocity::CanFilter()
{
	return true;
}

/**********************************************/
/** Begin Match Feature Bone Height Functions */
/**********************************************/
FText FAssetTypeActions_MatchFeatureBoneHeight::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MatchFeatureBoneHeight",
		"Match Feature - Bone Height");
}

FColor FAssetTypeActions_MatchFeatureBoneHeight::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FAssetTypeActions_MatchFeatureBoneHeight::GetSupportedClass() const
{
	return UMatchFeature_BoneHeight::StaticClass();
}

uint32 FAssetTypeActions_MatchFeatureBoneHeight::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

bool FAssetTypeActions_MatchFeatureBoneHeight::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MatchFeatureBoneHeight::CanFilter()
{
	return true;
}

/***************************************************/
/** Begin Match Feature Body Momentum 2D Functions */
/***************************************************/
FText FAssetTypeActions_MatchFeatureBodyMomentum2D::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MatchFeatureBodyMomentum2D",
		"Match Feature - Body Momentum 2D");
}

FColor FAssetTypeActions_MatchFeatureBodyMomentum2D::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FAssetTypeActions_MatchFeatureBodyMomentum2D::GetSupportedClass() const
{
	return UMatchFeature_BodyMomentum2D::StaticClass();
}

uint32 FAssetTypeActions_MatchFeatureBodyMomentum2D::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

bool FAssetTypeActions_MatchFeatureBodyMomentum2D::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MatchFeatureBodyMomentum2D::CanFilter()
{
	return true;
}

/***************************************************/
/** Begin Match Feature Body Momentum Rot Functions */
/***************************************************/
FText FAssetTypeActions_MatchFeatureBodyMomentumRot::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MatchFeatureBodyMomentumRot",
		"Match Feature - Body Momentum Rotation");
}

FColor FAssetTypeActions_MatchFeatureBodyMomentumRot::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FAssetTypeActions_MatchFeatureBodyMomentumRot::GetSupportedClass() const
{
	return UMatchFeature_BodyMomentumRot::StaticClass();
}

uint32 FAssetTypeActions_MatchFeatureBodyMomentumRot::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

bool FAssetTypeActions_MatchFeatureBodyMomentumRot::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MatchFeatureBodyMomentumRot::CanFilter()
{
	return true;
}

/************************************************/
/** Begin Match Feature Trajectory 2D Functions */
/************************************************/
FText FAssetTypeActions_MatchFeatureTrajectory2D::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MatchFeatureTrajectory2D",
		"Match Feature - Trajectory 2D");
}

FColor FAssetTypeActions_MatchFeatureTrajectory2D::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FAssetTypeActions_MatchFeatureTrajectory2D::GetSupportedClass() const
{
	return UMatchFeature_Trajectory2D::StaticClass();
}

uint32 FAssetTypeActions_MatchFeatureTrajectory2D::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

bool FAssetTypeActions_MatchFeatureTrajectory2D::HasActions(const TArray<UObject*>& InObjects) const
{
	return false;
}

bool FAssetTypeActions_MatchFeatureTrajectory2D::CanFilter()
{
	return true;
}

#undef LOCTEXT_NAMESPACE