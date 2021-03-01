// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "MotionPreProcessToolkit.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/Commands/UIAction.h"
#include "EditorStyleSet.h"
#include "Widgets/Docking/SDockTab.h"
#include "IDetailsView.h"
#include "Controls/MotionSequenceTimelineCommands.h"
#include "MotionPreProcessorToolkitCommands.h"
#include "GUI/Widgets/SAnimList.h"
#include "GUI/Dialogs/AddNewAnimDialog.h"
#include "GUI/Dialogs/BonePickerDialog.h"
#include "GUI/Dialogs/SkeletonPickerDialog.h"
#include "Misc/MessageDialog.h"
#include "AnimPreviewInstance.h"
#include "SScrubControlPanel.h"
#include "../GUI/Widgets/SMotionTimeline.h"
#include "MotionSymphonyEditor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "CustomAssets/MotionAnimMetaDataWrapper.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessEditor"

const FName MotionPreProcessorAppName = FName(TEXT("MotionPreProcessorEditorApp"));

struct FMotionPreProcessorEditorTabs
{
	static const FName DetailsID;
	static const FName ViewportID;
	static const FName AnimationsID;
	static const FName AnimationDetailsID;
};

const FName FMotionPreProcessorEditorTabs::DetailsID(TEXT("Details"));
const FName FMotionPreProcessorEditorTabs::ViewportID(TEXT("Viewport"));
const FName FMotionPreProcessorEditorTabs::AnimationsID(TEXT("Animations"));
const FName FMotionPreProcessorEditorTabs::AnimationDetailsID(TEXT("Animation Details"));

FMotionPreProcessToolkit::~FMotionPreProcessToolkit()
{
	DetailsView.Reset();
	AnimDetailsView.Reset();
}

void FMotionPreProcessToolkit::Initialize(class UMotionDataAsset* InPreProcessAsset, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost> InToolkitHost)
{
	ActiveMotionDataAsset = InPreProcessAsset;

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	if(AssetEditorSubsystem)
	{
		AssetEditorSubsystem->CloseOtherEditors(InPreProcessAsset, this);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MotionPreProcessToolkit: Failed to find AssetEditorSubsystem."))
	}


	CurrentAnimIndex = INDEX_NONE;
	CurrentAnimType = EMotionAnimAssetType::None;
	PreviewPoseStartIndex = INDEX_NONE;
	PreviewPoseEndIndex = INDEX_NONE;
	PreviewPoseCurrentIndex = INDEX_NONE;

	//Create the details panel
	const bool bIsUpdateable = false;
	const bool bAllowFavorites = true;
	const bool bIsLockable = false;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	const FDetailsViewArgs DetailsViewArgs(bIsUpdateable, bIsLockable, true, FDetailsViewArgs::ObjectsUseNameArea, false);
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	AnimDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	//Setup toolkit commands
	FMotionSequenceTimelineCommands::Register();
	FMotionPreProcessToolkitCommands::Register();
	BindCommands();

	//Register UndoRedo
	ActiveMotionDataAsset->SetFlags(RF_Transactional);
	GEditor->RegisterForUndo(this);

	//Setup Layout for editor toolkit
	TSharedPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr = SharedThis(this);
	ViewportPtr = SNew(SMotionPreProcessToolkitViewport, MotionPreProcessToolkitPtr)
		.MotionDataBeingEdited(this, &FMotionPreProcessToolkit::GetActiveMotionDataAsset);

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_MotionPreProcessorAssetEditor")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(1.0f)
					->Split
					(
						FTabManager::NewStack()
							->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
							->SetHideTabWell(true)
							->SetSizeCoefficient(0.1f)
					)
					->Split
					(
						FTabManager::NewSplitter()
							->SetOrientation(Orient_Horizontal)
							->SetSizeCoefficient(0.9f)
							->Split
							(
								FTabManager::NewSplitter()
								->SetOrientation(Orient_Vertical)
								->SetSizeCoefficient(1.0f)
								->Split
								(
									FTabManager::NewStack()
									->AddTab(FMotionPreProcessorEditorTabs::AnimationsID, ETabState::OpenedTab)
									->SetHideTabWell(true)
									->SetSizeCoefficient(0.6f)
								)
								->Split
								(
									FTabManager::NewStack()
									->AddTab(FMotionPreProcessorEditorTabs::AnimationDetailsID, ETabState::OpenedTab)
									->SetHideTabWell(true)
									->SetSizeCoefficient(0.4f)
								)
							)
							->Split
							(
								FTabManager::NewStack()
								->AddTab(FMotionPreProcessorEditorTabs::ViewportID, ETabState::OpenedTab)
								->SetHideTabWell(true)
								->SetSizeCoefficient(0.6f)
							)
							->Split
							(
								FTabManager::NewStack()
								->AddTab(FMotionPreProcessorEditorTabs::DetailsID, ETabState::OpenedTab)
								->SetHideTabWell(true)
								->SetSizeCoefficient(0.2f)
							)
					)
			)
	);


	FAssetEditorToolkit::InitAssetEditor(
		InMode,
		InToolkitHost,
		MotionPreProcessorAppName,
		Layout,
		true,
		true,
		InPreProcessAsset
	);

	if (DetailsView.IsValid())
	{
		DetailsView->SetObject((UObject*)ActiveMotionDataAsset);
	}

	ExtendMenu();
	ExtendToolbar();
	RegenerateMenusAndToolbars();

}

FString FMotionPreProcessToolkit::GetDocumentationLink() const
{
	return FString();
}

void FMotionPreProcessToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_MotionPreProcessorAsset", "MotionPreProcessEditor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FMotionPreProcessorEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FMotionPreProcessToolkit::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(FMotionPreProcessorEditorTabs::AnimationsID, FOnSpawnTab::CreateSP(this, &FMotionPreProcessToolkit::SpawnTab_Animations))
		.SetDisplayName(LOCTEXT("AnimationsTab", "Animations"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));

	InTabManager->RegisterTabSpawner(FMotionPreProcessorEditorTabs::AnimationDetailsID, FOnSpawnTab::CreateSP(this, &FMotionPreProcessToolkit::SpawnTab_AnimationDetails))
		.SetDisplayName(LOCTEXT("AnimDetailsTab", "Animation Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));

	InTabManager->RegisterTabSpawner(FMotionPreProcessorEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FMotionPreProcessToolkit::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

}

void FMotionPreProcessToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FMotionPreProcessorEditorTabs::ViewportID);
	InTabManager->UnregisterTabSpawner(FMotionPreProcessorEditorTabs::DetailsID);
	InTabManager->UnregisterTabSpawner(FMotionPreProcessorEditorTabs::AnimationsID);
	InTabManager->UnregisterTabSpawner(FMotionPreProcessorEditorTabs::AnimationDetailsID);
}

FText FMotionPreProcessToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("MotionPreProcessorToolkitAppLabel", "MotionPreProcessor Toolkit");
}

FName FMotionPreProcessToolkit::GetToolkitFName() const
{
	return FName("MotionPreProcessorToolkit");
}

FText FMotionPreProcessToolkit::GetToolkitName() const
{
	const bool bDirtyState = ActiveMotionDataAsset->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("MotionPreProcessName"), FText::FromString(ActiveMotionDataAsset->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("MotionpreProcessorToolkitName", "{MotionPreProcessorName}{DirtyState}"), Args);
}

FText FMotionPreProcessToolkit::GetToolkitToolTipText() const
{
	return LOCTEXT("Tooltip", "Motion PreProcessor Editor");
}

FLinearColor FMotionPreProcessToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor();
}

FString FMotionPreProcessToolkit::GetWorldCentricTabPrefix() const
{
	return FString();
}

void FMotionPreProcessToolkit::AddReferencedObjects(FReferenceCollector & Collector)
{
}

void FMotionPreProcessToolkit::PostUndo(bool bSuccess)
{
}

void FMotionPreProcessToolkit::PostRedo(bool bSuccess)
{
}

TSharedRef<SDockTab> FMotionPreProcessToolkit::SpawnTab_Viewport(const FSpawnTabArgs & Args)
{
	ViewInputMin = 0.0f;
	ViewInputMax = GetTotalSequenceLength();
	LastObservedSequenceLength = ViewInputMax;

	TSharedPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr = SharedThis(this);

	TSharedRef<FUICommandList> LocalToolkitCommands = GetToolkitCommands();
	MotionTimelinePtr = SNew(SMotionTimeline, LocalToolkitCommands, TWeakPtr<FMotionPreProcessToolkit>(MotionPreProcessToolkitPtr));

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
				[
					ViewportPtr.ToSharedRef()
				]
			+SVerticalBox::Slot()
				.Padding(0, 8, 0, 0)
				.AutoHeight()
				.HAlign(HAlign_Fill)
				[
					MotionTimelinePtr.ToSharedRef()
				]
		];
}

TSharedRef<SDockTab> FMotionPreProcessToolkit::SpawnTab_Details(const FSpawnTabArgs & Args)
{
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		.TabColorScale(GetTabColorScale())
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMotionPreProcessToolkit::SpawnTab_Animations(const FSpawnTabArgs& Args)
{
	TSharedPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr = SharedThis(this);
	AnimationListPtr = SNew(SAnimList, MotionPreProcessToolkitPtr);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("AnimationsTab_Title", "Animations"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(0.5f)
			[
				SNew(SBorder)
				[
					AnimationListPtr.ToSharedRef()
				]
			]
		];
}

TSharedRef<SDockTab> FMotionPreProcessToolkit::SpawnTab_AnimationDetails(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		.TabColorScale(GetTabColorScale())
		[
			AnimDetailsView.ToSharedRef()
		];
}

void FMotionPreProcessToolkit::BindCommands()
{
	const FMotionPreProcessToolkitCommands& Commands = FMotionPreProcessToolkitCommands::Get();

	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();

	UICommandList->MapAction(Commands.PickAnims,
		FExecuteAction::CreateSP(this, &FMotionPreProcessToolkit::OpenPickAnimsDialog));
	UICommandList->MapAction(Commands.PickBones,
		FExecuteAction::CreateSP(this, &FMotionPreProcessToolkit::OpenPickBonesDialog));
	UICommandList->MapAction(Commands.PreProcess,
		FExecuteAction::CreateSP(this, &FMotionPreProcessToolkit::PreProcessAnimData));
	
}

void FMotionPreProcessToolkit::ExtendMenu()
{
}

void FMotionPreProcessToolkit::ExtendToolbar()
{
	struct local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("Commands");
			{
				ToolbarBuilder.AddToolBarButton(FMotionPreProcessToolkitCommands::Get().PickAnims);
				ToolbarBuilder.AddToolBarButton(FMotionPreProcessToolkitCommands::Get().PickBones);
				ToolbarBuilder.AddToolBarButton(FMotionPreProcessToolkitCommands::Get().PreProcess);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension
	(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&local::FillToolbar)
	);

	AddToolbarExtender(ToolbarExtender);

	FMotionSymphonyEditorModule* MotionSymphonyEditorModule = &FModuleManager::LoadModuleChecked<FMotionSymphonyEditorModule>("MotionSymphonyEditor");
	AddToolbarExtender(MotionSymphonyEditorModule->GetPreProcessToolkitToolbarExtensibilityManager()->GetAllExtenders());
}

FReply FMotionPreProcessToolkit::OnClick_Forward()
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();
	UAnimPreviewInstance* previewInstance = previewSkeletonMeshComponent->PreviewInstance;

	if (!previewSkeletonMeshComponent || !previewInstance)
		return FReply::Handled();

	const bool bIsReverse = previewInstance->IsReverse();
	const bool bIsPlaying = previewInstance->IsPlaying();

	if (bIsPlaying)
	{
		if (bIsReverse)
		{
			previewInstance->SetReverse(false);
		}
		else
		{
			previewInstance->StopAnim();
		}
	}
	else
	{
		previewInstance->SetPlaying(true);
	}

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Forward_Step()
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!previewSkeletonMeshComponent)
		return FReply::Handled();

	previewSkeletonMeshComponent->PreviewInstance->StopAnim();
	SetCurrentFrame(GetCurrentFrame() + 1);

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Forward_End()
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!previewSkeletonMeshComponent)
		return FReply::Handled();

	previewSkeletonMeshComponent->PreviewInstance->StopAnim();
	previewSkeletonMeshComponent->PreviewInstance->SetPosition(GetTotalSequenceLength(), false);

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Backward()
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();
	UAnimPreviewInstance* previewInstance = previewSkeletonMeshComponent->PreviewInstance;

	if (!previewSkeletonMeshComponent || !previewInstance)
		return FReply::Handled();

	const bool bIsReverse = previewInstance->IsReverse();
	const bool bIsPlaying = previewInstance->IsPlaying();

	if (bIsPlaying)
	{
		if (bIsReverse)
		{
			previewInstance->StopAnim();
		}
		else
		{
			previewInstance->SetReverse(true);
		}
	}
	else
	{
		if (!bIsReverse)
			previewInstance->SetReverse(true);

		previewInstance->SetPlaying(true);
	}

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Backward_Step()
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!previewSkeletonMeshComponent)
		return FReply::Handled();

	previewSkeletonMeshComponent->PreviewInstance->StopAnim();
	SetCurrentFrame(GetCurrentFrame() - 1);

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Backward_End()
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!previewSkeletonMeshComponent)
		return FReply::Handled();

	previewSkeletonMeshComponent->PreviewInstance->StopAnim();
	previewSkeletonMeshComponent->PreviewInstance->SetPosition(0.0f, false);

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_ToggleLoop()
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!previewSkeletonMeshComponent)
		return FReply::Handled();

	previewSkeletonMeshComponent->PreviewInstance->SetLooping(
		previewSkeletonMeshComponent->PreviewInstance->IsLooping());

	return FReply::Handled();
}

uint32 FMotionPreProcessToolkit::GetTotalFrameCount() const
{
	if (GetCurrentAnimation())
		return GetCurrentAnimation()->GetNumberOfFrames();

	return 0;
}

uint32 FMotionPreProcessToolkit::GetTotalFrameCountPlusOne() const
{
	return GetTotalFrameCount() + 1;
}

float FMotionPreProcessToolkit::GetTotalSequenceLength() const
{
	UAnimSequence* currentAnim = GetCurrentAnimation();

	if (GetCurrentAnimation())
	{
		return GetCurrentAnimation()->GetPlayLength();
	}

	return 0.0f;
}

float FMotionPreProcessToolkit::GetPlaybackPosition() const
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (previewSkeletonMeshComponent)
	{
		return previewSkeletonMeshComponent->PreviewInstance->GetCurrentTime();
	}

	return 0.0f;
}

void FMotionPreProcessToolkit::SetPlaybackPosition(float NewTime)
{
	UDebugSkelMeshComponent*  previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	FindCurrentPose(NewTime);

	//TODO: Check if we are previewing individual poses and if so lock step the NewTime to the pose time
	if (previewSkeletonMeshComponent)
	{
		NewTime = FMath::Clamp<float>(NewTime, 0.0f, GetTotalSequenceLength());
		previewSkeletonMeshComponent->PreviewInstance->SetPosition(NewTime, false);
	}
}

void FMotionPreProcessToolkit::FindCurrentPose(float Time)
{
	//Set the current preview pose if preprocessed
	if (ActiveMotionDataAsset->bIsProcessed
		&& PreviewPoseCurrentIndex != INDEX_NONE
		&& PreviewPoseEndIndex != INDEX_NONE)
	{
		PreviewPoseCurrentIndex = PreviewPoseStartIndex + FMath::RoundToInt(Time / ActiveMotionDataAsset->PoseInterval);
		PreviewPoseCurrentIndex = FMath::Clamp(PreviewPoseCurrentIndex, PreviewPoseStartIndex, PreviewPoseEndIndex);
	}
	else
	{
		PreviewPoseCurrentIndex = INDEX_NONE;
	}
}

bool FMotionPreProcessToolkit::IsLooping() const
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (previewSkeletonMeshComponent)
	{
		return previewSkeletonMeshComponent->PreviewInstance->IsLooping();
	}

	return false;
}

EPlaybackMode::Type FMotionPreProcessToolkit::GetPlaybackMode() const
{
	UDebugSkelMeshComponent* previewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (previewSkeletonMeshComponent)
	{
		if (previewSkeletonMeshComponent->PreviewInstance->IsPlaying())
		{
			return previewSkeletonMeshComponent->PreviewInstance->IsReverse() ? 
				EPlaybackMode::PlayingReverse : EPlaybackMode::PlayingForward;
		}
	}

	return EPlaybackMode::Stopped;
}

float FMotionPreProcessToolkit::GetViewRangeMin() const
{
	return ViewInputMin;
}

float FMotionPreProcessToolkit::GetViewRangeMax() const
{
	const float SequenceLength = GetTotalSequenceLength();
	if (SequenceLength != LastObservedSequenceLength)
	{
		LastObservedSequenceLength = SequenceLength;
		ViewInputMin = 0.0f;
		ViewInputMax = SequenceLength;
	}

	return ViewInputMax;
}

void FMotionPreProcessToolkit::SetViewRange(float NewMin, float NewMax)
{
	ViewInputMin = FMath::Max<float>(NewMin, 0.0f);
	ViewInputMax = FMath::Min<float>(NewMax, GetTotalSequenceLength());
}

UMotionDataAsset* FMotionPreProcessToolkit::GetActiveMotionDataAsset() const
{
	return ActiveMotionDataAsset;
}

FText FMotionPreProcessToolkit::GetAnimationName(const int32 AnimIndex)
{
	if(ActiveMotionDataAsset->GetSourceAnimCount() > 0)
	{
		FMotionAnimSequence& MotionAnim = ActiveMotionDataAsset->GetEditableSourceAnimAtIndex(AnimIndex);

		if (MotionAnim.Sequence)
		{
			return FText::AsCultureInvariant(MotionAnim.Sequence->GetName());
		}
	}

	return LOCTEXT("NullAnimation", "Null Animation");
}

FText FMotionPreProcessToolkit::GetBlendSpaceName(const int32 BlendSpaceIndex)
{
	if(ActiveMotionDataAsset->GetSourceBlendSpaceCount() > 0)
	{
		FMotionBlendSpace& MotionBlendSpace = ActiveMotionDataAsset->GetEditableSourceBlendSpaceAtIndex(BlendSpaceIndex);

		if (MotionBlendSpace.BlendSpace)
		{
			return FText::AsCultureInvariant(MotionBlendSpace.BlendSpace->GetName());
		}
	}

	return LOCTEXT("NullAnimation", "Null Animation");
}

void FMotionPreProcessToolkit::SetCurrentAnimation(const int32 AnimIndex)
{
	if (IsValidAnim(AnimIndex) && (AnimIndex != CurrentAnimIndex || CurrentAnimType != EMotionAnimAssetType::Sequence) 
		&& ActiveMotionDataAsset->SetAnimMetaPreviewIndex(EMotionAnimAssetType::Sequence, AnimIndex))
	{
		CurrentAnimIndex = AnimIndex;
		CurrentAnimType = EMotionAnimAssetType::Sequence;

		//Determine the range of pose indexes for previewing with this selected animation
		if (ActiveMotionDataAsset->bIsProcessed)
		{
			int32 PoseCount = ActiveMotionDataAsset->Poses.Num();
			for (int32 i = 0; i < PoseCount; ++i)
			{
				FPoseMotionData& StartPose = ActiveMotionDataAsset->Poses[i];

				if (StartPose.AnimId == CurrentAnimIndex
					&& StartPose.AnimType == CurrentAnimType)
				{
					PreviewPoseStartIndex = i;
					PreviewPoseCurrentIndex = i;

					for (int32 k = i; k < PoseCount; ++k)
					{
						FPoseMotionData& EndPose = ActiveMotionDataAsset->Poses[k];

						if (EndPose.AnimId != CurrentAnimIndex
							&& EndPose.AnimType == CurrentAnimType)
						{
							PreviewPoseEndIndex = k - 1;
							break;
						}
					}

					break;
				}
			}
		}
		else
		{
			PreviewPoseCurrentIndex = INDEX_NONE;
			PreviewPoseEndIndex = INDEX_NONE;
			PreviewPoseStartIndex = INDEX_NONE;
		}

		SetPreviewAnimation(ActiveMotionDataAsset->GetEditableSourceAnimAtIndex(AnimIndex));
		CacheTrajectory();

		//Set the anim meta data as the AnimDetailsViewObject
		if (AnimDetailsView.IsValid())
		{
			AnimDetailsView->SetObject(ActiveMotionDataAsset->MotionMetaWrapper);
		}
	}
	else
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		PreviewPoseCurrentIndex = INDEX_NONE;
		PreviewPoseEndIndex = INDEX_NONE;
		PreviewPoseStartIndex = INDEX_NONE;
		SetPreviewAnimationNull();

		if (AnimDetailsView.IsValid())
			AnimDetailsView->SetObject(nullptr);
	}
}

void FMotionPreProcessToolkit::SetCurrentBlendSpace(const int32 BlendSpaceIndex)
{
	if (IsValidBlendSpace(BlendSpaceIndex) && (BlendSpaceIndex != CurrentAnimIndex || CurrentAnimType != EMotionAnimAssetType::BlendSpace)
		&& ActiveMotionDataAsset->SetAnimMetaPreviewIndex(EMotionAnimAssetType::BlendSpace, BlendSpaceIndex))
	{
		CurrentAnimIndex = BlendSpaceIndex;
		CurrentAnimType = EMotionAnimAssetType::BlendSpace;

		//Determine the range of pose indexes for previewing with this selected animation
		if (ActiveMotionDataAsset->bIsProcessed)
		{
			int32 PoseCount = ActiveMotionDataAsset->Poses.Num();
			for (int32 i = 0; i < PoseCount; ++i)
			{
				FPoseMotionData& StartPose = ActiveMotionDataAsset->Poses[i];

				if (StartPose.AnimId == CurrentAnimIndex 
					&& StartPose.AnimType == CurrentAnimType)
				{
					PreviewPoseStartIndex = i;
					PreviewPoseCurrentIndex = i;

					for (int32 k = i; k < PoseCount; ++k)
					{
						FPoseMotionData& EndPose =  ActiveMotionDataAsset->Poses[k];

						if (EndPose.AnimId != CurrentAnimIndex 
							&& EndPose.AnimType == CurrentAnimType)
						{
							PreviewPoseEndIndex = k - 1;
							break;
						}
					}

					break;
				}
			}
		}
		else
		{
			PreviewPoseCurrentIndex = INDEX_NONE;
			PreviewPoseEndIndex = INDEX_NONE;
			PreviewPoseStartIndex = INDEX_NONE;
		}

		//SetPreviewAnimation(ActiveMotionDataAsset->GetEditableSourceBlendSpaceAtIndex(BlendSpaceIndex));
		CacheTrajectory();

		//Set the anim meta data as the AnimDetailsViewObject
		if (AnimDetailsView.IsValid())
		{
			AnimDetailsView->SetObject(ActiveMotionDataAsset->MotionMetaWrapper);
		}
	}
	else
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		PreviewPoseCurrentIndex = INDEX_NONE;
		PreviewPoseEndIndex = INDEX_NONE;
		PreviewPoseStartIndex = INDEX_NONE;
		SetPreviewAnimationNull();

		if (AnimDetailsView.IsValid())
			AnimDetailsView->SetObject(nullptr);
	}

	//RebuildTagTimelines();
}

UAnimSequence* FMotionPreProcessToolkit::GetCurrentAnimation() const
{
	if (!ActiveMotionDataAsset)
		return nullptr;

	if (ActiveMotionDataAsset->IsValidSourceAnimIndex(CurrentAnimIndex))
	{
		FMotionAnimSequence& MotionAnim = ActiveMotionDataAsset->GetEditableSourceAnimAtIndex(CurrentAnimIndex);

		UAnimSequence* currentAnim = ActiveMotionDataAsset->GetEditableSourceAnimAtIndex(CurrentAnimIndex).Sequence;
		if (currentAnim)
		{
			check(currentAnim);
			return(currentAnim);
		}
	}

	return nullptr;
}

void FMotionPreProcessToolkit::DeleteAnimSequence(const int32 AnimIndex)
{
	if (AnimIndex == CurrentAnimIndex)
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		SetPreviewAnimationNull();
		AnimDetailsView->SetObject(nullptr, true);
	}

	ActiveMotionDataAsset->DeleteSourceAnim(AnimIndex);
	AnimationListPtr.Get()->Rebuild();

	if (ActiveMotionDataAsset->GetSourceAnimCount() == 0)
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		SetPreviewAnimationNull();
		AnimDetailsView->SetObject(nullptr, true);
	}
}

void FMotionPreProcessToolkit::DeleteBlendSpace(const int32 BlendSpaceIndex)
{
	if (BlendSpaceIndex == CurrentAnimIndex)
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		SetPreviewAnimationNull();
		AnimDetailsView->SetObject(nullptr, true);
	}

	ActiveMotionDataAsset->DeleteSourceBlendSpace(BlendSpaceIndex);
	AnimationListPtr.Get()->Rebuild();

	if (ActiveMotionDataAsset->GetSourceBlendSpaceCount() == 0)
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		SetPreviewAnimationNull();
		AnimDetailsView->SetObject(nullptr, true);
	}
}

void FMotionPreProcessToolkit::ClearAnimList()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Clear Source Anim List",
		"Are you sure you want to remove all animations from the anim list?"))
		== EAppReturnType::No)
	{
		return;
	}

	CurrentAnimIndex = INDEX_NONE;
	CurrentAnimType = EMotionAnimAssetType::None;
	SetPreviewAnimationNull();

	AnimDetailsView->SetObject(nullptr, true);

	ActiveMotionDataAsset->ClearSourceAnims();
	AnimationListPtr.Get()->Rebuild();
}

void FMotionPreProcessToolkit::ClearBlendSpaceList()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Clear Source Anim List",
		"Are you sure you want to remove all animations from the anim list?"))
		== EAppReturnType::No)
	{
		return;
	}

	AnimDetailsView->SetObject(nullptr, true);

	ActiveMotionDataAsset->ClearSourceBlendSpaces();
	AnimationListPtr.Get()->Rebuild();
}

void FMotionPreProcessToolkit::AddNewAnimSequences(TArray<UAnimSequence*> SequenceList)
{
	for (int32 i = 0; i < SequenceList.Num(); ++i)
	{
		UAnimSequence* AnimSequence = SequenceList[i];

		if (AnimSequence)
		{
			ActiveMotionDataAsset->AddSourceAnim(AnimSequence);
		}
	}

	AnimationListPtr.Get()->Rebuild();
	//RebuildTagTimelines();
}

void FMotionPreProcessToolkit::AddNewBlendSpaces(TArray<UBlendSpaceBase*> BlendSpaceList)
{
	for (int32 i = 0; i < BlendSpaceList.Num(); ++i)
	{
		UBlendSpaceBase* BlendSpace = BlendSpaceList[i];

		if (BlendSpace)
		{
			ActiveMotionDataAsset->AddSourceBlendSpace(BlendSpace);
		}
	}

	AnimationListPtr.Get()->Rebuild();
}

FString FMotionPreProcessToolkit::GetSkeletonName()
{
	return ActiveMotionDataAsset->GetSkeleton()->GetName();
}

USkeleton* FMotionPreProcessToolkit::GetSkeleton()
{
	return ActiveMotionDataAsset->GetSkeleton();
}

void FMotionPreProcessToolkit::SetSkeleton(USkeleton* Skeleton)
{
	ActiveMotionDataAsset->SetSkeleton(Skeleton);
}

void FMotionPreProcessToolkit::ClearMatchBones()
{
	//ActiveMotionDataAsset->PoseJoints.Empty();
}

void FMotionPreProcessToolkit::AddMatchBone(const int32 BoneIndex)
{
	//ActiveMotionDataAsset->PoseJoints.Add(BoneIndex);
}

bool FMotionPreProcessToolkit::AnimationAlreadyAdded(const FName SequenceName)
{
	const int32 SequenceCount = ActiveMotionDataAsset->GetSourceAnimCount();

	for (int32 i = 0; i < SequenceCount; ++i)
	{
		FMotionAnimSequence& MotionAnim = ActiveMotionDataAsset->GetEditableSourceAnimAtIndex(i);

		if (MotionAnim.Sequence != nullptr && MotionAnim.Sequence->GetFName() == SequenceName)
		{
			return true;
		}
	}

	const int32 BlendSpaceCount = ActiveMotionDataAsset->GetSourceBlendSpaceCount();

	for(int32 i = 0; i < BlendSpaceCount; ++i)
	{
		FMotionBlendSpace& MotionBlendSpace = ActiveMotionDataAsset->GetEditableSourceBlendSpaceAtIndex(i);

		if (MotionBlendSpace.BlendSpace != nullptr && MotionBlendSpace.BlendSpace->GetFName() == SequenceName)
		{
			return true;
		}
	}

	return false;
}

bool FMotionPreProcessToolkit::IsValidAnim(const int32 AnimIndex)
{
	if (ActiveMotionDataAsset->IsValidSourceAnimIndex(AnimIndex))
	{
		if (ActiveMotionDataAsset->GetSourceAnimAtIndex(AnimIndex).Sequence)
		{
			return true;
		}
	}

	return false;
}

bool FMotionPreProcessToolkit::IsValidBlendSpace(const int32 BlendSpaceIndex)
{
	if (ActiveMotionDataAsset->IsValidSourceBlendSpaceIndex(BlendSpaceIndex))
	{
		if (ActiveMotionDataAsset->GetSourceBlendSpaceAtIndex(BlendSpaceIndex).BlendSpace)
		{
			return true;
		}
	}

	return false;
}

bool FMotionPreProcessToolkit::SetPreviewAnimation(FMotionAnimSequence& MotionAnimSequence) const
{
	UDebugSkelMeshComponent* DebugMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!DebugMeshComponent || !DebugMeshComponent->SkeletalMesh)
		return false;
	
	UAnimSequence* AnimSequence = MotionAnimSequence.Sequence;
	MotionTimelinePtr->SetAnimation(&MotionAnimSequence, DebugMeshComponent);
	
	if(AnimSequence)
	{
		if (AnimSequence->GetSkeleton() == DebugMeshComponent->SkeletalMesh->Skeleton)
		{
			DebugMeshComponent->EnablePreview(true, AnimSequence);
			DebugMeshComponent->SetAnimation(AnimSequence);
			return true;
		}
		else
		{
			DebugMeshComponent->EnablePreview(true, nullptr);
			return false;
		}
	}
	else
	{
		SetPreviewAnimationNull();
		return true;
	}

	return false;
}



void FMotionPreProcessToolkit::SetPreviewAnimationNull() const
{
	UDebugSkelMeshComponent* DebugMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!DebugMeshComponent || !DebugMeshComponent->SkeletalMesh)
		return;

	DebugMeshComponent->EnablePreview(true, nullptr);
}

UDebugSkelMeshComponent* FMotionPreProcessToolkit::GetPreviewSkeletonMeshComponent() const
{
	UDebugSkelMeshComponent* PreviewComponent = ViewportPtr->GetPreviewComponent();
	check(PreviewComponent);

	return PreviewComponent;
}

bool FMotionPreProcessToolkit::SetPreviewComponentSkeletalMesh(USkeletalMesh* SkeletalMesh) const
{
	UDebugSkelMeshComponent* previewSkelMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!previewSkelMeshComponent)
		return false;

	if (SkeletalMesh)
	{
		USkeletalMesh* previewSkelMesh = previewSkelMeshComponent->SkeletalMesh;

		if (previewSkelMesh)
		{
			if (previewSkelMesh->Skeleton != SkeletalMesh->Skeleton)
			{
				SetPreviewAnimationNull();
				previewSkelMeshComponent->SetSkeletalMesh(SkeletalMesh, true);
				ViewportPtr->OnFocusViewportToSelection();
				return false;
			}
			else
			{
				previewSkelMeshComponent->SetSkeletalMesh(SkeletalMesh, false);
				ViewportPtr->OnFocusViewportToSelection();
				return true;
			}
		}
		
		SetPreviewAnimationNull();

		previewSkelMeshComponent->SetSkeletalMesh(SkeletalMesh, true);
		ViewportPtr->OnFocusViewportToSelection();
	}
	else
	{
		SetPreviewAnimationNull();
		previewSkelMeshComponent->SetSkeletalMesh(nullptr, true);
	}

	return false;
}

void FMotionPreProcessToolkit::SetCurrentFrame(int32 NewIndex)
{
	const int32 TotalLengthInFrames = GetTotalFrameCount();
	int32 ClampedIndex = FMath::Clamp<int32>(NewIndex, 0, TotalLengthInFrames);
	SetPlaybackPosition(ClampedIndex / GetFramesPerSecond());
}

int32 FMotionPreProcessToolkit::GetCurrentFrame() const
{
	const int32 TotalLengthInFrames = GetTotalFrameCount();

	if (TotalLengthInFrames == 0)
	{
		return INDEX_NONE;
	}
	else
	{
		return FMath::Clamp<int32>((int32)(GetPlaybackPosition() 
			* GetFramesPerSecond()), 0, TotalLengthInFrames);
	}

}

float FMotionPreProcessToolkit::GetFramesPerSecond() const
{
	return 30.0f;
}

void FMotionPreProcessToolkit::PreProcessAnimData()
{
	if (!ActiveMotionDataAsset)
		return;

	if (!CheckValidForPreProcess())
		return;


	ActiveMotionDataAsset->Modify();
	ActiveMotionDataAsset->PreProcess();
	ActiveMotionDataAsset->MarkPackageDirty();

}

void FMotionPreProcessToolkit::OpenPickAnimsDialog()
{
	if (!ActiveMotionDataAsset)
		return;

	if (ActiveMotionDataAsset->GetSkeleton() == nullptr)
	{
		if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Cannot Add Anims With Null Skeleton",
			"No Skeleton has been selected for this Motion Data Asset. Do you want to set one now?"))
			== EAppReturnType::Yes)
		{
			SSkeletonPickerDialog::OnOpenFollowUpWindow.BindSP(this, &FMotionPreProcessToolkit::OpenPickAnimsDialog);
			SSkeletonPickerDialog::ShowWindow(AnimationListPtr->MotionPreProcessToolkitPtr.Pin());
		}

		return;
	}

	SAddNewAnimDialog::ShowWindow(AnimationListPtr->MotionPreProcessToolkitPtr.Pin());
}


void FMotionPreProcessToolkit::OpenPickBonesDialog()
{
	if (ActiveMotionDataAsset->GetSkeleton() == nullptr)
	{
		if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Cannot Pick Bones With Null Skeleton",
			"No Skeleton has been selected for this Motion Data Asset. Do you want to set one now?"))
			== EAppReturnType::Yes)
		{
			SSkeletonPickerDialog::OnOpenFollowUpWindow.BindSP(this, &FMotionPreProcessToolkit::OpenPickBonesDialog);
			SSkeletonPickerDialog::ShowWindow(AnimationListPtr->MotionPreProcessToolkitPtr.Pin());
		}
		
		return;
	}

	SBonePickerDialog::ShowWindow(AnimationListPtr->MotionPreProcessToolkitPtr.Pin());
}

bool FMotionPreProcessToolkit::CheckValidForPreProcess()
{
	bool valid = true;

	UMotionMatchConfig* MMConfig = ActiveMotionDataAsset->MotionMatchConfig;

	if (!MMConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("MotionData PreProcess Validity Check Failed: Missing MotionMatchConfig reference."));
		return false;
	}

	//Check that there is a Skeleton set
	if (!MMConfig->GetSkeleton())
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: Skeleton not set on MotionMatchConfig asset"));
		valid = false;
	}
	
	//Check that there are pose joints set to match
	if (MMConfig->PoseBones.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: No pose bones set. Check your MotionMatchConfig asset"));
		valid = false;
	}

	//Check that there are trajectory points set
	if (MMConfig->TrajectoryTimes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: No trajectory times set"));
		valid = false;
	}

	//Check that there is at least one animation to pre-process
	int32 SourceAnimCount = ActiveMotionDataAsset->GetSourceAnimCount() + ActiveMotionDataAsset->GetSourceBlendSpaceCount();
	if (SourceAnimCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: No animations added"));
		valid = false;
	}

	//Check that there is at least one trajectory point in the future
	float highestTrajPoint = -1.0f;
	for (int32 i = 0; i < MMConfig->TrajectoryTimes.Num(); ++i)
	{
		float timeValue = MMConfig->TrajectoryTimes[i];

		if (timeValue > 0.0f)
		{
			highestTrajPoint = timeValue;
			break;
		}
	}

	if (highestTrajPoint < 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: Must have at least one trajectory point set in the future"));
		valid = false;
	}

	if (!valid)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Invalid Motion Data", 
			"The current setup of the motion data asset is not valid for pre-processing. Please see the output log for more details."));
	}

	return valid;
}

void FMotionPreProcessToolkit::CacheTrajectory()
{
	if (!ActiveMotionDataAsset)
		return;

	UAnimSequence* curAnim = GetCurrentAnimation();

	if (!curAnim)
		return;

	CachedTrajectoryPoints.Empty(FMath::CeilToInt(curAnim->SequenceLength / 0.1f));

	//Step through the animation 0.1s at a time and record a trajectory point
	CachedTrajectoryPoints.Add(FVector(0.0f));
	for (float time = 0.1f; time < curAnim->SequenceLength; time += 0.1f)
	{
		CachedTrajectoryPoints.Add(curAnim->ExtractRootMotion(0.0f, time, false).GetLocation());
	}
}


void FMotionPreProcessToolkit::DrawCachedTrajectoryPoints(FPrimitiveDrawInterface* DrawInterface) const
{
	FVector lastPoint = FVector(0.0f);
	
	FLinearColor color = FLinearColor::Green;
	for (auto& point : CachedTrajectoryPoints)
	{
		DrawInterface->DrawLine(lastPoint, point, color, ESceneDepthPriorityGroup::SDPG_Foreground, 3.0f);
		lastPoint = point;
	}
}

bool FMotionPreProcessToolkit::GetPendingTimelineRebuild()
{
	return PendingTimelineRebuild;
}

void FMotionPreProcessToolkit::SetPendingTimelineRebuild(const bool IsPendingRebuild)
{
	PendingTimelineRebuild = IsPendingRebuild;
}

void FMotionPreProcessToolkit::HandleTagsSelected(const TArray<UObject*>& SelectedObjects)
{
	//Set the anim meta data as the AnimDetailsViewObject
	if (AnimDetailsView.IsValid())
	{
		AnimDetailsView->SetObjects(SelectedObjects);
		//AnimDetailsView->SetCustomFilterLabel(LOCTEXT("AnimNotify", "Anim Notify"));
	}
	
}

#undef LOCTEXT_NAMESPACE