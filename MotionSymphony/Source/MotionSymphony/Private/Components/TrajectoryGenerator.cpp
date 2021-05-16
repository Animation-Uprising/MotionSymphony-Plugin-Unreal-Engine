// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.


#include "TrajectoryGenerator.h"
#include "MotionMatchingUtil/MotionMatchingUtils.h"
#include "Math/UnrealMathVectorConstants.h"
#include "Camera/CameraComponent.h"
#include "Data/InputProfile.h"

#define EPSILON 0.0001f

UTrajectoryGenerator::UTrajectoryGenerator()
	: MaxSpeed(400.0f), 
	  MoveResponse(15.0f), 
	  TurnResponse(15.0f), 
	  bResetDirectionOnIdle(true),
	  LastDesiredOrientation(0.0f),
	  MoveResponse_Remapped(15.0f),
	  TurnResponse_Remapped(15.0f)
{
}


void UTrajectoryGenerator::UpdatePrediction(float DeltaTime)
{
	FVector DesiredLinearVelocity;
	CalculateDesiredLinearVelocity(DesiredLinearVelocity);

	FVector DesiredLinearDisplacement = DesiredLinearVelocity / SampleRate;

	float DesiredOrientation = 0.0f;
	if (DesiredLinearDisplacement.SizeSquared() > EPSILON)
	{
		DesiredOrientation = FMath::RadiansToDegrees(FMath::Atan2(
			DesiredLinearDisplacement.Y, DesiredLinearDisplacement.X));
	}
	else
	{
		if(bResetDirectionOnIdle)
		{
			DesiredOrientation = OwningActor->GetActorRotation().Euler().Z;
		}
		else
		{
			DesiredOrientation = LastDesiredOrientation;
		}
	}

	LastDesiredOrientation = DesiredOrientation;

	NewTrajPosition[0] = FVector::ZeroVector;
	TrajRotations[0] = 0.0f;

	int32 Iterations = TrajPositions.Num();
	float CumRotation = 0.0f;

	for (int32 i = 1; i < Iterations; ++i)
	{
		float Percentage = (float)i / (float)(Iterations - 1);
		FVector TrajDisplacement = TrajPositions[i] - TrajPositions[i-1];

		FVector AdjustedTrajDisplacement = FMath::Lerp(TrajDisplacement, DesiredLinearDisplacement,
			1.0f - FMath::Exp((-MoveResponse_Remapped * DeltaTime) * Percentage));

		NewTrajPosition[i] = NewTrajPosition[i - 1] + AdjustedTrajDisplacement;

		//TrajRotations[i] = DesiredOrientation;

		TrajRotations[i] = FMath::RadiansToDegrees(FMotionMatchingUtils::LerpAngle(
			FMath::DegreesToRadians(TrajRotations[i]),
			FMath::DegreesToRadians(DesiredOrientation) ,
			1.0f - FMath::Exp((-TurnResponse_Remapped * DeltaTime) * Percentage)));
	}

	for (int32 i = 0; i < Iterations; ++i)
	{
		TrajPositions[i] = NewTrajPosition[i];
	}
}

void UTrajectoryGenerator::Setup(TArray<float>& InTrajTimes)
{
	NewTrajPosition.Empty(TrajectoryIterations);

	FVector ActorPosition = OwningActor->GetActorLocation();
	for (int32 i = 0; i < TrajectoryIterations; ++i)
	{
		NewTrajPosition.Emplace(ActorPosition);
	}
}

void UTrajectoryGenerator::CalculateDesiredLinearVelocity(FVector & OutVelocity)
{
	MoveResponse_Remapped = MoveResponse;
	TurnResponse_Remapped = TurnResponse;

	if (InputProfile != nullptr)
	{
		const FInputSet* InputSet = InputProfile->GetInputSet(InputVector);

		if (InputSet != nullptr)
		{
			InputVector.Normalize();
			InputVector *= InputSet->SpeedMultiplier;

			MoveResponse_Remapped = MoveResponse * InputSet->MoveResponseMultiplier;
			TurnResponse_Remapped = TurnResponse * InputSet->TurnResponseMultiplier;
		}
	}

	if(InputVector.SizeSquared() > 1.0f)
		InputVector.Normalize();

	OutVelocity = FVector(InputVector.X, InputVector.Y, 0.0f) * MaxSpeed;
}