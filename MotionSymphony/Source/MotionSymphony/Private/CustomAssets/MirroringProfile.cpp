// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.


#include "CustomAssets/MirroringProfile.h"

#define LOCTEXT_NAMESPACE "MirroringProfile"

FBoneMirrorPair::FBoneMirrorPair()
	: BoneName(),
	  MirrorBoneName(),
	  MirrorAxis(EAxis::Y),
	  FlipAxis(EAxis::X),
	  bHasMirrorBone(false),
	  bMirrorPosition(false),
	  RotationOffset(FRotator::ZeroRotator)
{
}

FBoneMirrorPair::FBoneMirrorPair(const FString& InBoneName, const EAxis::Type InMirrorAxis, const EAxis::Type InFlipAxis)
	: BoneName(InBoneName),
	  MirrorBoneName(InBoneName),
	  MirrorAxis(InMirrorAxis),
	  FlipAxis(InFlipAxis),
	  bHasMirrorBone(false),
	  bMirrorPosition(false),
	  RotationOffset(FRotator::ZeroRotator)
{
}

FBoneMirrorPair::FBoneMirrorPair(const FString& InBoneName, const FString& InMirrorBoneName, 
	const EAxis::Type InMirrorAxis, const EAxis::Type InFlipAxis)
	: BoneName(InBoneName),
	  MirrorBoneName(InMirrorBoneName),
	  MirrorAxis(InMirrorAxis),
	  FlipAxis(InFlipAxis),
	  bHasMirrorBone(true),
	  bMirrorPosition(false),
	  RotationOffset(FRotator::ZeroRotator)
{
}

UMirroringProfile::UMirroringProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	SourceSkeleton(nullptr),
	MirrorAxis_Default(EAxis::Y),
	FlipAxis_Default(EAxis::X),
	RotationOffset_Default(FRotator::ZeroRotator),
	bMirrorPosition_Default(false),
	LeftAffix("_l"),
	RightAffix("_r")
{
}

void UMirroringProfile::AutoMap()
{
	if (SourceSkeleton == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot auto map mirroring profile with a null skeleton"));
		return;
	}

	const FReferenceSkeleton& RefSkeleton = SourceSkeleton->GetReferenceSkeleton();
	int32 BoneCount = RefSkeleton.GetNum();
	MirrorPairs.Empty(BoneCount + 1);
	TArray<FString> BoneStrings;

	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		FString BoneStr = RefSkeleton.GetBoneName(BoneIndex).ToString();

		int32 LeftAffixPosition = BoneStr.Find(LeftAffix, ESearchCase::IgnoreCase, ESearchDir::FromEnd, -1);
		int32 RightAffixPosition = BoneStr.Find(RightAffix, ESearchCase::IgnoreCase, ESearchDir::FromEnd, -1);

		if (LeftAffixPosition != -1 || RightAffixPosition != -1)
		{
			FString MirrorBoneStr;
			bool ValidMirror = true;
			if (RightAffixPosition != -1)
			{
				MirrorBoneStr = BoneStr.Mid(0, RightAffixPosition);
				MirrorBoneStr += LeftAffix;
				MirrorBoneStr += BoneStr.Mid(RightAffixPosition + RightAffix.Len());

				if (FMath::Abs(MirrorBoneStr.Len() - LeftAffix.Len()) != FMath::Abs(BoneStr.Len() - LeftAffix.Len()))
				{
					ValidMirror = false;
				}
			}
			else
			{
				MirrorBoneStr = BoneStr.Mid(0, LeftAffixPosition);
				MirrorBoneStr += RightAffix;
				MirrorBoneStr += BoneStr.Mid(LeftAffixPosition + LeftAffix.Len());

				if (FMath::Abs(MirrorBoneStr.Len() - RightAffix.Len()) != FMath::Abs(BoneStr.Len() - RightAffix.Len()))
				{
					ValidMirror = false;
				}
			}

			if (ValidMirror)
			{
				FName MirrorBoneName = FName(*MirrorBoneStr);
				int32 MirrorBoneIndex = RefSkeleton.FindBoneIndex(FName(*MirrorBoneStr));

				if (!BoneStrings.Contains(MirrorBoneStr) && !BoneStrings.Contains(BoneStr))
				{
					FBoneMirrorPair NewMirrorPair = FBoneMirrorPair(BoneStr, MirrorBoneStr, EAxis::None, EAxis::None);
					NewMirrorPair.bMirrorPosition = bMirrorPosition_Default;
					NewMirrorPair.RotationOffset = RotationOffset_Default;

					MirrorPairs.Add(NewMirrorPair);
					BoneStrings.Add(MirrorBoneStr);
					BoneStrings.Add(BoneStr);
					continue;
				}
			}
		}

		if (!BoneStrings.Contains(BoneStr))
		{
			FBoneMirrorPair NewMirrorPair = FBoneMirrorPair(BoneStr, MirrorAxis_Default, FlipAxis_Default);
			NewMirrorPair.RotationOffset = RotationOffset_Default;
			NewMirrorPair.bMirrorPosition = bMirrorPosition_Default;

			MirrorPairs.Add(NewMirrorPair);
			BoneStrings.Add(BoneStr);
		}
	}
}

FName UMirroringProfile::FindBoneMirror(FName BoneName)
{
	FString BoneStr = BoneName.ToString();

	for (FBoneMirrorPair& MirrorPair : MirrorPairs)
	{
		if (MirrorPair.BoneName == BoneStr)
		{
			if (MirrorPair.bHasMirrorBone)
			{
				return FName(MirrorPair.MirrorBoneName);
			}
			else
			{
				return BoneName;
			}
		}
		else if (MirrorPair.MirrorBoneName == BoneStr)
		{
			if (MirrorPair.bHasMirrorBone)
			{
				return FName(MirrorPair.BoneName);
			}
			else
			{
				return BoneName;
			}
		}
	}

	return BoneName;
}

USkeleton* UMirroringProfile::GetSourceSkeleton()
{
	return SourceSkeleton;
}

void UMirroringProfile::SetSourceSkeleton(USkeleton* skeleton)
{
	Modify();
	SourceSkeleton = skeleton;
	MarkPackageDirty();
}

bool UMirroringProfile::IsSetupValid()
{
	if (!SourceSkeleton)
	{
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

