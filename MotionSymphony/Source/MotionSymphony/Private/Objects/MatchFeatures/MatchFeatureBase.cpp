#include "Objects/MatchFeatures/MatchFeatureBase.h"
#include "MMPreProcessUtils.h"

UMatchFeatureBase::UMatchFeatureBase(const FObjectInitializer& ObjectInitializer)
	: DefaultWeight(1.0f)
{
}

void UMatchFeatureBase::Initialize()
{

}

bool UMatchFeatureBase::IsSetupValid() const
{
	return true;
}

bool UMatchFeatureBase::IsMotionSnapshotCompatible() const
{
	return false;
}

int32 UMatchFeatureBase::Size() const
{
	return 0;
}

void UMatchFeatureBase::EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence, const float Time,
                                           const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile)
{
}

void UMatchFeatureBase::EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite, const float Time,
                                           const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile)
{
}

void UMatchFeatureBase::EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace, const float Time,
                                           const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition)
{
}

void UMatchFeatureBase::CacheMotionBones(FAnimInstanceProxy* InAnimInstanceProxy)
{
}

void UMatchFeatureBase::ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float* FeatureCacheLocation, FAnimInstanceProxy*
                                       AnimInstanceProxy, float DeltaTime)
{
}

void UMatchFeatureBase::ApplyInputBlending(TArray<float>& DesiredInputArray,
                                           const TArray<float>& CurrentPoseArray, const int32 FeatureOffset, const float Weight)
{
}

bool UMatchFeatureBase::NextPoseToleranceTest(const TArray<float>& DesiredInputArray, const TArray<float>& PoseMatrix,
                                              const int32 MatrixStartIndex, const int32 FeatureOffset, const float PositionTolerance, const float RotationTolerance)
{
	return true;
}

float UMatchFeatureBase::GetDefaultWeight(int32 AtomId) const
{
	return DefaultWeight;
}

void UMatchFeatureBase::CalculateDistanceSqrToMeanArrayForStandardDeviations(TArray<float>& OutDistToMeanSqrArray,
	const TArray<float>& InMeanArray, const TArray<float>& InPoseArray, const int32 FeatureOffset, const int32 PoseStartIndex) const
{
	for(int32 i = FeatureOffset; i < FeatureOffset + Size(); ++i)
	{
		const float DistanceToMean = InPoseArray[PoseStartIndex + i] - InMeanArray[i];
		OutDistToMeanSqrArray[i] += DistanceToMean * DistanceToMean;
	}
}

#if WITH_EDITOR
void UMatchFeatureBase::DrawPoseDebugEditor(UMotionDataAsset* MotionData, UDebugSkelMeshComponent* DebugSkeletalMesh,
                                            const int32 PreviewIndex, const int32 FeatureOffset, const UWorld* World, FPrimitiveDrawInterface* DrawInterface)
{
}

void UMatchFeatureBase::DrawAnimDebugEditor()
{
}

void UMatchFeatureBase::DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy, FMotionMatchingInputData&,
	const int32 FeatureOffset, UMotionMatchConfig* MMConfig)
{
}

void UMatchFeatureBase::DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy, UMotionDataAsset* MotionData,
                                                TArray<float>& CurrentPoseArray, const int32 FeatureOffset)
{
}

#endif
