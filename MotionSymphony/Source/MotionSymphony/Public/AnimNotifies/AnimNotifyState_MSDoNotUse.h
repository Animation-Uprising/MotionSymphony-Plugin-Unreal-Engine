// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MSDoNotUse.generated.h"

/**
 * 
 */
UCLASS()
class MOTIONSYMPHONY_API UAnimNotifyState_MSDoNotUse : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	FString GetNotifyName() const;
};
