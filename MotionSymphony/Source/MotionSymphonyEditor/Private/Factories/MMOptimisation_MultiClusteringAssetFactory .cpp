// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MMOptimisation_MultiClusteringAssetFactory.h"
#include "CustomAssets/MMOptimisation_MultiClustering.h"

UMMOptimisation_MultiClusteringFactory::UMMOptimisation_MultiClusteringFactory(const FObjectInitializer& ObjectInitializer)
{
	SupportedClass = UMMOptimisation_MultiClustering::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMMOptimisation_MultiClusteringFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return NewObject<UMMOptimisation_MultiClustering>(InParent, InClass, InName, Flags);
}

bool UMMOptimisation_MultiClusteringFactory::ShouldShowInNewMenu() const
{
	return true;
}
