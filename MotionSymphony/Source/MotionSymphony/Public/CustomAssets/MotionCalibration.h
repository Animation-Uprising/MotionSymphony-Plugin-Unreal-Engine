// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomAssets/MotionMatchConfig.h"
#include "MotionCalibration.generated.h"

class UMotionDataAsset;

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FJointWeightSet
{
	GENERATED_USTRUCT_BODY();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration|Pose", meta = (ClampMin = 0))
	float Weight_Pos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration|Pose", meta = (ClampMin = 0))
	float Weight_Vel;

public:
	FJointWeightSet();
	FJointWeightSet(float InWeightPos, float InWeightVel);

	FJointWeightSet operator * (const FJointWeightSet& rhs);
};

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FTrajectoryWeightSet
{
	GENERATED_USTRUCT_BODY();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration|Trajectory", meta = (ClampMin = 0))
	float Weight_Pos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration|Trajectory", meta = (ClampMin = 0))
	float Weight_Facing;

public:
	FTrajectoryWeightSet();
	FTrajectoryWeightSet(float InWeightPos, float InWeightFacing);

	FTrajectoryWeightSet operator * (const FTrajectoryWeightSet& rhs);
};

UCLASS(Blueprintable, BlueprintType)
class MOTIONSYMPHONY_API UMotionCalibration : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	UMotionMatchConfig* MotionMatchConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	bool bOverrideDefaults;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Calibration", meta = (ClampMin = 0, ClampMax = 1))
	float QualityVsResponsivenessRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration", meta = (ClampMin = 0))
	float Weight_Momentum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration", meta = (ClampMin = 0))
	float Weight_AngularMomentum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration")
	TArray<FJointWeightSet> PoseJointWeights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration")
	TArray<FTrajectoryWeightSet> TrajectoryWeights;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults", meta = (ClampMin = 0))
	float JointPosition_DefaultWeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults", meta = (ClampMin = 0))
	float JointVelocity_DefaultWeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults", meta = (ClampMin = 0))
	float TrajectoryPosition_DefaultWeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults", meta = (ClampMin = 0))
	float TrajectoryFacing_DefaultWeight;

private:
	bool bIsInitialized;

public:
	UMotionCalibration(const FObjectInitializer& ObjectInitializer);

	void Initialize();
	void ValidateData();
	bool IsSetupValid(UMotionMatchConfig* InMotionMatchConfig);

	virtual void Serialize(FArchive& Ar) override;

	UFUNCTION(BlueprintNativeEvent, Category = "MotionSymphony|CostFunctions")
	void OnGenerateJointWeightings();
	void OnGenerateJointWeightings_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "MotionSymphony|CostFunctions")
	void OnGenerateTrajectoryWeightings();
	void OnGenerateTrajectoryWeightings_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "MotionSymphony|CostFunctions")
	void OnGeneratePoseWeightings();
	void OnGeneratePoseWeightings_Implementation();

#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	UFUNCTION(BlueprintCallable, Category = "MotionSymphony|Calibration")
	void SetBoneWeightSet(FName BoneName, float Weight_Pos, float Weight_Vel);

	UFUNCTION(BlueprintCallable, Category = "MotionSymphony|Calibration")
	void SetTrajectoryWeightSet(int32 Index, float Weight_Pos, float Weight_Facing);
};