// Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

#include "TrajectoryGenerator.h"
#include "Camera/CameraComponent.h"
#include "Data/InputProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utility/MotionMatchingUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"

#define EPSILON 0.0001f

UTrajectoryGenerator::UTrajectoryGenerator()
	: MaxSpeed(400.0f), 
	  MoveResponse(15.0f), 
	  TurnResponse(15.0f), 
	  StrafeDirection(FVector(0.0f)),
	  bResetDirectionOnIdle(true),
	  TrajectoryBehaviour(ETrajectoryMoveMode::Standard),
	  TrajectoryControlMode(ETrajectoryControlMode::PlayerControlled),
	  LastDesiredOrientation(0.0f),
      MoveResponse_Remapped(15.0f),
	  TurnResponse_Remapped(15.0f)
{
}

void UTrajectoryGenerator::UpdatePrediction(float DeltaTime)
{
	if(TrajectoryControlMode == ETrajectoryControlMode::AIControlled
		&& !bUsePathAsTrajectoryForAI)
	{
		CalculateInputVectorFromAINavAgent();
	}
	
	FVector DesiredLinearVelocity;
	CalculateDesiredLinearVelocity(DesiredLinearVelocity);
	const FVector DesiredLinearDisplacement = DesiredLinearVelocity / FMath::Max(EPSILON, SampleRate);
	
	if(TrajectoryControlMode == ETrajectoryControlMode::AIControlled
		&& bUsePathAsTrajectoryForAI)
	{
		PathFollowPrediction(DeltaTime, TrajectoryIterations, DesiredLinearDisplacement);
	}
	else
	{
		InputPrediction(DeltaTime, DesiredLinearDisplacement);
		//CapsulePrediction(DeltaTime);
	}
}

void UTrajectoryGenerator::PathFollowPrediction(const float DeltaTime, const int32 Iterations, const FVector& DesiredLinearDisplacement)
{
	const FVector RefLocation = OwningActor->GetActorLocation();
	auto DebugDraw = [&]()
	{
		for(int32 i = 0; i < Iterations; ++i)
		{
			DrawDebugCoordinateSystem(GetWorld(), RefLocation + TrajPositions[i],
				FRotator(0.0f, TrajRotations[i], 0.0f), 10, false, DeltaTime * 1.2f);		
		}
	};
	
	if(APawn* Pawn = Cast<APawn>(OwningActor))
	{
		if(AAIController* Controller = Pawn->GetController<AAIController>())
		{
			if(UPathFollowingComponent* PathComponent = Controller->GetPathFollowingComponent())
			{
				if(const FNavigationPath* PathPtr = PathComponent->GetPath().Get())
				{
					const TArray<FNavPathPoint>& Points = PathPtr->GetPathPoints();
					
					if(Points.Num() > 0)
					{
						const float Step = DesiredLinearDisplacement.Size();
						float Overflow = Step;

						const int32 PathStartIndex = PathComponent->GetCurrentPathIndex() + 1;
						int32 TrajectoryPointIndex = 1;
						bool bTrajectoryEnded = Step <= 0;
						for(int32 PathPointIndex = PathStartIndex; !bTrajectoryEnded && PathPointIndex < Points.Num(); ++PathPointIndex)
						{
							FVector StartLocation = Points[PathPointIndex - 1].Location;
							FVector EndLocation = Points[PathPointIndex].Location;
							if(PathPointIndex == PathStartIndex)
							{
								StartLocation = FMath::ClosestPointOnLine(StartLocation, EndLocation, RefLocation);
							}

							const FVector Segment = EndLocation - StartLocation;
							const FVector SegmentDir = Segment.GetSafeNormal();
							const float Yaw = SegmentDir.Rotation().Yaw;
							const float SegmentLength = Segment.Size();

							float Length = Overflow;
							Overflow = 0.0f;
							FVector Ref = StartLocation - RefLocation;
							Ref.Z = 0.0f;
							while(!bTrajectoryEnded&& Length < SegmentLength)
							{
								TrajPositions[TrajectoryPointIndex] = Ref + SegmentDir * Length;
								TrajPositions[TrajectoryPointIndex].Z = 0;
								TrajRotations[TrajectoryPointIndex] = Yaw;
								Length += Step;

								bTrajectoryEnded = ++TrajectoryPointIndex == Iterations;
							}
							Overflow = Length - SegmentLength;
						}
						if(!bTrajectoryEnded)
						{
							while(TrajectoryPointIndex < Iterations)
							{
								TrajPositions[TrajectoryPointIndex] = TrajPositions[TrajectoryPointIndex - 1];
								TrajRotations[TrajectoryPointIndex] = TrajRotations[TrajectoryPointIndex - 1];
								++TrajectoryPointIndex;
							}
						}

						if(bDrawTrajectory)
						{
							DebugDraw();
						}

						TrajRotations[0] = OwningActor->GetActorRotation().Yaw;
					}
				}
			}
		}
	}
}

void UTrajectoryGenerator::InputPrediction(const float DeltaTime, const FVector& DesiredLinearDisplacement)
{
	float DesiredOrientation = 0.0f;
	if (TrajectoryBehaviour != ETrajectoryMoveMode::Standard)
	{
		DesiredOrientation = FMath::RadiansToDegrees(FMath::Atan2(StrafeDirection.Y, StrafeDirection.X));
	}
	else if (DesiredLinearDisplacement.SizeSquared() > EPSILON)
	{
		DesiredOrientation = FMath::RadiansToDegrees(FMath::Atan2(
			DesiredLinearDisplacement.Y, DesiredLinearDisplacement.X));
	}
	else
	{
		if(bResetDirectionOnIdle)
		{
			const USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(
			OwningActor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
			
			DesiredOrientation = SkelMesh->GetComponentToWorld().Rotator().Yaw + CharacterFacingOffset;
		}
		else
		{
			DesiredOrientation = LastDesiredOrientation;
		}
	}

	LastDesiredOrientation = DesiredOrientation;

	NewTrajPosition[0] = FVector::ZeroVector;
	TrajRotations[0] = 0.0f;

	const int32 Iterations = TrajPositions.Num();
	float CumRotation = 0.0f;
	
	for (int32 i = 1; i < Iterations; ++i)
	{
		const float Percentage = (float)i / FMath::Max(1.0f, (float)(Iterations - 1));
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

void UTrajectoryGenerator::CapsulePrediction(const float DeltaTime)
{
	const FVector CurrentLocation = OwningActor->GetActorLocation();
	const FVector Velocity = CharacterMovement->Velocity;
	const FVector Acceleration = CharacterMovement->GetCurrentAcceleration();
	const bool bZeroAcceleration = Acceleration.IsZero();
	float Friction = CharacterMovement->GroundFriction;
	float BrakingDeceleration = CharacterMovement->BrakingDecelerationWalking;
	
	if(!HasMoveInput()) //Todo: This needs to be a check if braking friction is required.
	{
		Friction *= CharacterMovement->BrakingFriction;
	}

	Friction = FMath::Max(Friction, 0.0f);
	const bool bZeroFriction = (Friction < 0.00001f);
	const bool bZeroBraking = (BrakingDeceleration == 0.00001f);
	
	const float MIN_TICK_TIME = 1e-6;

	if(bZeroAcceleration
		&& bZeroFriction
		&& bZeroBraking)
	{
		return;
	}
	
	FVector LastVelocity = bZeroAcceleration ? Velocity : Velocity.ProjectOnToNormal(Acceleration.GetSafeNormal());
	LastVelocity.Z = 0.0f;

	FVector LastLocation = CurrentLocation;

	TrajPositions[0] = FVector::ZeroVector;
	
	for(int32 TrajectoryIndex = 1; TrajectoryIndex < TrajectoryIterations; ++TrajectoryIndex)
	{
		const FVector OldVel = LastVelocity;

		if(bZeroAcceleration)
		{
			// subdivide braking to get reasonably consistent results at lower frame rates
			// (important for packet loss situations w/ networking)
			float RemainingTime = SampleRate;
			const float MaxDeltaTime = 1.0f / 33.0f;

			// Decelerate to brake to a stop
			const FVector BrakeDeceleration (bZeroBraking ? FVector::ZeroVector : (-BrakingDeceleration * LastVelocity.GetSafeNormal()));
			while(RemainingTime >= MIN_TICK_TIME)
			{
				const float DT = ((RemainingTime > MaxDeltaTime && !bZeroFriction) ? FMath::Min(MaxDeltaTime, RemainingTime * 0.5f) : RemainingTime);
				RemainingTime -= DT;

				// apply friction and braking
				LastVelocity = LastVelocity + ((-Friction) * LastVelocity + BrakeDeceleration) * DT;

				//Don't reverse Direction
				// if(LastVelocity | OldVel <= 0.0f)
				// {
				// 	LastVelocity = FVector::ZeroVector;
				// 	break;
				// }
			}

			// Clamp to zero if nearly zero, or if below min threshold and braking.
			const float VSizeSq = LastVelocity.SizeSquared();
			if(VSizeSq <= 1.0f || (!bZeroBraking && VSizeSq <= FMath::Square(10)))
			{
				 LastVelocity = FVector::ZeroVector;
			}
		}
		else
		{
			FVector TotalAcceleration = Acceleration;
			TotalAcceleration.Z = 0;

			// Friction affects our ability to change direction. This is only done for input acceleration, not path following.
			const FVector AccelDir = TotalAcceleration.GetSafeNormal();
			const float VelSize = LastVelocity.Size();
			TotalAcceleration += -(LastVelocity - AccelDir * VelSize) * Friction;
			// Apply acceleration
			LastVelocity += TotalAcceleration * DeltaTime;
		}

		LastLocation += LastVelocity * SampleRate;

		TrajPositions[TrajectoryIndex] = LastLocation - OwningActor->GetTransform().GetLocation();
	}

}

void UTrajectoryGenerator::Setup(TArray<float>& InTrajTimes)
{
	CharacterMovement = Cast<UCharacterMovementComponent>(
		OwningActor->GetComponentByClass(UCharacterMovementComponent::StaticClass()));

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
	{
		InputVector.Normalize();
	}

	OutVelocity = InputVector * MaxSpeed;
}

void UTrajectoryGenerator::CalculateInputVectorFromAINavAgent()
{
	if(!CharacterMovement)
	{
		TrajectoryControlMode = ETrajectoryControlMode::PlayerControlled;
		return;
	}
	
	if(CharacterMovement->UseAccelerationForPathFollowing())
	{
		InputVector = CharacterMovement->GetCurrentAcceleration();
	}
	else
	{
		InputVector = CharacterMovement->RequestedVelocity;
	}
		
	InputVector.Normalize();
}

void UTrajectoryGenerator::SetStrafeDirectionFromCamera(UCameraComponent* Camera)
{
	if (!Camera)
	{
		return;
	}

	StrafeDirection = Camera->GetForwardVector();
	StrafeDirection.Z = 0.0f;

	StrafeDirection.Normalize();
}
