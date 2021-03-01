// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CustomAssets/MotionDataAsset.h"
#include "MotionPreProcessToolkitViewport.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"
#include "EditorUndoClient.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "ITransportControl.h"
#include "MotionSymphony.h"

class FSpawnTabArgs;
class ISlateStyle;
class IToolkitHost;
class IDetailsView;
class SDockTab;
class UMotionDataAsset;
class SAnimList;
class SMotionTimeline;

class FMotionPreProcessToolkit
	: public FAssetEditorToolkit
	, public FEditorUndoClient
	, public FGCObject
{
public:
	FMotionPreProcessToolkit(){}
	virtual ~FMotionPreProcessToolkit();

public:
	int32 CurrentAnimIndex;
	EMotionAnimAssetType CurrentAnimType;

	int32 PreviewPoseStartIndex;
	int32 PreviewPoseEndIndex;
	int32 PreviewPoseCurrentIndex;
	
protected: 
	UMotionDataAsset* ActiveMotionDataAsset;
	TSharedPtr<SAnimList> AnimationListPtr;
	TSharedPtr<class SMotionPreProcessToolkitViewport> ViewportPtr;
	TSharedPtr<class SMotionTimeline> MotionTimelinePtr;

	mutable float ViewInputMin;
	mutable float ViewInputMax;
	mutable float LastObservedSequenceLength;

	TArray<FVector> CachedTrajectoryPoints;

	bool PendingTimelineRebuild = false;

public:
	void Initialize(UMotionDataAsset* InPreProcessAsset, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost> InToolkitHost );

	virtual FString GetDocumentationLink() const override;

	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	// End of IToolkit interface

	// FAssetEditorToolkit 
	virtual FText GetBaseToolkitName() const override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	// End of FAssetEditorToolkit

	// FSerializableObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FSerializableObject interface

	UMotionDataAsset* GetActiveMotionDataAsset() const;

	FText GetAnimationName(const int32 AnimIndex);
	FText GetBlendSpaceName(const int32 BlendSpaceIndex);
	void SetCurrentAnimation(const int32 AnimIndex);
	void SetCurrentBlendSpace(const int32 BlendSpaceIndex);
	UAnimSequence* GetCurrentAnimation() const;
	void DeleteAnimSequence(const int32 AnimIndex);
	void DeleteBlendSpace(const int32 BlenSpaceIndex);
	void ClearAnimList();
	void ClearBlendSpaceList();
	void AddNewAnimSequences(TArray<UAnimSequence*> FromSequences);
	void AddNewBlendSpaces(TArray<UBlendSpaceBase*> FromBlendSpaces);
	bool AnimationAlreadyAdded(const FName SequenceName);
	FString GetSkeletonName();
	USkeleton* GetSkeleton();
	void SetSkeleton(USkeleton* Skeleton);

	void ClearMatchBones();
	void AddMatchBone(const int32 BoneIndex);

	UDebugSkelMeshComponent* GetPreviewSkeletonMeshComponent() const;
	bool SetPreviewComponentSkeletalMesh(USkeletalMesh* SkeletalMesh) const;

	void SetCurrentFrame(int32 NewIndex);
	int32 GetCurrentFrame() const;
	float GetFramesPerSecond() const;

	void FindCurrentPose(float Time);

	void DrawCachedTrajectoryPoints(FPrimitiveDrawInterface* DrawInterface) const;

	bool GetPendingTimelineRebuild();
	void SetPendingTimelineRebuild(const bool IsPendingRebuild);

	void HandleTagsSelected(const TArray<UObject*>& SelectedTags);

protected:
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Animations(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_AnimationDetails(const FSpawnTabArgs& Args);

	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<class IDetailsView> AnimDetailsView;

	void BindCommands();
	void ExtendMenu();
	void ExtendToolbar();

	//Timeline callbacks
	FReply OnClick_Forward();
	FReply OnClick_Forward_Step();
	FReply OnClick_Forward_End();
	FReply OnClick_Backward();
	FReply OnClick_Backward_Step();
	FReply OnClick_Backward_End();
	FReply OnClick_ToggleLoop();

	uint32 GetTotalFrameCount() const;
	uint32 GetTotalFrameCountPlusOne() const;
	float GetTotalSequenceLength() const;
	float GetPlaybackPosition() const;
	void SetPlaybackPosition(float NewTime);
	bool IsLooping() const;
	EPlaybackMode::Type GetPlaybackMode() const;

	float GetViewRangeMin() const;
	float GetViewRangeMax() const;
	void SetViewRange(float NewMin, float NewMax);

private:
	bool IsValidAnim(const int32 AnimIndex);
	bool IsValidBlendSpace(const int32 BlendSpaceIndex);
	bool SetPreviewAnimation(FMotionAnimSequence& MotionSequence) const;
	void SetPreviewAnimationNull() const;

	void PreProcessAnimData();
	void OpenPickAnimsDialog();
	void OpenPickBonesDialog();

	bool CheckValidForPreProcess();

	void CacheTrajectory();
};