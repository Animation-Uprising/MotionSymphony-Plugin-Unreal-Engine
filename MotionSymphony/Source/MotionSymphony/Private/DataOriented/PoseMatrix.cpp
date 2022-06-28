// Fill out your copyright notice in the Description page of Project Settings.


#include "DataOriented/PoseMatrix.h"

#include "MotionDataAsset.h"

FPoseMatrix::FPoseMatrix(int32 InPoseCount, int32 InAtomCount)
{
	PoseArray.Empty(InPoseCount * InAtomCount + 1);
}

FPoseMatrix::FPoseMatrix()
	: PoseCount(0),
	AtomCount(0)
{
}

float& FPoseMatrix::GetAtom(int32 PoseId, int32 AtomId)
{
	return PoseArray[PoseId * AtomCount + AtomId];
}

const float& FPoseMatrix::GetAtom(int32 PoseId, int32 AtomId) const
{
	return PoseArray[PoseId * AtomCount + AtomId];
}
