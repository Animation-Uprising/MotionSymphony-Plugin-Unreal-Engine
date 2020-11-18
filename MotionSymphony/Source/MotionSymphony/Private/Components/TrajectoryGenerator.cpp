// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "TrajectoryGenerator.h"
#include "MotionMatchingUtil/MotionMatchingUtils.h"
#include "Math/UnrealMathVectorConstants.h"

#define EPSILON 0.0001f

UTrajectoryGenerator::UTrajectoryGenerator()
	: MaxSpeed(4.5f), 
	  MoveResponse(15.0f), 
	  TurnResponse(15.0f), 
	  bResetDirectionOnIdle(true),
	  LastDesiredOrientation(0.0f)
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
			1.0f - FMath::Exp((-MoveResponse * DeltaTime) * Percentage));

		NewTrajPosition[i] = NewTrajPosition[i - 1] + AdjustedTrajDisplacement;

		//TrajRotations[i] = DesiredOrientation;

		TrajRotations[i] = FMath::RadiansToDegrees(FMotionMatchingUtils::LerpAngle(
			FMath::DegreesToRadians(TrajRotations[i]),
			FMath::DegreesToRadians(DesiredOrientation) ,
			1.0f - FMath::Exp((-TurnResponse * DeltaTime) * Percentage)));
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
	if(InputVector.SizeSquared() > 1.0f)
		InputVector.Normalize();

	OutVelocity = FVector(InputVector.X, InputVector.Y, 0.0f) * MaxSpeed;

	//Rotate it to the camera view projection relative.
}