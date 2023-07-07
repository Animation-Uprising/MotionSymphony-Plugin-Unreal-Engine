#pragma once

#include "CoreMinimal.h"
#include "MatchFeatureBase.h"
#include "MatchFeature_Trajectory2D.generated.h"


class UMotionDataAsset;
struct FPoseMatrix;

/** Motion matching feature which stores and matches bone velocity relative to the character root */
UCLASS(BlueprintType)
class MOTIONSYMPHONY_API UMatchFeature_Trajectory2D : public UMatchFeatureBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Match Feature")
	float DefaultDirectionWeighting;
	
	UPROPERTY(EditAnywhere, Category = "Match Feature|Trajectory")
	TArray<float> TrajectoryTiming;

public:
	UMatchFeature_Trajectory2D(const FObjectInitializer& ObjectInitializer);
	virtual bool IsSetupValid() const override;
	virtual int32 Size() const override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionAnimSequence& InSequence,
		const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile) override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionComposite& InComposite,
		const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile) override;
	virtual void EvaluatePreProcess(float* ResultLocation, FMotionBlendSpace& InBlendSpace,
		const float Time, const float PoseInterval, const bool bMirror, UMirroringProfile* MirrorProfile, const FVector2D BlendSpacePosition) override;

	virtual void ApplyInputBlending(TArray<float>& DesiredInputArray, const TArray<float>& CurrentPoseArray, const int32 FeatureOffset, const float Weight) override;
	virtual bool NextPoseToleranceTest(const TArray<float>& DesiredInputArray, const TArray<float>& PoseMatrix,
	                                   const int32 MatrixStartIndex, const int32 FeatureOffset, const float PositionTolerance, const float RotationTolerance) override;

	virtual float GetDefaultWeight(int32 AtomId) const override;

	virtual void CalculateDistanceSqrToMeanArrayForStandardDeviations(TArray<float>& OutDistToMeanSqrArray,
		const TArray<float>& InMeanArray, const TArray<float>& InPoseArray, const int32 FeatureOffset, const int32 PoseStartIndex) const override;

#if WITH_EDITOR
	virtual void DrawPoseDebugEditor(UMotionDataAsset* MotionData, UDebugSkelMeshComponent* DebugSkeletalMesh,
		const int32 PreviewIndex, const int32 FeatureOffset, const UWorld* World, FPrimitiveDrawInterface* DrawInterface) override;
	virtual void DrawDebugDesiredRuntime(FAnimInstanceProxy* AnimInstanceProxy, FMotionMatchingInputData& InputData,
		const int32 FeatureOffset, UMotionMatchConfig* MMConfig) override;
	virtual void DrawDebugCurrentRuntime(FAnimInstanceProxy* AnimInstanceProxy, UMotionDataAsset* MotionData,
		TArray<float>& CurrentPoseArray, const int32 FeatureOffset)  override;
#endif
};