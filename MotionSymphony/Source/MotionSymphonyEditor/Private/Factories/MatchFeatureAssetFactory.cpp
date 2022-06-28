// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MatchFeatureAssetFactory.h"
#include "DataOriented/MatchFeature_BodyMomentum2D.h"
#include "DataOriented/MatchFeature_BodyMomentumRot.h"
#include "DataOriented/MatchFeature_BoneHeight.h"
#include "DataOriented/MatchFeature_BoneLocation.h"
#include "DataOriented/MatchFeature_BoneVelocity.h"
#include "DataOriented/MatchFeature_Trajectory2D.h"

#define LOCTEXT_NAMESPACE "MatchFeatureFactory"

/***********************************************/
/** Begin Match Feature Bone Location Functions*/
/***********************************************/
UMatchFeatureBoneLocationFactory::UMatchFeatureBoneLocationFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMatchFeature_BoneLocation::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMatchFeatureBoneLocationFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UMatchFeature_BoneLocation* NewMMFeature = NewObject<UMatchFeature_BoneLocation>(InParent, InClass, InName, Flags);

	return NewMMFeature;
}

bool UMatchFeatureBoneLocationFactory::ShouldShowInNewMenu() const
{
	return true;
}
/***********************************************/
/** Begin Match Feature Bone Velocity Functions*/
/***********************************************/
UMatchFeatureBoneVelocityFactory::UMatchFeatureBoneVelocityFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMatchFeature_BoneVelocity::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMatchFeatureBoneVelocityFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UMatchFeature_BoneVelocity* NewMMFeature = NewObject<UMatchFeature_BoneVelocity>(InParent, InClass, InName, Flags);

	return NewMMFeature;
}

bool UMatchFeatureBoneVelocityFactory::ShouldShowInNewMenu() const
{
	return true;
}
/***********************************************/
/** Begin Match Feature Bone Height Functions  */
/***********************************************/
UMatchFeatureBoneHeightFactory::UMatchFeatureBoneHeightFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMatchFeature_BoneHeight::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMatchFeatureBoneHeightFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UMatchFeature_BoneHeight* NewMMFeature = NewObject<UMatchFeature_BoneHeight>(InParent, InClass, InName, Flags);
	
	return NewMMFeature;
}

bool UMatchFeatureBoneHeightFactory::ShouldShowInNewMenu() const
{
	return true;
}

/***************************************************/
/** Begin Match Feature Body Momentum 2D Functions */
/***************************************************/
UMatchFeatureBodyMomentum2DFactory::UMatchFeatureBodyMomentum2DFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMatchFeature_BodyMomentum2D::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMatchFeatureBodyMomentum2DFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UMatchFeature_BodyMomentum2D* NewMMFeature = NewObject<UMatchFeature_BodyMomentum2D>(InParent, InClass, InName, Flags);

	return NewMMFeature;
}

bool UMatchFeatureBodyMomentum2DFactory::ShouldShowInNewMenu() const
{
	return true;
}

/****************************************************/
/** Begin Match Feature Body Momentum Rot Functions */
/****************************************************/
UMatchFeatureBodyMomentumRotFactory::UMatchFeatureBodyMomentumRotFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMatchFeature_BodyMomentumRot::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMatchFeatureBodyMomentumRotFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UMatchFeature_BodyMomentumRot* NewMMFeature = NewObject<UMatchFeature_BodyMomentumRot>(InParent, InClass, InName, Flags);

	return NewMMFeature;
}

bool UMatchFeatureBodyMomentumRotFactory::ShouldShowInNewMenu() const
{
	return true;
}

/************************************************/
/** Begin Match Feature Trajectory 2D Functions */
/************************************************/
UMatchFeatureTrajectory2DFactory::UMatchFeatureTrajectory2DFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMatchFeature_Trajectory2D::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMatchFeatureTrajectory2DFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UMatchFeature_Trajectory2D* NewMMFeature = NewObject<UMatchFeature_Trajectory2D>(InParent, InClass, InName, Flags);
	
	return NewMMFeature;
}

bool UMatchFeatureTrajectory2DFactory::ShouldShowInNewMenu() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
