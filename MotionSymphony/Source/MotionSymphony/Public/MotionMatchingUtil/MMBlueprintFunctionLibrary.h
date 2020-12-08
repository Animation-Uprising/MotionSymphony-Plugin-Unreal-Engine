// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MMBlueprintFunctionLibrary.generated.h"

class UCameraComponent;
/**
 * 
 */
UCLASS()
class MOTIONSYMPHONY_API UMMBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	UFUNCTION(BlueprintCallable, Category = "Motion Symphony")
	static FVector GetInputVectorRelativeToCamera(FVector InputVector, UCameraComponent* CameraComponent);
};
