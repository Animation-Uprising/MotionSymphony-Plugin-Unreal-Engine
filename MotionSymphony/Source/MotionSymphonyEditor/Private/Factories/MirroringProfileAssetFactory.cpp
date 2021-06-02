// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MirroringProfileAssetFactory.h"

UMirroringProfileFactory::UMirroringProfileFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMirroringProfile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMirroringProfileFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UMirroringProfile* NewMirroringProfile = NewObject<UMirroringProfile>(InParent, InClass, InName, Flags);

	return NewMirroringProfile; 
}

bool UMirroringProfileFactory::ShouldShowInNewMenu() const
{
	return true;
}