// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MatchFeatureAssetFactory.generated.h"


UCLASS(hidecategories=Object)
class UMatchFeatureBoneLocationFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, 
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual bool ShouldShowInNewMenu() const override;
};

UCLASS(hidecategories=Object)
class UMatchFeatureBoneVelocityFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, 
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual bool ShouldShowInNewMenu() const override;
};


UCLASS(hidecategories=Object)
class UMatchFeatureBoneHeightFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, 
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual bool ShouldShowInNewMenu() const override;
};

UCLASS(hidecategories=Object)
class UMatchFeatureBodyMomentum2DFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, 
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual bool ShouldShowInNewMenu() const override;
};

UCLASS(hidecategories=Object)
class UMatchFeatureBodyMomentumRotFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, 
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual bool ShouldShowInNewMenu() const override;
};


UCLASS(hidecategories=Object)
class UMatchFeatureTrajectory2DFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, 
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual bool ShouldShowInNewMenu() const override;
};
