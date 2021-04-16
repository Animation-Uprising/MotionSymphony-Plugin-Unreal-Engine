// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputProfile.generated.h"


USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FInputSet
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D InputRemapRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveResponseMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TurnResponseMultiplier;

public:
	FInputSet();
};

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FInputProfile
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Profile")
	TArray<FInputSet> InputSets;

public:
	FInputProfile();

	const FInputSet* GetInputSet(FVector2D Input);
};