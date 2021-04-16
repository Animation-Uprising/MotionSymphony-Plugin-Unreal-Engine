// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MMOptimisation_TraitBinsAssetFactory.h"
#include "CustomAssets/MMOptimisation_TraitBins.h"

UMMOptimisation_TraitBinsFactory::UMMOptimisation_TraitBinsFactory(const FObjectInitializer& ObjectInitializer)
{
	SupportedClass = UMMOptimisation_TraitBins::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMMOptimisation_TraitBinsFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return NewObject<UMMOptimisation_TraitBins>(InParent, InClass, InName, Flags);
}

bool UMMOptimisation_TraitBinsFactory::ShouldShowInNewMenu() const
{
	return true;
}
