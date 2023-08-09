#pragma once

#include "CoreMinimal.h"
#include "Objects/MatchFeatures/MatchFeatureBase.h"
#include "MatchFeature_BodyMomentum3D.generated.h"

struct FPoseMatrix;

UCLASS(BlueprintType)
class MOTIONSYMPHONY_API UMatchFeature_BodyMomentum3D : public UMatchFeatureBase
{
	GENERATED_BODY()

public:
	virtual int32 Size() const override;
	
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable) override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable) override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable, const FVector2D BlendSpacePosition) override;

	virtual void ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float* FeatureCacheLocation, FAnimInstanceProxy*
								AnimInstanceProxy, float DeltaTime) override;

	virtual void CalculateDistanceSqrToMeanArrayForStandardDeviations(TArray<float>& OutDistToMeanSqrArray,
																	  const TArray<float>& InMeanArray, const TArray<float>& InPoseArray, const int32 FeatureOffset, const int32 PoseStartIndex) const override;

#if WITH_EDITOR	
	virtual void DrawPoseDebugEditor(UMotionDataAsset* MotionData, UDebugSkelMeshComponent* DebugSkeletalMesh,
		const int32 PreviewIndex, const int32 FeatureOffset, const UWorld* World, FPrimitiveDrawInterface* DrawInterface) override;
	virtual void DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy, UMotionDataAsset* MotionData,
		TArray<float>& CurrentPoseArray, const int32 FeatureOffset) override;
#endif
};