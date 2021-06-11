// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Components/DistanceMatching.h"
#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "MotionSymphony"

static TAutoConsoleVariable<int32> CVarDistanceMatchingDebug(
	TEXT("a.AnimNode.MoSymph.DistanceMatch.Debug"),
	0,
	TEXT("Enables Debug Mode for Distance Matching. \n")
	TEXT("<=0: Off \n")
	TEXT("  1: On"));

UDistanceMatching::UDistanceMatching()
	: bAutomaticTriggers(false),
	DistanceTolerance(5.0f),
	MinPlantDetectionAngle(130.0f),
	MinPlantSpeed(100.0f),
	MinPlantAccel(100.0f),
	TriggeredTransition(EDistanceMatchTrigger::None),
	CurrentInstanceId(0),
	bDestinationReached(false),
	LastFrameAccelSqr(0.0f),
	DistanceToMarker(0.0f),
	TimeToMarker(0.0f),
	MarkerVector(FVector::ZeroVector),
	DistanceMatchType(EDistanceMatchType::None),
	DistanceMatchBasis(EDistanceMatchBasis::Positional),
	ParentActor(nullptr),
	MovementComponent(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDistanceMatching::TriggerStart(float DeltaTime)
{
	DistanceMatchType = EDistanceMatchType::Backward;
	DistanceMatchBasis = EDistanceMatchBasis::Positional;
	TriggeredTransition = EDistanceMatchTrigger::Start;
	MarkerVector = ParentActor->GetActorLocation();
	
	
	if ((++CurrentInstanceId) > 1000000)
	{
		CurrentInstanceId = 0;
	}
}

void UDistanceMatching::TriggerStop(float DeltaTime)
{
	if(CalculateStopLocation(MarkerVector, DeltaTime, 50))
	{
		DistanceMatchType = EDistanceMatchType::Forward;
		DistanceMatchBasis = EDistanceMatchBasis::Positional;
		TriggeredTransition = EDistanceMatchTrigger::Stop;
		bDestinationReached = false;
	
		if ((++CurrentInstanceId) > 1000000)
		{
			CurrentInstanceId = 0;
		}
	}
}

void UDistanceMatching::TriggerPlant(float DeltaTime)
{
	if (CalculateStopLocation(MarkerVector, DeltaTime, 50))
	{
		DistanceMatchType = EDistanceMatchType::Both;
		DistanceMatchBasis = EDistanceMatchBasis::Positional;
		TriggeredTransition = EDistanceMatchTrigger::Plant;
		bDestinationReached = false;

		if ((++CurrentInstanceId) > 1000000)
		{
			CurrentInstanceId = 0;
		}
	}
}

void UDistanceMatching::TriggerPivotFrom()
{
	DistanceMatchType = EDistanceMatchType::Backward;
	DistanceMatchBasis = EDistanceMatchBasis::Rotational;
	TriggeredTransition = EDistanceMatchTrigger::Pivot;

	MarkerVector = ParentActor->GetActorForwardVector();

	if ((++CurrentInstanceId) > 1000000)
	{
		CurrentInstanceId = 0;
	}
}

void UDistanceMatching::TriggerPivotTo()
{
	FVector Accel = MovementComponent->GetCurrentAcceleration();
	Accel.Z = 0.0f;
	if(Accel.SizeSquared() > 0.0001f)
	{
		MarkerVector = Accel.GetSafeNormal();

		DistanceMatchType = EDistanceMatchType::Forward;
		DistanceMatchBasis = EDistanceMatchBasis::Rotational;
		TriggeredTransition = EDistanceMatchTrigger::Pivot;
		bDestinationReached = false;

		if ((++CurrentInstanceId) > 1000000)
		{
			CurrentInstanceId = 0;
		}
	}
}

void UDistanceMatching::TriggerTurnInPlaceFrom()
{
	MarkerVector = ParentActor->GetActorForwardVector();

	DistanceMatchType = EDistanceMatchType::Backward;
	DistanceMatchBasis = EDistanceMatchBasis::Rotational;
	TriggeredTransition = EDistanceMatchTrigger::TurnInPlace;


	if ((++CurrentInstanceId) > 1000000)
	{
		CurrentInstanceId = 0;
	}
}

void UDistanceMatching::TriggerTurnInPlaceTo(FVector DesiredDirection)
{
	MarkerVector = DesiredDirection; //Could be sourced from Camera

	DistanceMatchType = EDistanceMatchType::Forward;
	DistanceMatchBasis = EDistanceMatchBasis::Rotational;
	TriggeredTransition = EDistanceMatchTrigger::TurnInPlace;


	if ((++CurrentInstanceId) > 1000000)
	{
		CurrentInstanceId = 0;
	}
}

void UDistanceMatching::TriggerJump(float DeltaTime)
{
	DistanceMatchType = EDistanceMatchType::Both;
	TriggeredTransition = EDistanceMatchTrigger::Jump;
	bDestinationReached = false;

	if ((++CurrentInstanceId) > 1000000)
	{
		CurrentInstanceId = 0;
	}
}

void UDistanceMatching::StopDistanceMatching()
{
	DistanceMatchType = EDistanceMatchType::None;
	TriggeredTransition = EDistanceMatchTrigger::None;
	bDestinationReached = false;

	if ((++CurrentInstanceId) > 1000000)
	{
		CurrentInstanceId = 0;
	}
}

float UDistanceMatching::GetMarkerDistance()
{
	return DistanceToMarker;
}

void UDistanceMatching::DetectTransitions(float DeltaTime)
{
	if(!MovementComponent)
	{
		return;
	}

	FVector Velocity = MovementComponent->Velocity;
	FVector Acceleration = MovementComponent->GetCurrentAcceleration();
	float SpeedSqr = Velocity.SizeSquared();
	float AccelSqr = Acceleration.SizeSquared();

	//Detect Starts
	if(DistanceMatchType != EDistanceMatchType::Backward
		&& LastFrameAccelSqr < 0.001f && AccelSqr > 0.001f)
	{
		TriggerStart(DeltaTime);
		return;
	}
	else if(DistanceMatchType != EDistanceMatchType::Forward
		&& SpeedSqr > 0.001f && AccelSqr < 0.001f)
	{
		TriggerStop(DeltaTime);
		return;
	}
	else if(DistanceMatchType != EDistanceMatchType::Both)
	{
		//Detect plants
		//Can only plant if the speed is above a certain amount
		if(Velocity.SizeSquared() > MinPlantSpeed * MinPlantSpeed
			&& Acceleration.SizeSquared() > MinPlantAccel * MinPlantAccel)
		{
			Velocity.Normalize();
			Acceleration.Normalize();

			float Angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(Velocity, Acceleration)));

			if(FMath::Abs(Angle) > MinPlantDetectionAngle)
			{
				TriggerPlant(DeltaTime);
				return;
			}
		}
	}

	LastFrameAccelSqr = AccelSqr;
}

EDistanceMatchTrigger UDistanceMatching::GetAndConsumeTriggeredTransition()
{
	EDistanceMatchTrigger ConsumedTrigger = TriggeredTransition;
	TriggeredTransition = EDistanceMatchTrigger::None;

	return ConsumedTrigger;
}

float UDistanceMatching::CalculateMarkerDistance()
{
	if(!ParentActor || DistanceMatchType == EDistanceMatchType::None)
	{
		return 0.0f;
	}

	if(DistanceMatchBasis == EDistanceMatchBasis::Positional)
	{
		return (ParentActor->GetActorLocation() - MarkerVector).Size();
	}
	else
	{
		return FMath::RadiansToDegrees(acosf(FVector::DotProduct(ParentActor->GetActorForwardVector(), MarkerVector)));
	}
}

float UDistanceMatching::GetTimeToMarker()
{
	return TimeToMarker;
}

EDistanceMatchType UDistanceMatching::GetDistanceMatchType()
{
	return DistanceMatchType;
}

uint32 UDistanceMatching::GetCurrentInstanceId()
{
	return CurrentInstanceId;
}

FDistanceMatchPayload UDistanceMatching::GetDistanceMatchPayload()
{
	bool bTrigger = TriggeredTransition != EDistanceMatchTrigger::None ? true : false;
	TriggeredTransition = EDistanceMatchTrigger::None;

	return FDistanceMatchPayload(bTrigger, DistanceMatchType, DistanceMatchBasis, DistanceToMarker);
}

// Called when the game starts
void UDistanceMatching::BeginPlay()
{
	Super::BeginPlay();

	PrimaryComponentTick.bCanEverTick = bAutomaticTriggers;
	
	ParentActor = GetOwner();
	MovementComponent = Cast<UCharacterMovementComponent>(GetOwner()->GetComponentByClass(UCharacterMovementComponent::StaticClass()));

	if (MovementComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("DistanceMatching: Cannot find Movement Component to predict motion. Disabling component."))

		PrimaryComponentTick.bCanEverTick = false;
		bAutomaticTriggers = false;
		return;
	}
}


// Called every frame
void UDistanceMatching::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(bAutomaticTriggers)
	{
		DetectTransitions(DeltaTime);
	}

	if(DistanceMatchType == EDistanceMatchType::None)
	{
		return;
	}

	DistanceToMarker = CalculateMarkerDistance();

	switch(DistanceMatchType)
	{
		case EDistanceMatchType::Backward: 
		{
			DistanceToMarker *= -1.0f;
		} 
		break;
		case EDistanceMatchType::Forward: 
		{
			if(DistanceToMarker < DistanceTolerance)
			{
				StopDistanceMatching();
			}
		}
		break;
		case EDistanceMatchType::Both:
		{
			if(!bDestinationReached)
			{
				if (DistanceToMarker < DistanceTolerance)
				{
					bDestinationReached = true;
				}
			}
			else
			{
				DistanceToMarker *= -1.0f;
			}
		}
		break;
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	int32 DebugLevel = CVarDistanceMatchingDebug.GetValueOnGameThread();

	if (DebugLevel == 1 && DistanceMatchType != EDistanceMatchType::None)
	{
		FColor DebugColor = FColor::Green;
		switch (DistanceMatchType)
		{
			case EDistanceMatchType::Backward: DebugColor = FColor::Blue; break;
			case EDistanceMatchType::Forward: DebugColor = FColor::Green; break;
			case EDistanceMatchType::Both: DebugColor = FColor::Purple; break;
		}

		UWorld* World = ParentActor->GetWorld();

		if(World)
		{
			DrawDebugSphere(World, MarkerVector, 10, 16, DebugColor, false, -1.0f, 0, 0.5f);
		}
	}
#endif
}



FDistanceMatchingModule::FDistanceMatchingModule()
	: LastKeyChecked(0),
	MaxDistance(0.0f),
	AnimSequence(nullptr)
{

}

void FDistanceMatchingModule::Setup(UAnimSequenceBase* InAnimSequence, const FName& DistanceCurveName)
{
	AnimSequence = InAnimSequence;

	if(!AnimSequence)
	{
		return;
	}

	FSmartName CurveName;
#if ENGINE_MAJOR_VERSION > 4
	const FRawCurveTracks& RawCurves = InAnimSequence->GetCurveData();
#else
	const FRawCurveTracks& RawCurves = InAnimSequence->RawCurveData;
#endif
	InAnimSequence->GetSkeleton()->GetSmartNameByName(USkeleton::AnimCurveMappingName, DistanceCurveName, CurveName);

	if (CurveName.IsValid())
	{
		const FFloatCurve* DistanceCurve = static_cast<const FFloatCurve*>(RawCurves.GetCurveData(CurveName.UID));

		if(DistanceCurve)
		{
			CurveKeys = DistanceCurve->FloatCurve.GetCopyOfKeys();

			for (FRichCurveKey& Key : CurveKeys)
			{
				if (FMath::Abs(Key.Value) > MaxDistance)
				{
					MaxDistance = FMath::Abs(Key.Value);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Distance matching curve could not be found. Distance matching node will not operate as expected."));
		}
	}
}

void FDistanceMatchingModule::Initialize()
{
	LastKeyChecked = 0;
}

float FDistanceMatchingModule::FindMatchingTime(float DesiredDistance, bool bNegateCurve)
{
	if(CurveKeys.Num() < 2
	|| FMath::Abs(DesiredDistance) > FMath::Abs(MaxDistance))
	{
		return -1.0f;
	}

	//Find the time in the animation with the matching distance
	LastKeyChecked = FMath::Clamp(LastKeyChecked, 0, CurveKeys.Num() - 1);
	FRichCurveKey* PKey = &CurveKeys[LastKeyChecked];
	FRichCurveKey* SKey = nullptr;

	const float Negator = bNegateCurve ? -1.0f : 1.0f;

	for (int32 i = LastKeyChecked; i < CurveKeys.Num(); ++i)
	{
		FRichCurveKey& Key = CurveKeys[i];

		if (Key.Value * Negator > DesiredDistance)
		{
			PKey = &Key;
			//LastKeyChecked = i;
		}
		else
		{
			SKey = &Key;
			break;
		}
	}

	if (!SKey)
	{
		return PKey->Time;
	}

	const float DV = (SKey->Value * Negator) - (PKey->Value * Negator);

	if(DV < 0.000001f)
	{
		return PKey->Time;
	}

	float DT = SKey->Time - PKey->Time;

	return ((DT / DV) * (DesiredDistance - (PKey->Value * Negator))) + PKey->Time;
}

//bool UDistanceMatching::CalculateStartLocation(FVector& OutStartLocation, const float DeltaTime, const int32 MaxIterations)
//{
//	/**This is currently a placeholder. The idea is to iterate backwards in time and see where the character would have been
//	stationary based on their current movement state and input. */
//	const FVector CurrentLocation = ParentActor->GetActorLocation();
//	const FVector Velocity = MovementComponent->Velocity;
//	const FVector Acceleration = MovementComponent->GetCurrentAcceleration();
//	float Friction = MovementComponent->GroundFriction;
//	float MoveAcceleration = MovementComponent->GetMaxAcceleration();
//
//	OutStartLocation = CurrentLocation;
//
//	const float MIN_TICK_TIME = 1e-6;
//	if (DeltaTime < MIN_TICK_TIME)
//	{
//		return false;
//	}
//
//	const bool bZeroAcceleration = Acceleration.IsZero();
//	Friction = FMath::Max(Friction, 0.0f);
//
//	FVector LastVelocity = bZeroAcceleration ? Velocity : Velocity.ProjectOnToNormal(Acceleration.GetSafeNormal());
//	LastVelocity.Z = 0;
//
//	FVector LastLocation = CurrentLocation;
//
//	int Iterations = 0;
//	float PredictionTime = 0.0f;
//
//	while (Iterations < MaxIterations)
//	{
//		++Iterations;
//
//		const FVector OldVel = LastVelocity;
//
//		float RemainingTime = DeltaTime;
//		const float MaxDeltaTime = (1.0f / 33.0f);
//
//		// Decelerate to brake to a stop
//		const FVector BrakeDecel = -MoveAcceleration * -LastVelocity.GetSafeNormal();
//		while (RemainingTime >= MIN_TICK_TIME)
//		{
//			// Zero friction uses constant deceleration, so no need for iteration.
//			const float dt = ((RemainingTime > MaxDeltaTime/* && !bZeroFriction*/) ? FMath::Min(MaxDeltaTime, RemainingTime * 0.5f) : RemainingTime);
//			RemainingTime -= dt;
//
//			// apply friction and braking
//			LastVelocity = LastVelocity + ((-Friction) * LastVelocity + BrakeDecel) * dt;
//
//			// Don't reverse direction
//			if ((LastVelocity | OldVel) <= 0.f)
//			{
//				LastVelocity = FVector::ZeroVector;
//				break;
//			}
//		}
//
//	}
//
//	return true;
//}

bool UDistanceMatching::CalculateStopLocation(FVector& OutStopLocation, const float DeltaTime, const int32 MaxIterations)
{
	const FVector CurrentLocation = ParentActor->GetActorLocation();
	const FVector Velocity = MovementComponent->Velocity;
	const FVector Acceleration = MovementComponent->GetCurrentAcceleration();
	float Friction = MovementComponent->GroundFriction * MovementComponent->BrakingFrictionFactor;
	float BrakingDeceleration = MovementComponent->BrakingDecelerationWalking;

	const float MIN_TICK_TIME = 1e-6;
	if (DeltaTime < MIN_TICK_TIME)
	{
		return false;
	}
	
	const bool bZeroAcceleration = Acceleration.IsZero();

	if ((Acceleration | Velocity) > 0.0f)
	{
		return false;
	}
	
	BrakingDeceleration = FMath::Max(BrakingDeceleration, 0.0f);
	Friction = FMath::Max(Friction, 0.0f);
	const bool bZeroFriction = (Friction < 0.00001f);
	const bool bZeroBraking = (BrakingDeceleration == 0.00001f);

	//Won't stop if there is no Braking acceleration or friction
	if (bZeroAcceleration && bZeroFriction && bZeroBraking)
	{
		return false;
	}

	FVector LastVelocity = bZeroAcceleration ? Velocity : Velocity.ProjectOnToNormal(Acceleration.GetSafeNormal());
	LastVelocity.Z = 0;

	FVector LastLocation = CurrentLocation;

	int Iterations = 0;
	float PredictionTime = 0.0f;
	while (Iterations < MaxIterations)
	{
		++Iterations;

		const FVector OldVel = LastVelocity;

		// Only apply braking if there is no acceleration, or we are over our max speed and need to slow down to it.
		if (bZeroAcceleration)
		{
			// subdivide braking to get reasonably consistent results at lower frame rates
			// (important for packet loss situations w/ networking)
			float RemainingTime = DeltaTime;
			const float MaxDeltaTime = (1.0f / 33.0f);

			// Decelerate to brake to a stop
			const FVector BrakeDecel = (bZeroBraking ? FVector::ZeroVector : (-BrakingDeceleration * LastVelocity.GetSafeNormal()));
			while (RemainingTime >= MIN_TICK_TIME)
			{
				// Zero friction uses constant deceleration, so no need for iteration.
				const float DT = ((RemainingTime > MaxDeltaTime && !bZeroFriction) ? FMath::Min(MaxDeltaTime, RemainingTime * 0.5f) : RemainingTime);
				RemainingTime -= DT;
				
				// apply friction and braking
				LastVelocity = LastVelocity + ((-Friction) * LastVelocity + BrakeDecel) * DT;

				// Don't reverse direction
				if ((LastVelocity | OldVel) <= 0.f)
				{
					LastVelocity = FVector::ZeroVector;
					break;
				}
			}

			// Clamp to zero if nearly zero, or if below min threshold and braking.
			const float VSizeSq = LastVelocity.SizeSquared();
			if (VSizeSq <= 1.f || (!bZeroBraking && VSizeSq <= FMath::Square(10)))
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

		LastLocation += LastVelocity * DeltaTime;

		PredictionTime += DeltaTime;

		// Clamp to zero if nearly zero, or if below min threshold and braking.
		const float VSizeSq = LastVelocity.SizeSquared();
		if (VSizeSq <= 1.f
			|| (LastVelocity | OldVel) <= 0.f)
		{
			TimeToMarker = PredictionTime;
			OutStopLocation = LastLocation;
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE