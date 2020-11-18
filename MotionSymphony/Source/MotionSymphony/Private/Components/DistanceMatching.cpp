// Copyright 2020 Kenneth Claassen. All Rights Reserved.

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
	PredictionIterations(-1),
	MinPlantDetectionAngle(130.0f),
	MinPlantSpeed(100.0f),
	MinPlantAccel(100.0f),
	TriggeredTransition(EDistanceMatchTrigger::None),
	CurrentInstanceId(0),
	bDestinationReached(false),
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
	
	
	if((++CurrentInstanceId) > 1000000)
		CurrentInstanceId = 0;
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
			CurrentInstanceId = 0;
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
			CurrentInstanceId = 0;
	}
}

void UDistanceMatching::TriggerPivotFrom()
{
	DistanceMatchType = EDistanceMatchType::Backward;
	DistanceMatchBasis = EDistanceMatchBasis::Rotational;
	TriggeredTransition = EDistanceMatchTrigger::Pivot;

	MarkerVector = ParentActor->GetActorForwardVector();

	if ((++CurrentInstanceId) > 1000000)
		CurrentInstanceId = 0;
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
			CurrentInstanceId = 0;
	}
}

void UDistanceMatching::TriggerTurnInPlaceFrom()
{
	MarkerVector = ParentActor->GetActorForwardVector();

	DistanceMatchType = EDistanceMatchType::Backward;
	DistanceMatchBasis = EDistanceMatchBasis::Rotational;
	TriggeredTransition = EDistanceMatchTrigger::TurnInPlace;


	if ((++CurrentInstanceId) > 1000000)
		CurrentInstanceId = 0;
}

void UDistanceMatching::TriggerTurnInPlaceTo(FVector DesiredDirection)
{
	MarkerVector = DesiredDirection; //Could be sourced from Camera

	DistanceMatchType = EDistanceMatchType::Forward;
	DistanceMatchBasis = EDistanceMatchBasis::Rotational;
	TriggeredTransition = EDistanceMatchTrigger::TurnInPlace;


	if ((++CurrentInstanceId) > 1000000)
		CurrentInstanceId = 0;
}

void UDistanceMatching::TriggerJump(float DeltaTime)
{
	DistanceMatchType = EDistanceMatchType::Both;
	TriggeredTransition = EDistanceMatchTrigger::Jump;
	bDestinationReached = false;

	if ((++CurrentInstanceId) > 1000000)
		CurrentInstanceId = 0;
}

void UDistanceMatching::StopDistanceMatching()
{
	DistanceMatchType = EDistanceMatchType::None;
	TriggeredTransition = EDistanceMatchTrigger::None;
	bDestinationReached = false;

	if ((++CurrentInstanceId) > 1000000)
		CurrentInstanceId = 0;
}

float UDistanceMatching::GetMarkerDistance()
{
	return DistanceToMarker;
}

void UDistanceMatching::DetectTransitions(float DeltaTime)
{
	if(!MovementComponent)
		return;

	FVector Velocity = MovementComponent->Velocity;
	FVector Acceleration = MovementComponent->GetCurrentAcceleration();

	//Detect Starts

	//Detect Stops

	if(DistanceMatchType != EDistanceMatchType::Both)
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
		return 0.0f;

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

//void UDistanceMatching::PredictPlantPoint(float DeltaTime)
//{
//	float IterationTime = DeltaTime;
//
//	if (PredictionIterations > 0)
//		IterationTime = 1.0f / PredictionIterations;
//
//	float VelocityEpsilon = 10.f * FMath::Square(IterationTime);
//	FVector StartVelocity = MovementComponent->Velocity;
//	FVector Acceleration = MovementComponent->GetCurrentAcceleration();
//	FVector CurrentVelocity = StartVelocity;
//
//	MarkerVector = ParentActor->GetActorLocation();
//	int32 iter = 0;
//	for (; iter < 100; ++iter)
//	{
//		MarkerVector += CurrentVelocity * IterationTime;
//
//		CurrentVelocity += (IterationTime * Acceleration);
//
//		float Angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(CurrentVelocity.GetSafeNormal2D(), StartVelocity.GetSafeNormal2D())));
//
//		if (FMath::Abs(Angle) > 90.0f)
//			break;
//	}
//
//	TimeToMarker = IterationTime * iter;
//}

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
		return;

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
		return;

	FSmartName CurveName;
	const FRawCurveTracks& RawCurves = InAnimSequence->RawCurveData;
	InAnimSequence->GetSkeleton()->GetSmartNameByName(USkeleton::AnimCurveMappingName, DistanceCurveName, CurveName);

	if (CurveName.IsValid())
	{
		const FFloatCurve* DistanceCurve = static_cast<const FFloatCurve*>(RawCurves.GetCurveData(CurveName.UID));
		CurveKeys = DistanceCurve->FloatCurve.GetCopyOfKeys();

		for (FRichCurveKey& Key : CurveKeys)
		{
			if (Key.Value > MaxDistance)
				MaxDistance = Key.Value;
		}
	}
}

void FDistanceMatchingModule::Initialize()
{
	LastKeyChecked = 0;
}

float FDistanceMatchingModule::FindMatchingTime(float DesiredDistance)
{
	if(CurveKeys.Num() < 2)
		return -1.0f;

	if(DesiredDistance > MaxDistance)
		return -1.0f;

	//Find the time in the animation with the matching distance
	LastKeyChecked = FMath::Clamp(LastKeyChecked, 0, CurveKeys.Num() - 1);
	FRichCurveKey* PKey = &CurveKeys[LastKeyChecked];
	FRichCurveKey* SKey = nullptr;

	for (int32 i = LastKeyChecked; i < CurveKeys.Num(); ++i)
	{
		FRichCurveKey& Key = CurveKeys[i];

		if (Key.Value > DesiredDistance)
		{
			PKey = &Key;
			LastKeyChecked = i;
		}
		else
		{
			SKey = &Key;
			break;
		}
	}

	if (!SKey)
		return PKey->Time;

	float dV = SKey->Value - PKey->Value;

	if(dV < 0.000001f)
		return PKey->Time;

	float dT = SKey->Time - PKey->Time;

	return ((dT / dV) * (DesiredDistance - PKey->Value)) + PKey->Time;
}

bool UDistanceMatching::CalculateStopLocation(FVector& OutStopLocation, const float DeltaTime, const int32 MaxIterations)
{
	const FVector CurrentLocation = ParentActor->GetActorLocation();
	const FVector Velocity = MovementComponent->Velocity;
	const FVector Acceleration = MovementComponent->GetCurrentAcceleration();
	float Friction = MovementComponent->GroundFriction * MovementComponent->BrakingFrictionFactor;
	float BrakingDeceleration = MovementComponent->BrakingDecelerationWalking;

	const float MIN_TICK_TIME = 1e-6;
	if (DeltaTime < MIN_TICK_TIME)
		return false;
	
	const bool bZeroAcceleration = Acceleration.IsZero();

	if ((Acceleration | Velocity) > 0.0f)
		return false;
	
	BrakingDeceleration = FMath::Max(BrakingDeceleration, 0.0f);
	Friction = FMath::Max(Friction, 0.0f);
	const bool bZeroFriction = (Friction < 0.00001f);
	const bool bZeroBraking = (BrakingDeceleration == 0.00001f);

	//Won't stop if there is no Braking acceleration or friction
	if (bZeroAcceleration && bZeroFriction && bZeroBraking)
		return false;

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
				const float dt = ((RemainingTime > MaxDeltaTime && !bZeroFriction) ? FMath::Min(MaxDeltaTime, RemainingTime * 0.5f) : RemainingTime);
				RemainingTime -= dt;
				
				// apply friction and braking
				LastVelocity = LastVelocity + ((-Friction) * LastVelocity + BrakeDecel) * dt;

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