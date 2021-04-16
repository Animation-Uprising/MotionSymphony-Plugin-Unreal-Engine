// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MMOptimisation_LayeredAABBAssetFactory.h"
#include "CustomAssets/MMOptimisation_LayeredAABB.h"

UMMOptimisation_LayeredAABBFactory::UMMOptimisation_LayeredAABBFactory(const FObjectInitializer& ObjectInitializer)
{
	SupportedClass = UMMOptimisation_LayeredAABB::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true; 
}

UObject* UMMOptimisation_LayeredAABBFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return NewObject<UMMOptimisation_LayeredAABB>(InParent, InClass, InName, Flags);
}

bool UMMOptimisation_LayeredAABBFactory::ShouldShowInNewMenu() const
{
	return true;
}
