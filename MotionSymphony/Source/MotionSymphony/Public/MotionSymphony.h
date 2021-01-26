// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMotionSymphonyModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

UENUM(BlueprintType)
enum class EMotionAnimAssetType : uint8
{
	None,
	Sequence,
	BlendSpace
};