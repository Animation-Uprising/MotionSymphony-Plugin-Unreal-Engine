#pragma once

#include "CoreMinimal.h"
#include "MatchFeatureBase.h"
#include "MatchFeature_BoneFacing.generated.h"


struct FPoseMatrix;

/** Motion matching feature which stores and matches bone location relative to the character root */
UCLASS(BlueprintType)
class MOTIONSYMPHONY_API UMatchFeature_BoneFacing : public UMatchFeatureBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Match Feature|Bone Facing")
	FBoneReference BoneReference;

public:
	virtual bool IsMotionSnapshotCompatible() const override;
	
	virtual int32 Size() const override;
	virtual void EvaluatePreProcess(float* ResultLocation, UAnimSequence* InSequence,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable, void* InUserData) override;
	virtual void EvaluatePreProcess(float* ResultLocation, UAnimComposite* InComposite,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable, void* InUserData) override;
	virtual void EvaluatePreProcess(float* ResultLocation, UBlendSpace* InBlendSpace,
	                                const float Time, const float PoseInterval, const bool bMirror, UMirrorDataTable* MirrorDataTable, const FVector2D BlendSpacePosition, void
	                                * InUserData) override;

	virtual void CacheMotionBones(FAnimInstanceProxy* InAnimInstanceProxy) override;
	virtual void ExtractRuntime(FCSPose<FCompactPose>& CSPose, float* ResultLocation, float* FeatureCacheLocation, FAnimInstanceProxy*
	                            AnimInstanceProxy, float DeltaTime) override;

	virtual void CalculateDistanceSqrToMeanArrayForStandardDeviations(TArray<float>& OutDistToMeanSqrArray,
	                                                                  const TArray<float>& InMeanArray, const TArray<float>& InPoseArray, const int32 FeatureOffset, const int32 PoseStartIndex) const override;

#if WITH_EDITOR
	virtual void DrawPoseDebugEditor(UMotionDataAsset* MotionData, UDebugSkelMeshComponent* DebugSkeletalMesh,
		const int32 PreviewIndex, const int32 FeatureOffset, const UWorld* World, FPrimitiveDrawInterface* DrawInterface) override;
	virtual void DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy, FMotionMatchingInputData& InputData,
		const int32 FeatureOffset, UMotionMatchConfig* MMConfig) override;
	virtual void DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy, UMotionDataAsset* MotionData,
		TArray<float>& CurrentPoseArray, const int32 FeatureOffset) override;
#endif
};
