// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Data/CalibrationData.h"
#include "Data/MotionTraitField.h"
#include "CustomAssets/MotionDataAsset.h"
#include "CustomAssets/MotionMatchConfig.h"

FCalibrationData::FCalibrationData()
{
	Weights.Empty(30);
}

FCalibrationData::FCalibrationData(UMotionDataAsset* SourceMotionData)
{
	if(!SourceMotionData)
	{
		Weights.Empty(30);
		return;
	}

	UMotionMatchConfig* MMConfig = SourceMotionData->MotionMatchConfig;

	if(!MMConfig)
	{
		return;
	}
	
	Weights.SetNum(MMConfig->TotalDimensionCount);
}

FCalibrationData::FCalibrationData(UMotionMatchConfig* SourceConfig)
{
	Initialize(SourceConfig);
}

FCalibrationData::FCalibrationData(int32 AtomCount)
{
	Weights.SetNum(FMath::Max(0, AtomCount));
}

void FCalibrationData::Initialize(UMotionMatchConfig* SourceConfig)
{
	if (SourceConfig == nullptr)
	{
		Weights.Empty(30);
		return;
	}

	Weights.SetNum(SourceConfig->TotalDimensionCount);
}

bool FCalibrationData::IsValidWithConfig(UMotionMatchConfig* MotionConfig)
{
	if (!MotionConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("FCalibrationData: Trying to call IsValidWithConfig(UMotionMatchConfig*) but the motion config passed is null"));
		return false;
	}
	
	if(Weights.Num() != MotionConfig->TotalDimensionCount)
	{
		UE_LOG(LogTemp, Error, TEXT("FCalibrationData: Calibration Data is invalid as the weight array does not match the motion config pose array"));
		return false;
	}

	return true;
}

void FCalibrationData::GenerateStandardDeviationWeights(const UMotionDataAsset* SourceMotionData, const FMotionTraitField& MotionTrait)
{
	if (!SourceMotionData || !SourceMotionData->MotionMatchConfig)
	{
		return;
	}

	UMotionMatchConfig* MMConfig = SourceMotionData->MotionMatchConfig;
	Initialize(MMConfig);
	
	int32 SDPoseCount = 0;

	//Determine the total for each atom
	const int32 AtomCount = SourceMotionData->PoseMatrix.AtomCount;
	TArray<float> TotalsArray;
	TotalsArray.SetNumZeroed(AtomCount);
	const TArray<float>& PoseArray = SourceMotionData->PoseMatrix.PoseArray;
	for(const FPoseMotionData& Pose : SourceMotionData->Poses)
	{
		if(Pose.bDoNotUse || Pose.Traits != MotionTrait)
		{
			continue;
		}

		++SDPoseCount;
		const int32 PoseStartIndex = Pose.PoseId * AtomCount;
		for(int32 i = 0; i < AtomCount; ++i)
		{
			TotalsArray[i] += PoseArray[PoseStartIndex + i];
		}
	}
	
	SDPoseCount = FMath::Max(SDPoseCount, 1);

	//Determine the Mean for each atom
	TArray<float> MeanArray;
	MeanArray.SetNumZeroed(AtomCount);
	for(int32 i = 0; i < AtomCount; ++i)
	{
		MeanArray[i] = TotalsArray[i] / SDPoseCount;
	}

	//Determine the distance to the mean squared for each atom
	TArray<float> TotalDistanceSqrArray;
	TotalDistanceSqrArray.SetNumZeroed(AtomCount);
	for (const FPoseMotionData& Pose : SourceMotionData->Poses)
	{
		if (Pose.bDoNotUse || Pose.Traits != MotionTrait)
		{
			continue;
		}

		const int32 PoseStartIndex = Pose.PoseId * AtomCount;
		for(int32 i = 0; i < AtomCount; ++i)
		{
			const float DistanceToMean = PoseArray[PoseStartIndex + i] - MeanArray[i];
			TotalDistanceSqrArray[i] += DistanceToMean * DistanceToMean;
		}
	}

	//Calculate the standard deviation and final standard deviation weight
	for(int32 i = 0; i < AtomCount; ++i)
	{
		const float StandardDeviation = TotalDistanceSqrArray[i] / SDPoseCount;
		Weights[i] = FMath::IsNearlyZero(StandardDeviation)? 0.0f : 1.0f / StandardDeviation;
	}
}

void FCalibrationData::GenerateFinalWeights(const UMotionCalibration* SourceCalibration, const FCalibrationData& StdDeviationNormalizers)
{
	if (!SourceCalibration || !SourceCalibration->MotionMatchConfig)
	{
		return;
	}

	Initialize(SourceCalibration->MotionMatchConfig);
	
	const float ResponseMultiplier = SourceCalibration->QualityVsResponsivenessRatio * 2.0f;
	const float QualityMultiplier = (1.0f - SourceCalibration->QualityVsResponsivenessRatio) * 2.0f;

	int32 AtomIndex = 0;
	TArray<TObjectPtr<UMatchFeatureBase>>& Features = SourceCalibration->MotionMatchConfig->Features;
	for(TObjectPtr<UMatchFeatureBase> FeaturePtr : Features)
	{
		UMatchFeatureBase* Feature = FeaturePtr.Get();

		if(!Feature)
		{
			continue;
		}

		const int32 FeatureSize = Feature->Size();
		if(Feature->PoseCategory == EPoseCategory::Quality)
		{
			for(int32 i = 0; i < FeatureSize; ++i)
			{
				Weights[AtomIndex] = SourceCalibration->CalibrationArray[AtomIndex] * QualityMultiplier * StdDeviationNormalizers.Weights[AtomIndex];
				++AtomIndex;
			}
		}
		else
		{
			for(int32 i = 0; i < FeatureSize; ++i)
			{
				Weights[AtomIndex] = SourceCalibration->CalibrationArray[AtomIndex] * ResponseMultiplier * StdDeviationNormalizers.Weights[AtomIndex];
				++AtomIndex;
			}
		}
	}
}