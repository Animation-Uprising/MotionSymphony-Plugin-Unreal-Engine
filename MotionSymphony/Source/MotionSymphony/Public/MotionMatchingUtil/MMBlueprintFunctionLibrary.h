// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/MotionTraitField.h"
#include "MMBlueprintFunctionLibrary.generated.h"

class UCameraComponent;
/**
 * 
 */
UCLASS()
class MOTIONSYMPHONY_API UMMBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:	

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony")
	static FVector GetInputVectorRelativeToCamera(FVector InputVector, UCameraComponent* CameraComponent);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony")
	static FMotionTraitField CreateMotionTraitField(const FString TraitName);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony")
	static FMotionTraitField CreateMotionTraitFieldFromArray(UPARAM(ref) const TArray<FString>& TraitNames);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony|Traits")
	static void AddTrait(const FString TraitName, UPARAM(ref) FMotionTraitField& OutTraitField);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony|Traits")
	static void AddTraits(UPARAM(ref) const TArray<FString>& TraitNames, UPARAM(ref) FMotionTraitField& OutTraitField);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony|Traits")
	static void AddTraitField(const FMotionTraitField NewTraits, UPARAM(ref) FMotionTraitField& OutTraitField);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony|Traits")
	static void RemoveTrait(const FString TraitName, UPARAM(ref) FMotionTraitField& OutTraitField);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony|Traits")
	static void RemoveTraits(UPARAM(ref) const TArray<FString>& TraitNames, UPARAM(ref) FMotionTraitField& OutTraitField);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony|Traits")
	static void RemoveTraitField(const FMotionTraitField TraitsToRemove, UPARAM(ref) FMotionTraitField& OutTraitField);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony|Traits")
	static FMotionTraitField GetTraitHandle(const FString TraitName);

	UFUNCTION(BlueprintCallable, Category = "Motion Symphony|Traits")
	static FMotionTraitField GetTraitHandleFromArray(UPARAM(ref) const TArray<FString>& TraitNames);
};
