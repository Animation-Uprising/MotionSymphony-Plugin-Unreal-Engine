//Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

#include "Objects/Assets/MotionCalibration.h"

#define LOCTEXT_NAMESPACE "MotionCalibration"

FJointWeightSet::FJointWeightSet()
	: Weight_Pos(1.0f),
	  Weight_Vel(1.0f)
{}

FJointWeightSet::FJointWeightSet(float InWeightPos, float InWeightVel)
	: Weight_Pos(InWeightPos),
	  Weight_Vel(InWeightVel)
{}

FJointWeightSet FJointWeightSet::operator*(const FJointWeightSet& rhs)
{
	return FJointWeightSet(	Weight_Pos * rhs.Weight_Pos,
							Weight_Vel * rhs.Weight_Vel);
}

FTrajectoryWeightSet::FTrajectoryWeightSet()
	: Weight_Pos(5.0f),
	  Weight_Facing(3.0f)
{}


FTrajectoryWeightSet::FTrajectoryWeightSet(float InWeightPos, float InWeightFacing)
	: Weight_Pos(InWeightPos),
	  Weight_Facing(InWeightFacing)
{}

FTrajectoryWeightSet FTrajectoryWeightSet::operator*(const FTrajectoryWeightSet& rhs)
{
	return FTrajectoryWeightSet(Weight_Pos * rhs.Weight_Pos,
								Weight_Facing * rhs.Weight_Facing);
}

UMotionCalibration::UMotionCalibration(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer),
	MotionMatchConfig(nullptr),
	bOverrideDefaults(false),
	QualityVsResponsivenessRatio(0.5f),
	bIsInitialized(false)
{}

void UMotionCalibration::Initialize()
{
	if (!MotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("Trying to initialize MotionCalibration but the MotionMatchConfig is null. Please ensure your Motion Calibration has a configuration set."));
		return;
	}

	if(bIsInitialized)
	{
		return;
	}
	
	ValidateData();
	OnGenerateWeightings(false);

	int32 AtomIndex = 0;
	const float QualityAdjustment = (1.0f - QualityVsResponsivenessRatio) * 2.0f;
	const float ResponsivenessAdjustment = QualityVsResponsivenessRatio * 2.0f;
	for(TObjectPtr<UMatchFeatureBase> MatchFeaturePtr : MotionMatchConfig->Features)
	{
		const UMatchFeatureBase* MatchFeature = MatchFeaturePtr.Get();
		
		if(MatchFeature->PoseCategory == EPoseCategory::Quality)
		{
			for(int32 i = 0; i < MatchFeature->Size(); ++i)
			{
				CalibrationArray[AtomIndex] *= QualityAdjustment;
				++AtomIndex;
			}
		}
		else
		{
			for(int32 i = 0; i < MatchFeature->Size(); ++i)
			{
				CalibrationArray[AtomIndex] *= ResponsivenessAdjustment;
				++AtomIndex;
			}
		}
	}
}

void UMotionCalibration::ValidateData()
{
	if (!MotionMatchConfig)
	{
		//UE_LOG(LogTemp, Error, TEXT("Motion Calibration validation failed. Motion matching config not set"));
		UE_LOG(LogTemp, Warning, TEXT("Motion Calibration validation failed. Motion matching config not set"));
		return;
	}

	//Ensure that the calibration array is the correct size
	const int32 ArraySize = MotionMatchConfig->TotalDimensionCount;
	if(ArraySize != CalibrationArray.Num())
	{
		CalibrationArray.SetNum(ArraySize); //Todo: Check that this doesn't set all values to 1.0
		Modify(true);
	}
	
}

bool UMotionCalibration::IsSetupValid(UMotionMatchConfig* InMotionMatchConfig)
{
	if (!MotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Calibration validity check failed. The MotionMatchConfig property has not been set."));
		return false;
	}
	else if (MotionMatchConfig != InMotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Calibration validity check failed. The MotionMatchConfig property does not match the config set on the MotionData Asset."));
		return false;
	}

	return true;
}

void UMotionCalibration::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	//ValidateData();
}

void UMotionCalibration::OnGenerateWeightings_Implementation(bool bIgnoreInput /*= false*/)
{
	if(bOverrideDefaults)
	{
		return;
	}

	int32 AtomIndex = 0;
	for(TObjectPtr<UMatchFeatureBase> MatchFeaturePtr : MotionMatchConfig->Features)
	{
		if(!MatchFeaturePtr)
		{
			continue;
		}

		if(bIgnoreInput && MatchFeaturePtr->PoseCategory == EPoseCategory::Responsiveness)
		{
			for(int32 i = 0; i < MatchFeaturePtr->Size(); ++i)
			{
				CalibrationArray[AtomIndex] = 0.0f; 
				++AtomIndex;
			}

			continue;
		}
		
		for(int32 i = 0; i < MatchFeaturePtr->Size(); ++i)
		{
			CalibrationArray[AtomIndex] = MatchFeaturePtr->GetDefaultWeight(i); 
			++AtomIndex;
		}
	}
}

#if WITH_EDITORONLY_DATA
void UMotionCalibration::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	ValidateData();
}
#endif

#undef LOCTEXT_NAMESPACE