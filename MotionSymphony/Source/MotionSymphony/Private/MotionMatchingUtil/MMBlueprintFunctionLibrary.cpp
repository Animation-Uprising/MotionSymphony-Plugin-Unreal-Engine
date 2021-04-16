// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.


#include "MMBlueprintFunctionLibrary.h"
#include "MotionSymphonySettings.h"
#include "Camera/CameraComponent.h"

FVector UMMBlueprintFunctionLibrary::GetInputVectorRelativeToCamera(FVector InputVector, UCameraComponent* CameraComponent)
{
	FRotator CameraProjectedRotation = CameraComponent->GetComponentToWorld().GetRotation().Rotator();
	CameraProjectedRotation.Roll = 0.0f;
	CameraProjectedRotation.Pitch = 0.0f;

	return CameraProjectedRotation.RotateVector(InputVector);
}

FMotionTraitField UMMBlueprintFunctionLibrary::CreateMotionTraitField(const FString TraitName)
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		return FMotionTraitField();
	}

	int32 TraitIndex = Settings->TraitNames.IndexOfByKey(TraitName);

	if (TraitIndex > 63)
	{
		//Trait index out of range
		return FMotionTraitField();
	}

	return FMotionTraitField(TraitIndex);
}

FMotionTraitField UMMBlueprintFunctionLibrary::CreateMotionTraitFieldFromArray(const TArray<FString>& TraitNames)
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		return FMotionTraitField();
	}

	FMotionTraitField MotionTraits = FMotionTraitField();
	for (const FString& TraitName : TraitNames)
	{
		MotionTraits.SetTraitPosition(Settings->TraitNames.IndexOfByKey(TraitName));
	}

	return MotionTraits;
}

void UMMBlueprintFunctionLibrary::AddTrait(const FString TraitName, FMotionTraitField& OutTraitField)
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		return;
	}

	int32 TraitIndex = Settings->TraitNames.IndexOfByKey(TraitName);

	if (TraitIndex > 63)
	{
		//Trait index out of range
		return;
	}

	OutTraitField.SetTraitPosition(TraitIndex);
}

void UMMBlueprintFunctionLibrary::AddTraits(const TArray<FString>& TraitNames,FMotionTraitField& OutTraitField)
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		return;
	}

	for (const FString& TraitName : TraitNames)
	{
		int32 TraitIndex = Settings->TraitNames.IndexOfByKey(TraitName);

		if (TraitIndex > -1 && TraitIndex < 64)
		{
			OutTraitField.SetTraitPosition(TraitIndex);
		}
	}
}

void UMMBlueprintFunctionLibrary::AddTraitField(const FMotionTraitField NewTraits, FMotionTraitField& OutTraitField)
{
	OutTraitField |= NewTraits;
}

void UMMBlueprintFunctionLibrary::RemoveTrait(const FString TraitName, FMotionTraitField& OutTraitField)
{	
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		return;
	}

	int32 TraitIndex = Settings->TraitNames.IndexOfByKey(TraitName);

	if (TraitIndex > 63)
	{
		return;
	}

	OutTraitField.UnSetTraitPosition(TraitIndex);
}

void UMMBlueprintFunctionLibrary::RemoveTraits(const TArray<FString>& TraitNames, FMotionTraitField& OutTraitField)
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		return;
	}

	FMotionTraitField TraitsToRemove = FMotionTraitField();
	for (const FString& TraitName : TraitNames)
	{
		int32 TraitIndex = Settings->TraitNames.IndexOfByKey(TraitName);

		if (TraitIndex > -1 && TraitIndex < 64)
		{
			OutTraitField.UnSetTraitPosition(TraitIndex);
		}
	}
}

void UMMBlueprintFunctionLibrary::RemoveTraitField(const FMotionTraitField TraitsToRemove, FMotionTraitField& OutTraitField)
{
	OutTraitField.UnSetTraits(TraitsToRemove);
}

FMotionTraitField UMMBlueprintFunctionLibrary::GetTraitHandle(const FString TraitName)
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		return FMotionTraitField();
	}

	int32 TraitIndex = Settings->TraitNames.IndexOfByKey(TraitName);

	if (TraitIndex > 63)
	{
		return FMotionTraitField();
	}

	return FMotionTraitField(TraitIndex);
}

FMotionTraitField UMMBlueprintFunctionLibrary::GetTraitHandleFromArray(const TArray<FString>& TraitNames)
{
	UMotionSymphonySettings* Settings = GetMutableDefault<UMotionSymphonySettings>();

	if (!Settings)
	{
		return FMotionTraitField();
	}

	FMotionTraitField MotionTraits = FMotionTraitField();
	for (const FString& TraitName : TraitNames)
	{
		int32 TraitIndex = Settings->TraitNames.IndexOfByKey(TraitName);

		if (TraitIndex > -1 && TraitIndex < 64)
		{
			MotionTraits.SetTraitPosition(TraitIndex);
		}
	}

	return MotionTraits;
}