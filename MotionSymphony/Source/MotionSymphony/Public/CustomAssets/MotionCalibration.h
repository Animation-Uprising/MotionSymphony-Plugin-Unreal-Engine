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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	TArray<float> CalibrationArray;

private:
	bool bIsInitialized;

public:
	UMotionCalibration(const FObjectInitializer& ObjectInitializer);

	void Initialize();
	void ValidateData();
	bool IsSetupValid(UMotionMatchConfig* InMotionMatchConfig);

	virtual void Serialize(FArchive& Ar) override;

	UFUNCTION(BlueprintNativeEvent, Category = "MotionSymphony|CostFunctions")
	void OnGenerateWeightings();
	void OnGenerateWeightings_Implementation();


#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
};