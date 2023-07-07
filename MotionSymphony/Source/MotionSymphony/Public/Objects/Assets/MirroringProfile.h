// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoneContainer.h"
#include "MirroringProfile.generated.h"

class USkeleton;

/** Enumeration for defining mirror bone matching rules*/
UENUM(BlueprintType)
enum class EMirrorMatchingRule : uint8
{
	Never,
	Always,
	ExactMatch,
	HeadMatch,
	TailMatch,
	WordMatch
};

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FBoneMirrorPair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|RuntimeMirroring")
	FString BoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|RuntimeMirroring")
	FString MirrorBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|RuntimeMirroring")
	TEnumAsByte<EAxis::Type> MirrorAxis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|RuntimeMirroring")
	TEnumAsByte<EAxis::Type> FlipAxis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|RuntimeMirroring")
	bool bHasMirrorBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|RuntimeMirroring")
	bool bMirrorPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|RuntimeMirroring")
	FRotator RotationOffset;

public:
	FBoneMirrorPair();
	FBoneMirrorPair(const FString& InBoneName, const EAxis::Type InMirrorAxis, const EAxis::Type InFlipAxis);

	FBoneMirrorPair(const FString& InBoneName, const FString& InMirrorBoneName,  
		const EAxis::Type InMirrorAxis, const EAxis::Type InFlipAxis);
};

UCLASS()
class MOTIONSYMPHONY_API UMirroringProfile : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	USkeleton* SourceSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FVector CharacterMirrorAxis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	bool bMirrorPosition_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	FString LeftAffix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	FString RightAffix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BonePairs)
	TArray<FBoneMirrorPair> MirrorPairs;
	
public:
	UMirroringProfile(const FObjectInitializer& ObjectInitializer);

	void AutoMap();
	FName FindBoneMirror(FName BoneName);

	TEnumAsByte<EAxis::Type> GetMirrorAxis(int32 BoneIndex);
	TEnumAsByte<EAxis::Type> GetFlipAxis(int32 BoneIndex);

	USkeleton* GetSourceSkeleton();
	void SetSourceSkeleton(USkeleton* skeleton);

	bool IsSetupValid();
};
