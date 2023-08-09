// Copyright 2023 Kenneth Claassen. All Rights Reserved.

#include "Objects/MatchFeatures/MatchFeature_Distance.h"
#include "MMPreProcessUtils.h"
#include "Objects/Tags/Tag_DistanceMarker.h"
#include "Animation/AnimComposite.h"
#include "MotionAnimAsset.h"

UMatchFeature_Distance::UMatchFeature_Distance(const FObjectInitializer& ObjectInitializer)
	: UMatchFeatureBase(ObjectInitializer),
	DistanceMatchTrigger(EDistanceMatchTrigger::None),
	DistanceMatchType(EDistanceMatchType::None),
	DistanceMatchBasis(EDistanceMatchBasis::Positional)
{
	
}

bool UMatchFeature_Distance::IsSetupValid() const
{
	bool bIsValid = true;
	
	if(DistanceMatchTrigger != EDistanceMatchTrigger::None)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config validity check failed. Distance Match Feature trigger parameter set to 'None'."))
		bIsValid = false;
	}

	if(DistanceMatchType != EDistanceMatchType::None)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config validity check failed. Distance Match Feature type parameter set to 'None'."))
		bIsValid = false;
	}

	return bIsValid;
}

int32 UMatchFeature_Distance::Size() const
{
	return 1;
}

void UMatchFeature_Distance::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
                                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable)
{
	if(!InSequence.Sequence)
	{
		*ResultLocation = 0.0f;
		return;
	}
	
	for(FAnimNotifyEvent& NotifyEvent : InSequence.Tags)
	{
		if(const UTag_DistanceMarker* Tag = Cast<UTag_DistanceMarker>(NotifyEvent.Notify))
		{
			if(DoesTagMatch(Tag))
			{
				const float TagTime = NotifyEvent.GetTriggerTime();

				switch(DistanceMatchType)
				{
				case EDistanceMatchType::Backward: 
					{
						if(Time > TagTime && Time < TagTime + Tag->Tail)
						{
							*ResultLocation = -ExtractSqrDistanceToMarker(InSequence.Sequence->ExtractRootMotion(
								TagTime, Time - TagTime, false));

							return;
						}
					} break;
				case EDistanceMatchType::Forward:
					{
						if(Time > TagTime - Tag->Lead && Time < TagTime)
						{
							*ResultLocation = ExtractSqrDistanceToMarker(InSequence.Sequence->ExtractRootMotion(
								Time, TagTime-Time, false));

							return;
						}
					} break;
				case EDistanceMatchType::Both:
					{
						if(Time > TagTime - Tag->Lead && Time < TagTime) //Forward
						{
							*ResultLocation = ExtractSqrDistanceToMarker(InSequence.Sequence->ExtractRootMotion(
								Time, TagTime-Time, false));

							return;
						}
						
						if(Time > TagTime && Time < TagTime + Tag->Tail) //Backward
						{
							*ResultLocation = -ExtractSqrDistanceToMarker(InSequence.Sequence->ExtractRootMotion(
								TagTime, Time - TagTime, false));

							return;
						}
					}break;
				
				default: ;
				}
			}
		}
	}

	*ResultLocation = 0.0f;
}

void UMatchFeature_Distance::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite, const float Time,
                                                const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable)
{
	if(!InComposite.AnimComposite)
	{
		*ResultLocation = 0.0f;
		return;
	}
	
	for(FAnimNotifyEvent& NotifyEvent : InComposite.Tags)
	{
		if(const UTag_DistanceMarker* Tag = Cast<UTag_DistanceMarker>(NotifyEvent.Notify))
		{
			if(DoesTagMatch(Tag))
			{
				const float TagTime = NotifyEvent.GetTriggerTime();

				switch(DistanceMatchType)
				{
				case EDistanceMatchType::Backward: 
					{
						if(Time > TagTime && Time < TagTime + Tag->Tail)
						{
							*ResultLocation = -ExtractSqrDistanceToMarker(ExtractCompositeRootMotion(
								InComposite.AnimComposite,TagTime, Time - TagTime));

							return;
						}
					} break;
				case EDistanceMatchType::Forward:
					{
						if(Time > TagTime - Tag->Lead && Time < TagTime)
						{
							*ResultLocation = ExtractSqrDistanceToMarker(ExtractCompositeRootMotion(
								InComposite.AnimComposite,Time, TagTime-Time));

							return;
						}
					} break;
				case EDistanceMatchType::Both:
					{
						if(Time > TagTime - Tag->Lead && Time < TagTime) //Forward
						{
							*ResultLocation = ExtractSqrDistanceToMarker(ExtractCompositeRootMotion(
								InComposite.AnimComposite,Time, TagTime-Time));

							return;
						}

						if(Time > TagTime && Time < TagTime + Tag->Tail) //Backward
						{
							*ResultLocation = -ExtractSqrDistanceToMarker(ExtractCompositeRootMotion(
								InComposite.AnimComposite,TagTime, Time - TagTime));

							return;
						}
					}break;
				
				default: ;
				}
			}
		}
	}
	
	*ResultLocation = 0.0f;
}

void UMatchFeature_Distance::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
                                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable,
                                                const FVector2D BlendSpacePosition)
{
	if(!InBlendSpace.BlendSpace)
	{
		*ResultLocation = 0.0f;
		return;
	}

	for(FAnimNotifyEvent& NotifyEvent : InBlendSpace.Tags)
	{
		if(const UTag_DistanceMarker* Tag = Cast<UTag_DistanceMarker>(NotifyEvent.Notify))
		{
			if(DoesTagMatch(Tag))
			{
				const float TagTime = NotifyEvent.GetTriggerTime();

				switch(DistanceMatchType)
				{
				case EDistanceMatchType::Backward: 
					{
						if(Time > TagTime && Time < TagTime + Tag->Tail)
						{
							*ResultLocation = -ExtractSqrDistanceToMarker(ExtractBlendSpaceRootMotion(
								InBlendSpace.BlendSpace, BlendSpacePosition, TagTime, Time - TagTime));

							return;
						}
					} break;
				case EDistanceMatchType::Forward:
					{
						if(Time > TagTime - Tag->Lead && Time < TagTime)
						{
							*ResultLocation = ExtractSqrDistanceToMarker(ExtractBlendSpaceRootMotion(
								InBlendSpace.BlendSpace, BlendSpacePosition, Time, TagTime - Time));

							return;
						}
					} break;
				case EDistanceMatchType::Both:
					{
						if(Time > TagTime - Tag->Lead && Time < TagTime) //Forward
						{
							*ResultLocation = ExtractSqrDistanceToMarker(ExtractBlendSpaceRootMotion(
								InBlendSpace.BlendSpace, BlendSpacePosition, Time, TagTime - Time));

							return;
						}
						
						if(Time > TagTime && Time < TagTime + Tag->Tail) //Backward
						{
							*ResultLocation = -ExtractSqrDistanceToMarker(ExtractBlendSpaceRootMotion(
								InBlendSpace.BlendSpace, BlendSpacePosition, TagTime, Time - TagTime));

							return;
						}
					}break;
				
				default: ;
				}
			}
		}
	}

	*ResultLocation = 0.0f;
}

bool UMatchFeature_Distance::NextPoseToleranceTest(const TArray<float>& DesiredInputArray,
	const TArray<float>& PoseMatrix, const int32 MatrixStartIndex, const int32 FeatureOffset,
	const float PositionTolerance, const float RotationTolerance)
{
	return Super::NextPoseToleranceTest(DesiredInputArray, PoseMatrix, MatrixStartIndex, FeatureOffset,
	                                    PositionTolerance,
	                                    RotationTolerance);
}

bool UMatchFeature_Distance::DoesTagMatch(const UTag_DistanceMarker* InTag) const
{
	return InTag->DistanceMatchTrigger == DistanceMatchTrigger
		&& InTag->DistanceMatchType == DistanceMatchType
		&& InTag->DistanceMatchBasis == DistanceMatchBasis;
}

float UMatchFeature_Distance::ExtractSqrDistanceToMarker(const FTransform& InTagTransform) const
{
	if(DistanceMatchBasis == EDistanceMatchBasis::Positional) //Positional
	{
		const FVector TagLocation = InTagTransform.GetLocation();
		return TagLocation.SquaredLength();
	}
	else //Rotational
	{
		const FRotator TagRotation = InTagTransform.GetRotation().GetNormalized().Rotator();
		return TagRotation.Yaw;
	}
}

FTransform UMatchFeature_Distance::ExtractBlendSpaceRootMotion(const UBlendSpace* InBlendSpace,
	const FVector2D InBlendSpacePosition, const float InBaseTime, const float InDeltaTime)
{
	if(!InBlendSpace)
	{
		return FTransform();
	}
	
	TArray<FBlendSampleData> SampleDataList;
	int32 CachedTriangulationIndex = -1;
	InBlendSpace->GetSamplesFromBlendInput(FVector(InBlendSpacePosition.X, InBlendSpacePosition.Y, 0.0f),
		SampleDataList, CachedTriangulationIndex, false);

	FRootMotionMovementParams RootMotionParams;
	RootMotionParams.Clear();
	
	FMMPreProcessUtils::ExtractRootMotionParams(RootMotionParams, SampleDataList, InBaseTime, InDeltaTime, false);
	
	return RootMotionParams.GetRootMotionTransform();
}

FTransform UMatchFeature_Distance::ExtractCompositeRootMotion(const UAnimComposite* InAnimComposite,
	const float InBaseTime, const float InDeltaTime)
{
	if(!InAnimComposite)
	{
		return FTransform();
	}

	FRootMotionMovementParams RootMotionParams;
	RootMotionParams.Clear();
	InAnimComposite->ExtractRootMotionFromTrack(InAnimComposite->AnimationTrack, InBaseTime, InBaseTime + InDeltaTime, RootMotionParams);

	return RootMotionParams.GetRootMotionTransform();
}


