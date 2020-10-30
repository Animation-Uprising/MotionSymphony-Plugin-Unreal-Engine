// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "MotionMatchConfigAssetFactory.h"

UMotionMatchConfigFactory::UMotionMatchConfigFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMotionMatchConfig::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMotionMatchConfigFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return NewObject<UMotionMatchConfig>(InParent, InClass, InName, Flags);
}

bool UMotionMatchConfigFactory::ShouldShowInNewMenu() const
{
	return true;
}