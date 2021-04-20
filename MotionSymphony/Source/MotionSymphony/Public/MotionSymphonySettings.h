// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MotionSymphonySettings.generated.h"

UCLASS(config = Game, defaultconfig)
class MOTIONSYMPHONY_API UMotionSymphonySettings : public UObject
{
	GENERATED_BODY()

public:
	UMotionSymphonySettings(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, config, Category = "Traits")
	TArray<FString> TraitNames;

	//UPROPERTY(EditAnywhere, config, Category = "Actions")
	TArray<FString> ActionNames;
};

