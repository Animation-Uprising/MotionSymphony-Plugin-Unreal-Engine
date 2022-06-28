#include "DataOriented/MatchFeature_Trajectory2D.h"

#include <ThirdParty/SPIRV-Reflect/SPIRV-Reflect/include/spirv/unified1/spirv.h>

#include "MMPreProcessUtils.h"
#include "MotionAnimAsset.h"
#include "MotionDataAsset.h"
#include "MotionMatchConfig.h"
#include "MotionMatchingUtils.h"
#include "Trajectory.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/DebugSkelMeshComponent.h"


int32 UMatchFeature_Trajectory2D::Size() const
{
	return TrajectoryTiming.Num() * 4;
}

void UMatchFeature_Trajectory2D::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
                                                    const float Time, const float PoseInterval, const bool bMirror,
                                                    UMirroringProfile* MirrorProfile)
{
	if(!InSequence.Sequence)
	{
		*ResultLocation = 0.0f;
		for(int32 i = 1; i < Size(); ++i)
		{
			++ResultLocation;
			*ResultLocation = 0.0f;
		}

		return;
	}
	
	//--ResultLocation;
	for(const float PointTime : TrajectoryTiming)
	{
		FTrajectoryPoint TrajectoryPoint;
		if(InSequence.bLoop)
		{
			FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(TrajectoryPoint, InSequence.Sequence, Time,
				PointTime * InSequence.PlayRate);
		}
		else if(PointTime < 0.0f) //Past Trajectory Point
			{
			FMMPreProcessUtils::ExtractPastTrajectoryPoint(TrajectoryPoint, InSequence.Sequence, Time,
				PointTime * InSequence.PlayRate, InSequence.PastTrajectory, InSequence.PrecedingMotion);
			}
		else //Future Trajectory Point
			{
			FMMPreProcessUtils::ExtractFutureTrajectoryPoint(TrajectoryPoint, InSequence.Sequence, Time,
				PointTime * InSequence.PlayRate, InSequence.FutureTrajectory, InSequence.FollowingMotion);
			}
		
	
		*ResultLocation = static_cast<float>(bMirror ? -TrajectoryPoint.Position.X : TrajectoryPoint.Position.X);
		++ResultLocation;
		*ResultLocation = static_cast<float>(TrajectoryPoint.Position.Y);
		++ResultLocation;
		*ResultLocation = bMirror ? -FMath::Cos(FMath::DegreesToRadians(TrajectoryPoint.RotationZ))
			: FMath::Cos(FMath::DegreesToRadians(TrajectoryPoint.RotationZ));
		++ResultLocation;
		*ResultLocation = FMath::Sin(FMath::DegreesToRadians(TrajectoryPoint.RotationZ));
		++ResultLocation;
	}
}

void UMatchFeature_Trajectory2D::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
                                                    const float Time, const float PoseInterval, const bool bMirror,
                                                    UMirroringProfile* MirrorProfile)
{
	if(!InComposite.AnimComposite)
	{
		*ResultLocation = 0.0f;
		for(int32 i = 1; i < Size(); ++i)
		{
			++ResultLocation;
			*ResultLocation = 0.0f;
		}

		return;
	}
	
	for(const float PointTime : TrajectoryTiming)
	{
		FTrajectoryPoint TrajectoryPoint;
		if(InComposite.bLoop)
		{
			FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(TrajectoryPoint, InComposite.AnimComposite,
				Time, PointTime * InComposite.PlayRate);
		}
		else if(PointTime < 0.0f) //Past Trajectory Point
			{
			FMMPreProcessUtils::ExtractPastTrajectoryPoint(TrajectoryPoint, InComposite.AnimComposite, Time,
				PointTime * InComposite.PlayRate, InComposite.PastTrajectory, InComposite.PrecedingMotion);
			}
		else //Future Trajectory Point
			{
			FMMPreProcessUtils::ExtractFutureTrajectoryPoint(TrajectoryPoint, InComposite.AnimComposite, Time,
				PointTime * InComposite.PlayRate, InComposite.FutureTrajectory, InComposite.FollowingMotion);
			}
		
		*ResultLocation = static_cast<float>(bMirror ? -TrajectoryPoint.Position.X : TrajectoryPoint.Position.X);
		++ResultLocation;
		*ResultLocation = static_cast<float>(TrajectoryPoint.Position.Y);
		++ResultLocation;
		*ResultLocation = bMirror ? -FMath::Cos(FMath::DegreesToRadians(TrajectoryPoint.RotationZ))
			: FMath::Cos(FMath::DegreesToRadians(TrajectoryPoint.RotationZ));
		++ResultLocation;
		*ResultLocation = FMath::Sin(FMath::DegreesToRadians(TrajectoryPoint.RotationZ));
		++ResultLocation;
	}
}

void UMatchFeature_Trajectory2D::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
                                                    const float Time, const float PoseInterval, const bool bMirror,
                                                    UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition)
{
	if(!InBlendSpace.BlendSpace)
	{
		*ResultLocation = 0.0f;
		for(int32 i = 1; i < Size(); ++i)
		{
			++ResultLocation;
			*ResultLocation = 0.0f;
		}

		return;
	}

	TArray<FBlendSampleData> SampleDataList;
	int32 CachedTriangulationIndex = -1;
	InBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(BlendSpacePosition.X, BlendSpacePosition.Y, 0.0f),
		SampleDataList, CachedTriangulationIndex, false);
	
	for(const float PointTime : TrajectoryTiming)
	{
		FTrajectoryPoint TrajectoryPoint;
		if(InBlendSpace.bLoop)
		{
			FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(TrajectoryPoint, SampleDataList, Time,
				PointTime * InBlendSpace.PlayRate);
		}
		else if(PointTime < 0.0f) //Past Trajectory Point
		{
			FMMPreProcessUtils::ExtractPastTrajectoryPoint(TrajectoryPoint, SampleDataList, Time,
				PointTime * InBlendSpace.PlayRate, InBlendSpace.PastTrajectory, InBlendSpace.PrecedingMotion);
		}
		else //Future Trajectory Point
		{
			FMMPreProcessUtils::ExtractFutureTrajectoryPoint(TrajectoryPoint, SampleDataList, Time,
				PointTime * InBlendSpace.PlayRate, InBlendSpace.FutureTrajectory, InBlendSpace.FollowingMotion);
		}
		
		*ResultLocation = static_cast<float>(bMirror ? -TrajectoryPoint.Position.X : TrajectoryPoint.Position.X);
		++ResultLocation;
		*ResultLocation = static_cast<float>(TrajectoryPoint.Position.Y);
		++ResultLocation;
		*ResultLocation = bMirror ? -FMath::Cos(FMath::DegreesToRadians(TrajectoryPoint.RotationZ))
			: FMath::Cos(FMath::DegreesToRadians(TrajectoryPoint.RotationZ));
		++ResultLocation;
		*ResultLocation = FMath::Sin(FMath::DegreesToRadians(TrajectoryPoint.RotationZ));
		++ResultLocation;
	}
}

void UMatchFeature_Trajectory2D::ApplyInputBlending(TArray<float>& DesiredInputArray,
	const TArray<float>& CurrentPoseArray, const int32 FeatureOffset, const float Weight)
{
	if(DesiredInputArray.Num() < TrajectoryTiming.Num() * 4
		|| CurrentPoseArray.Num() != DesiredInputArray.Num())
	{
		return;
	}

	const float TotalTime = FMath::Max(0.00001f, TrajectoryTiming.Last());
	
	for(int32 i = 0; i < TrajectoryTiming.Num(); ++i)
	{
		const int32 StartIndex = FeatureOffset + i * 4;
		const float Time = TrajectoryTiming[i];

		if(Time > 0.0f)
		{
			const float Progress = ((TotalTime - Time) / TotalTime) * Weight;
			DesiredInputArray[StartIndex] = FMath::Lerp(CurrentPoseArray[StartIndex], DesiredInputArray[StartIndex], Progress);
			DesiredInputArray[StartIndex+1] = FMath::Lerp(CurrentPoseArray[StartIndex+1], DesiredInputArray[StartIndex+1], Progress);
			DesiredInputArray[StartIndex+2] = FMath::Lerp(CurrentPoseArray[StartIndex+2], DesiredInputArray[StartIndex+2], Progress);
			DesiredInputArray[StartIndex+3] = FMath::Lerp(CurrentPoseArray[StartIndex+3], DesiredInputArray[StartIndex+3], Progress);
		}
	}
}

bool UMatchFeature_Trajectory2D::NextPoseToleranceTest(const TArray<float>& DesiredInputArray, const TArray<float>& PoseMatrix,
	const int32 MatrixStartIndex, const int32 FeatureOffset, const float PositionTolerance, const float RotationTolerance)
{
	for(int32 i = 0; i < TrajectoryTiming.Num(); ++i)
	{
		const int32 PointIndex = FeatureOffset + i * 4;
		const int32 MatrixIndex = MatrixStartIndex + i * 4;
		
		const float PredictionTime = TrajectoryTiming[i];
		
		const float RelativeTolerance_Pos = PredictionTime * PositionTolerance;
		const float RelativeTolerance_Rot = PredictionTime * RotationTolerance;
		
		float SqrDistance = FMath::Abs(DesiredInputArray[PointIndex] - PoseMatrix[MatrixIndex]);
		SqrDistance += FMath::Abs(DesiredInputArray[PointIndex+1] - PoseMatrix[MatrixIndex+1]);

		if(SqrDistance > RelativeTolerance_Pos * RelativeTolerance_Pos)
		{
			return false;
		}

		SqrDistance = FMath::Abs(DesiredInputArray[PointIndex + 2] - PoseMatrix[MatrixIndex + 2]);
		SqrDistance += FMath::Abs(DesiredInputArray[PointIndex + 3] - PoseMatrix[MatrixIndex + 3]);

		if(SqrDistance > RelativeTolerance_Rot * RelativeTolerance_Rot)
		{
			return false;
		}
	}

	return true;
}

#if WITH_EDITOR

void UMatchFeature_Trajectory2D::DrawPoseDebugEditor(UMotionDataAsset* MotionData, UDebugSkelMeshComponent* DebugSkeletalMesh,
                                                     const int32 PreviewIndex, const int32 FeatureOffset, const UWorld* World,
                                                     FPrimitiveDrawInterface* DrawInterface)
                                                     
{
	if(!MotionData || !DebugSkeletalMesh || !World)
	{
		return;
	}
	
	TArray<float>& PoseArray = MotionData->PoseMatrix.PoseArray;
	const int32 StartIndex = PreviewIndex * MotionData->PoseMatrix.AtomCount + FeatureOffset;
	
	if(PoseArray.Num() < StartIndex + Size())
	{
		return;
	}

	const FTransform PreviewTransform = DebugSkeletalMesh->GetComponentTransform();
	
	FVector LastPointPos = PreviewTransform.TransformPosition(FVector(PoseArray[StartIndex], PoseArray[StartIndex+1], 0.0f));
	
	const FQuat FacingOffset(FVector::UpVector, FMath::DegreesToRadians(
		FMotionMatchingUtils::GetFacingAngleOffset(MotionData->MotionMatchConfig->ForwardAxis)));

	for(int32 i = 0; i < TrajectoryTiming.Num(); ++i)
	{
		const int32 PointIndex = StartIndex + i*4;
		
		FVector RawPointPos(PoseArray[PointIndex], PoseArray[PointIndex + 1], 0.0f);
		FVector PointPos = PreviewTransform.TransformPosition(RawPointPos);

		DrawDebugSphere(World, PointPos, 5.0f, 8, FColor::Red, true, -1, 0);

		DrawInterface->DrawLine(LastPointPos, PointPos, FLinearColor::Red,
			ESceneDepthPriorityGroup::SDPG_Foreground, 3.0f);

		FVector RawArrowVector(PoseArray[PointIndex + 2], PoseArray[PointIndex + 3], 0.0f);
		FVector ArrowVector = PreviewTransform.TransformVector(FacingOffset * RawArrowVector * 30.0f);

		DrawDebugDirectionalArrow(World, PointPos, PointPos + ArrowVector, 20.0f, FColor::Red,
			true, -1, 0, 1.5f);

		LastPointPos = PointPos;
	}
}

void UMatchFeature_Trajectory2D::DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy,
	FMotionMatchingInputData& InputData, const int32 FeatureOffset, UMotionMatchConfig* MMConfig)
{
	const FTransform& MeshTransform = AnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	const FVector ActorLocation = MeshTransform.GetLocation();

	TArray<float>& InputArray = InputData.DesiredInputArray;
	
	const FQuat FacingOffset(FVector::UpVector, FMath::DegreesToRadians(
		FMotionMatchingUtils::GetFacingAngleOffset(MMConfig->ForwardAxis)));

	FColor Color;
	FVector LastPoint;
	for(int32 i = 0; i < TrajectoryTiming.Num(); ++i)
	{
		const int32 StartIndex = FeatureOffset + i * 4;
		
		if(StartIndex + 3 >= InputArray.Num()) //safety
		{
			break;
		}

		if(TrajectoryTiming[i] < 0.0f)
		{
			Color = FColor(0, 128, 0);
		}
		else
		{
			Color = FColor::Green;
		}
		
		FVector PointPosition = MeshTransform.TransformPosition(FVector(InputArray[StartIndex], InputArray[StartIndex+1], 0.0f));
		AnimInstanceProxy->AnimDrawDebugSphere(PointPosition, 5.0f, 32, Color, false, -1.0f, 0.0f);
		
		FVector DrawTo = PointPosition + MeshTransform.TransformVector(FacingOffset *
			FVector(InputArray[StartIndex+2], InputArray[StartIndex+3], 0.0f) * 30.0f);
		
		AnimInstanceProxy->AnimDrawDebugDirectionalArrow(PointPosition, DrawTo, 40.0f, Color,
			false, -1.0f, 2.0f);

		if(i > 0) 
		{
			if(TrajectoryTiming[i - 1] < 0.0f && TrajectoryTiming[i] > 0.0f)
			{
				AnimInstanceProxy->AnimDrawDebugLine(LastPoint, ActorLocation, FColor::Blue, false, -1.0f, 2.0f);
				AnimInstanceProxy->AnimDrawDebugLine(ActorLocation, PointPosition, FColor::Blue, false, -1.0f, 2.0f);
				AnimInstanceProxy->AnimDrawDebugSphere(ActorLocation, 5.0f, 32, FColor::Blue, false, -1.0f);
			}
			else
			{
				AnimInstanceProxy->AnimDrawDebugLine(LastPoint, PointPosition, Color, false, -1.0f, 2.0f);
			}
		}

		LastPoint = PointPosition;
	}
}

void UMatchFeature_Trajectory2D::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy,
                                                         UMotionDataAsset* MotionData, TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
{
	if(!MotionData || TrajectoryTiming.Num() == 0)
	{
		return;
	}

	const UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;
	
	const FTransform& MeshTransform = AnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	const FVector ActorLocation = MeshTransform.GetLocation();
	FVector LastPoint;

	const float FacingOffset = FMotionMatchingUtils::GetFacingAngleOffset(MMConfig->ForwardAxis);

	for(int32 i = 0; i < TrajectoryTiming.Num(); ++i)
	{
		const int32 StartIndex = FeatureOffset + i * 4;
		
		if(StartIndex + 3 >= CurrentPoseArray.Num()) //safety
		{
			break;
		}
		
		FColor Color = FColor::Red;
		if(TrajectoryTiming[i] < 0.0f)
		{
			Color = FColor(128,0,0);
		}

		FVector PointPosition = MeshTransform.TransformPosition(FVector(CurrentPoseArray[StartIndex], CurrentPoseArray[StartIndex+1], 0.0f));
		AnimInstanceProxy->AnimDrawDebugSphere(PointPosition, 5.0f, 32, Color, false, -1.0f, 0.0f);
		
		FVector DrawTo = PointPosition + MeshTransform.TransformVector(FacingOffset *
			FVector(CurrentPoseArray[StartIndex+2], CurrentPoseArray[StartIndex+3], 0.0f) * 30.0f);
		
		AnimInstanceProxy->AnimDrawDebugDirectionalArrow(PointPosition, DrawTo, 40.0f, Color,
			false, -1.0f, 2.0f);

		if(i > 0) 
		{
			if(TrajectoryTiming[i - 1] < 0.0f && TrajectoryTiming[i] > 0.0f)
			{
				AnimInstanceProxy->AnimDrawDebugLine(LastPoint, ActorLocation, FColor::Orange, false, -1.0f, 2.0f);
				AnimInstanceProxy->AnimDrawDebugLine(ActorLocation, PointPosition, FColor::Orange, false, -1.0f, 2.0f);
				AnimInstanceProxy->AnimDrawDebugSphere(ActorLocation, 5.0f, 32, FColor::Orange, false, -1.0f);
			}
			else
			{
				AnimInstanceProxy->AnimDrawDebugLine(LastPoint, PointPosition, Color, false, -1.0f, 2.0f);
			}
		}

		LastPoint = PointPosition;
	}
}

#endif