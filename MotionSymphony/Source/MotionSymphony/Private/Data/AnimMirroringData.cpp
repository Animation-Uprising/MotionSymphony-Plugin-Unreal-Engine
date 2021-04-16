// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimMirroringData.h"
#include "BoneContainer.h"

void FAnimMirroringData::Initialize(const UMirroringProfile* MirroringProfile, const USkeletalMeshComponent* SkelMesh)
{
	if (MirroringProfile == nullptr || SkelMesh== nullptr)
		return;

	IndexedMirrorPairs.Empty(MirroringProfile->MirrorPairs.Num() + 1);

	//Then add all bone pairs from the mirroring profile asset
	for (int32 i = 0; i < MirroringProfile->MirrorPairs.Num(); ++i)
	{
		AddPair(MirroringProfile->MirrorPairs[i], SkelMesh);
	}
}

void FAnimMirroringData::Initialize(const TArray<FBoneMirrorPair>& OverrideMirrorPairs,
	const UMirroringProfile* MirroringProfile, const USkeletalMeshComponent* SkelMesh)
{
	IndexedMirrorPairs.Empty(( MirroringProfile ? MirroringProfile->MirrorPairs.Num() : 0) + OverrideMirrorPairs.Num() + 1);

	//First add all override bone pairs
	for (int32 i = 0; i < OverrideMirrorPairs.Num(); ++i)
	{
		AddPair(OverrideMirrorPairs[i], SkelMesh);
	}

	if (!MirroringProfile)
		return;

	//Then add all bone pairs from the mirroring profile asset
	for (int32 i = 0; i < MirroringProfile->MirrorPairs.Num(); ++i)
	{
		AddPair(MirroringProfile->MirrorPairs[i], SkelMesh);
	}
}

void FAnimMirroringData::AddPair(const FBoneMirrorPair& MirrorPair, const USkeletalMeshComponent* SkelMesh)
{
	if(SkelMesh == nullptr)
		return;

	int32 BoneIndexA = SkelMesh->GetBoneIndex(FName(MirrorPair.BoneName));
	int32 BoneIndexB = SkelMesh->GetBoneIndex(FName(MirrorPair.MirrorBoneName));

	IndexedMirrorPairs.Emplace(FIndexedMirrorPair(BoneIndexA, BoneIndexB));
}

const FIndexedMirrorPair& FAnimMirroringData::FindPair(const int32 PairIndex) const
{
	if(PairIndex < 0 && PairIndex >= IndexedMirrorPairs.Num())
		return NullMirrorPair;

	return IndexedMirrorPairs[PairIndex];
}

int32 FAnimMirroringData::FindMirrorBone(const int32 BoneIndex) const
{
	for (const FIndexedMirrorPair& MirrorPair : IndexedMirrorPairs)
	{
		if(MirrorPair.BoneIndex == BoneIndex)
			return MirrorPair.MirrorBoneIndex;

		if(MirrorPair.MirrorBoneIndex == BoneIndex)
			return MirrorPair.BoneIndex;
	}

	return -1;
}

bool FAnimMirroringData::IsMatchBoneName(const FString& BoneName, const FString MatchStr, EMirrorMatchingRule MatchRule)
{
	switch (MatchRule)
	{
		case EMirrorMatchingRule::HeadMatch: return BoneName.StartsWith(MatchStr, ESearchCase::IgnoreCase);
		case EMirrorMatchingRule::TailMatch: return BoneName.EndsWith(MatchStr, ESearchCase::IgnoreCase);
		case EMirrorMatchingRule::WordMatch: return BoneName.Contains(MatchStr, ESearchCase::IgnoreCase);
	}

	checkNoEntry();

	return false;
}

FString FAnimMirroringData::GetPairBoneName(const FString& BoneName, const FString& SourceMatchStr, 
	const FString& TargetMatchStr, EMirrorMatchingRule MatchRule)
{
	switch (MatchRule)
	{
		case EMirrorMatchingRule::HeadMatch: return BoneName.RightChop(SourceMatchStr.Len());
		case EMirrorMatchingRule::TailMatch: return BoneName.LeftChop(SourceMatchStr.Len()) + TargetMatchStr;
		case EMirrorMatchingRule::WordMatch: return BoneName.Replace(*SourceMatchStr, *TargetMatchStr);
	}

	checkNoEntry();

	return FString();
}

FIndexedMirrorPair::FIndexedMirrorPair()
	: BoneIndex(-1),
	MirrorBoneIndex(-1)
{
}

FIndexedMirrorPair::FIndexedMirrorPair(int32 InBoneIndex, int32 InMirrorBoneIndex)
	: BoneIndex(InBoneIndex),
	MirrorBoneIndex(InMirrorBoneIndex)
{
}
