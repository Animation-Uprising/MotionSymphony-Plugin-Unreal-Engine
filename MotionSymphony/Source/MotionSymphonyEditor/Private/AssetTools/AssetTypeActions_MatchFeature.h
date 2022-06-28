// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"

/** Bone Location */
class FAssetTypeActions_MatchFeatureBoneLocation
	: public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_MatchFeatureBoneLocation(){}

public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;

	virtual uint32 GetCategories() override;
	//virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual bool CanFilter() override;
};

/** Bone Velocity */
class FAssetTypeActions_MatchFeatureBoneVelocity
	: public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_MatchFeatureBoneVelocity(){}

public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;

	virtual uint32 GetCategories() override;
	//virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual bool CanFilter() override;
};

/** Bone Height */
class FAssetTypeActions_MatchFeatureBoneHeight
	: public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_MatchFeatureBoneHeight(){}

public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;

	virtual uint32 GetCategories() override;
	//virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual bool CanFilter() override;
};

/** Body Momentum 2D */
class FAssetTypeActions_MatchFeatureBodyMomentum2D
	: public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_MatchFeatureBodyMomentum2D(){}

public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;

	virtual uint32 GetCategories() override;
	//virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual bool CanFilter() override;
};

/** Body Momentum Rot */
class FAssetTypeActions_MatchFeatureBodyMomentumRot
	: public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_MatchFeatureBodyMomentumRot(){}

public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;

	virtual uint32 GetCategories() override;
	//virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual bool CanFilter() override;
};

/**  Trajectory 2D */
class FAssetTypeActions_MatchFeatureTrajectory2D
	: public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_MatchFeatureTrajectory2D(){}

public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;

	virtual uint32 GetCategories() override;
	//virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual bool CanFilter() override;
};