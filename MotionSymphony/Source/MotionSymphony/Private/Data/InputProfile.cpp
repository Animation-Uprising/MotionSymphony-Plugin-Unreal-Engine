// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "Data/InputProfile.h"

FInputSet::FInputSet()
{
}

FInputProfile::FInputProfile()
{
}

const FInputSet* FInputProfile::GetInputSet(FVector2D Input)
{
	float InputSqrMagnitude = Input.SizeSquared();

	for (FInputSet& InputSet : InputSets)
	{
		if (InputSqrMagnitude >= (InputSet.InputRemapRange.X * InputSet.InputRemapRange.X)
			&& InputSqrMagnitude < (InputSet.InputRemapRange.Y * InputSet.InputRemapRange.Y))
		{
			return &InputSet;
		}
	}

	return nullptr;
}