// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

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
#include "Misc/MessageDialog.h"
#include "AnimPreviewInstance.h"
#include "AssetSelection.h"
#include "SScrubControlPanel.h"
#include "../GUI/Widgets/SMotionTimeline.h"
#include "MotionSymphonyEditor.h"
#include "TagPoint.h"
#include "TagSection.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Data/MotionAnimMetaDataWrapper.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessEditor"

const FName MotionPreProcessorAppName = FName(TEXT("MotionPreProcessorEditorApp"));

TArray<FMotionAnimSequence> FMotionPreProcessToolkit::CopiedSequences;
TArray<FMotionComposite> FMotionPreProcessToolkit::CopiedComposites;
TArray<FMotionBlendSpace> FMotionPreProcessToolkit::CopiedBlendSpaces;

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
	const bool bIsLockable = false;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;

	DetailsViewArgs.bUpdatesFromSelection = bIsUpdateable;
	DetailsViewArgs.bLockable = bIsLockable;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;
	DetailsViewArgs.bHideSelectionTip = false;
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
					//->Split
					//(
					//	FTabManager::NewStack()
					//		->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
					//		->SetHideTabWell(true)
					//		->SetSizeCoefficient(0.1f)
					//)
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
		DetailsView->SetObject(ActiveMotionDataAsset);
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
	const auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FMotionPreProcessorEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FMotionPreProcessToolkit::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
#else
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));
#endif

	InTabManager->RegisterTabSpawner(FMotionPreProcessorEditorTabs::AnimationsID, FOnSpawnTab::CreateSP(this, &FMotionPreProcessToolkit::SpawnTab_Animations))
		.SetDisplayName(LOCTEXT("AnimationsTab", "Animations"))
		.SetGroup(WorkspaceMenuCategoryRef)
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));
#else
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));
#endif

	InTabManager->RegisterTabSpawner(FMotionPreProcessorEditorTabs::AnimationDetailsID, FOnSpawnTab::CreateSP(this, &FMotionPreProcessToolkit::SpawnTab_AnimationDetails))
		.SetDisplayName(LOCTEXT("AnimDetailsTab", "Animation Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));
#else
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));
#endif

	InTabManager->RegisterTabSpawner(FMotionPreProcessorEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FMotionPreProcessToolkit::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
#else
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
#endif

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
	return FText::Format(LOCTEXT("MotionpreProcessorToolkitName", "{MotionPreProcessName}{DirtyState}"), Args);
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

	const TSharedPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr = SharedThis(this);

	TSharedRef<FUICommandList> LocalToolkitCommands = GetToolkitCommands();
	MotionTimelinePtr = SNew(SMotionTimeline, LocalToolkitCommands, TWeakPtr<FMotionPreProcessToolkit>(MotionPreProcessToolkitPtr));

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#else
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif
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
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#else
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		.TabColorScale(GetTabColorScale())
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMotionPreProcessToolkit::SpawnTab_Animations(const FSpawnTabArgs& Args)
{
	TSharedPtr<FMotionPreProcessToolkit> MotionPreProcessToolkitPtr = SharedThis(this);
	//AnimationListPtr = SNew(SAnimList, MotionPreProcessToolkitPtr);
	SAssignNew(AnimationTreePtr, SAnimTree, MotionPreProcessToolkitPtr);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#else
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif
		.Label(LOCTEXT("AnimationsTab_Title", "Animations"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(0.5f)
			[
				SNew(SBorder)
				[
					//AnimationListPtr.ToSharedRef()
					AnimationTreePtr.ToSharedRef()
				]
			]
		];
}

TSharedRef<SDockTab> FMotionPreProcessToolkit::SpawnTab_AnimationDetails(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#else
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif
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
	UICommandList->MapAction(Commands.ClearAnims,
		FExecuteAction::CreateSP(this, &FMotionPreProcessToolkit::ClearAnimList));
	UICommandList->MapAction(Commands.LastAnim,
		FExecuteAction::CreateSP(this, &FMotionPreProcessToolkit::SelectPreviousAnim));
	UICommandList->MapAction(Commands.NextAnim,
		FExecuteAction::CreateSP(this, &FMotionPreProcessToolkit::SelectNextAnim));
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
			ToolbarBuilder.BeginSection("Animations");
			{
				ToolbarBuilder.AddToolBarButton(FMotionPreProcessToolkitCommands::Get().PickAnims, NAME_None,
					TAttribute<FText>(), TAttribute<FText>(),
					FSlateIcon(FMotionSymphonyStyle::GetStyleSetName(), "MotionSymphony.Toolbar.AddAnims"));

				ToolbarBuilder.AddToolBarButton(FMotionPreProcessToolkitCommands::Get().ClearAnims, NAME_None,
					TAttribute<FText>(), TAttribute<FText>(),
					FSlateIcon(FMotionSymphonyStyle::GetStyleSetName(), "MotionSymphony.Toolbar.ClearAnims"));
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Navigation");
			{

				ToolbarBuilder.AddToolBarButton(FMotionPreProcessToolkitCommands::Get().LastAnim, NAME_None, 
					TAttribute<FText>(), TAttribute<FText>(),
					FSlateIcon(FMotionSymphonyStyle::GetStyleSetName(), "MotionSymphony.Toolbar.LastAnim"));

				ToolbarBuilder.AddToolBarButton(FMotionPreProcessToolkitCommands::Get().NextAnim, NAME_None,
					TAttribute<FText>(), TAttribute<FText>(),
					FSlateIcon(FMotionSymphonyStyle::GetStyleSetName(), "MotionSymphony.Toolbar.NextAnim"));
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Processing");
			{
				ToolbarBuilder.AddToolBarButton(FMotionPreProcessToolkitCommands::Get().PreProcess, NAME_None,
					TAttribute<FText>(), TAttribute<FText>(),
					FSlateIcon(FMotionSymphonyStyle::GetStyleSetName(), "MotionSymphony.Toolbar.PreProcess"));
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
	UDebugSkelMeshComponent* PreviewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();
	UAnimPreviewInstance* PreviewInstance = PreviewSkeletonMeshComponent->PreviewInstance;

	if (!PreviewSkeletonMeshComponent || !PreviewInstance)
	{
		return FReply::Handled();
	}

	const bool bIsReverse = PreviewInstance->IsReverse();
	const bool bIsPlaying = PreviewInstance->IsPlaying();

	if (bIsPlaying)
	{
		if (bIsReverse)
		{
			PreviewInstance->SetReverse(false);
		}
		else
		{
			PreviewInstance->StopAnim();
		}
	}
	else
	{
		PreviewInstance->SetPlaying(true);
	}

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Forward_Step()
{
	UDebugSkelMeshComponent* PreviewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!PreviewSkeletonMeshComponent)
	{
		return FReply::Handled();
	}
		
	PreviewSkeletonMeshComponent->PreviewInstance->StopAnim();
	SetCurrentFrame(GetCurrentFrame() + 1);

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Forward_End()
{
	UDebugSkelMeshComponent* PreviewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!PreviewSkeletonMeshComponent)
	{
		return FReply::Handled();
	}

	PreviewSkeletonMeshComponent->PreviewInstance->StopAnim();
	PreviewSkeletonMeshComponent->PreviewInstance->SetPosition(GetTotalSequenceLength(), false);

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Backward()
{
	UDebugSkelMeshComponent* PreviewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();
	UAnimPreviewInstance* PreviewInstance = PreviewSkeletonMeshComponent->PreviewInstance;

	if (!PreviewSkeletonMeshComponent || !PreviewInstance)
	{
		return FReply::Handled();
	}

	const bool bIsReverse = PreviewInstance->IsReverse();
	const bool bIsPlaying = PreviewInstance->IsPlaying();

	if (bIsPlaying)
	{
		if (bIsReverse)
		{
			PreviewInstance->StopAnim();
		}
		else
		{
			PreviewInstance->SetReverse(true);
		}
	}
	else
	{
		if (!bIsReverse)
		{
			PreviewInstance->SetReverse(true);
		}

		PreviewInstance->SetPlaying(true);
	}

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Backward_Step()
{
	UDebugSkelMeshComponent* PreviewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!PreviewSkeletonMeshComponent)
	{
		return FReply::Handled();
	}

	PreviewSkeletonMeshComponent->PreviewInstance->StopAnim();
	SetCurrentFrame(GetCurrentFrame() - 1);

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_Backward_End()
{
	UDebugSkelMeshComponent* PreviewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!PreviewSkeletonMeshComponent)
	{
		return FReply::Handled();
	}

	PreviewSkeletonMeshComponent->PreviewInstance->StopAnim();
	PreviewSkeletonMeshComponent->PreviewInstance->SetPosition(0.0f, false);

	return FReply::Handled();
}

FReply FMotionPreProcessToolkit::OnClick_ToggleLoop()
{
	UDebugSkelMeshComponent* PreviewSkeletonMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!PreviewSkeletonMeshComponent)
	{
		return FReply::Handled();
	}

	PreviewSkeletonMeshComponent->PreviewInstance->SetLooping(
		PreviewSkeletonMeshComponent->PreviewInstance->IsLooping());

	return FReply::Handled();
}

uint32 FMotionPreProcessToolkit::GetTotalFrameCount() const
{
	UAnimSequence* CurrentAnim = GetCurrentAnimation();
	
	if (CurrentAnim)
	{
		return CurrentAnim->GetNumberOfSampledKeys();
	}

	return 0;
}

uint32 FMotionPreProcessToolkit::GetTotalFrameCountPlusOne() const
{
	return GetTotalFrameCount() + 1;
}

float FMotionPreProcessToolkit::GetTotalSequenceLength() const
{
	UAnimSequence* CurrentAnim = GetCurrentAnimation();

	if (!CurrentAnim)
	{
		return 0.0f;
	}

	return CurrentAnim->GetPlayLength();
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
		const FMotionAnimAsset* AnimAsset = GetCurrentMotionAnim();
		
		PreviewPoseCurrentIndex = PreviewPoseStartIndex + FMath::RoundToInt(Time / (ActiveMotionDataAsset->PoseInterval * AnimAsset->PlayRate));
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
		const FMotionBlendSpace& MotionBlendSpace = ActiveMotionDataAsset->GetEditableSourceBlendSpaceAtIndex(BlendSpaceIndex);

		if (MotionBlendSpace.BlendSpace)
		{
			return FText::AsCultureInvariant(MotionBlendSpace.BlendSpace->GetName());
		}
	}

	return LOCTEXT("NullAnimation", "Null Animation");
}

FText FMotionPreProcessToolkit::GetCompositeName(const int32 CompositeIndex)
{
	if (ActiveMotionDataAsset->GetSourceCompositeCount() > 0)
	{
		const FMotionComposite& MotionComposite = ActiveMotionDataAsset->GetEditableSourceCompositeAtIndex(CompositeIndex);

		if (MotionComposite.AnimComposite)
		{
			return FText::AsCultureInvariant(MotionComposite.AnimComposite->GetName());
		}
	}

	return LOCTEXT("NullAnimation", "Null Animation");
}

void FMotionPreProcessToolkit::SetCurrentAnimation(const int32 AnimIndex, const EMotionAnimAssetType AnimType)
{
	if (IsValidAnim(AnimIndex, AnimType)
		&& ActiveMotionDataAsset->SetAnimMetaPreviewIndex(AnimType, AnimIndex))
	{
		if (AnimIndex != CurrentAnimIndex || CurrentAnimType != AnimType)
		{
			CurrentAnimIndex = AnimIndex;
			CurrentAnimType = AnimType;

			if (ActiveMotionDataAsset->bIsProcessed)
			{
				const int32 PoseCount = ActiveMotionDataAsset->Poses.Num();
				for (int32 i = 0; i < PoseCount; ++i)
				{
					const FPoseMotionData& StartPose = ActiveMotionDataAsset->Poses[i];

					if (StartPose.AnimId == CurrentAnimIndex
						&& StartPose.AnimType == CurrentAnimType)
					{
						PreviewPoseStartIndex = i;
						PreviewPoseCurrentIndex = i;

						for (int32 k = i; k < PoseCount; ++k)
						{
							const FPoseMotionData& EndPose = ActiveMotionDataAsset->Poses[k];

							if (k == PoseCount - 1)
							{
								PreviewPoseEndIndex = k;
								break;
							}

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

			switch (CurrentAnimType)
			{
				case EMotionAnimAssetType::Sequence:
				{
					SetPreviewAnimation(ActiveMotionDataAsset->GetEditableSourceAnimAtIndex(AnimIndex)); 
					CurrentAnimName = GetAnimationName(CurrentAnimIndex);
				} break;
				case EMotionAnimAssetType::BlendSpace: 
				{
					SetPreviewAnimation(ActiveMotionDataAsset->GetEditableSourceBlendSpaceAtIndex(AnimIndex));
					CurrentAnimName = GetBlendSpaceName(CurrentAnimIndex);
				} break;
				case EMotionAnimAssetType::Composite: 
				{
					SetPreviewAnimation(ActiveMotionDataAsset->GetEditableSourceCompositeAtIndex(AnimIndex)); 
					CurrentAnimName = GetCompositeName(CurrentAnimIndex);
				} break;
				default: break;
			}

			CacheTrajectory();

			//Set the anim meta data as the AnimDetailsViewObject
			if (AnimDetailsView.IsValid())
			{
				AnimDetailsView->SetObject(ActiveMotionDataAsset->MotionMetaWrapper);
			}
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

		if (AnimDetailsView.IsValid() && AnimIndex != CurrentAnimIndex)
		{
			AnimDetailsView->SetObject(nullptr);
		}
	}
}

FMotionAnimAsset* FMotionPreProcessToolkit::GetCurrentMotionAnim() const
{
	if(!ActiveMotionDataAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get current motion anim as the ActiveMotionDataAsset is null."))
		return nullptr;
	}

	return ActiveMotionDataAsset->GetSourceAnim(CurrentAnimIndex, CurrentAnimType);
}

UAnimSequence* FMotionPreProcessToolkit::GetCurrentAnimation() const
{
	if (!ActiveMotionDataAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get current animation as the ActiveMotionDataAsset is null."))
		return nullptr;
	}

	if (ActiveMotionDataAsset->IsValidSourceAnimIndex(CurrentAnimIndex))
	{
		UAnimSequence* CurrentAnim = ActiveMotionDataAsset->GetEditableSourceAnimAtIndex(CurrentAnimIndex).Sequence;
		if (CurrentAnim)
		{
			check(CurrentAnim);
			return(CurrentAnim);
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
	AnimationTreePtr.Get()->RebuildAnimTree();

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
	//AnimationListPtr.Get()->Rebuild();
	AnimationTreePtr.Get()->RebuildAnimTree();

	if (ActiveMotionDataAsset->GetSourceBlendSpaceCount() == 0)
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		SetPreviewAnimationNull();
		AnimDetailsView->SetObject(nullptr, true);
	}
}

void FMotionPreProcessToolkit::DeleteComposite(const int32 CompositeIndex)
{
	if (CompositeIndex == CurrentAnimIndex)
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		SetPreviewAnimationNull();
		AnimDetailsView->SetObject(nullptr, true);
	}

	ActiveMotionDataAsset->DeleteSourceComposite(CompositeIndex);
	AnimationTreePtr.Get()->RebuildAnimTree();

	if (ActiveMotionDataAsset->GetSourceCompositeCount() == 0)
	{
		CurrentAnimIndex = INDEX_NONE;
		CurrentAnimType = EMotionAnimAssetType::None;
		SetPreviewAnimationNull();
		AnimDetailsView->SetObject(nullptr, true);
	}
}

void FMotionPreProcessToolkit::CopyAnim(const int32 CopyAnimIndex, const EMotionAnimAssetType AnimType) const
{
	switch(AnimType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			const FMotionAnimSequence& MotionSequence = ActiveMotionDataAsset->GetSourceAnimAtIndex(CopyAnimIndex);
			CopiedSequences.Emplace(FMotionAnimSequence(&MotionSequence));
				
			for(const FAnimNotifyEvent& CopyNotify : MotionSequence.Tags)
			{
				CopyPasteNotify(CopiedSequences.Last(), CopyNotify);
			}
				
		} break;
		case EMotionAnimAssetType::Composite:
		{
			const FMotionComposite& MotionComposite = ActiveMotionDataAsset->GetSourceCompositeAtIndex(CopyAnimIndex);	
			CopiedComposites.Emplace(FMotionComposite(&MotionComposite));

			for(const FAnimNotifyEvent& CopyNotify : MotionComposite.Tags)
			{
				CopyPasteNotify(CopiedComposites.Last(), CopyNotify);
			}
				
		} break;
		case EMotionAnimAssetType::BlendSpace:
		{
			const FMotionBlendSpace& MotionBlendSpace = ActiveMotionDataAsset->GetSourceBlendSpaceAtIndex(CopyAnimIndex);	
			CopiedBlendSpaces.Emplace(FMotionBlendSpace(MotionBlendSpace));

			for(const FAnimNotifyEvent& CopyNotify : MotionBlendSpace.Tags)
			{
				CopyPasteNotify(CopiedBlendSpaces.Last(), CopyNotify);
			}
				
		} break;
		default: ;
	}
}

void FMotionPreProcessToolkit::PasteAnims()
{
	if(!ActiveMotionDataAsset)
	{
		return;
	}

	ActiveMotionDataAsset->Modify();
	
	for(const FMotionAnimSequence& MotionSequence : CopiedSequences)
	{
		TArray<FMotionAnimSequence>& SequenceArray = ActiveMotionDataAsset->SourceMotionAnims;
		
		SequenceArray.Emplace(FMotionAnimSequence(&MotionSequence));
		FMotionAnimSequence& NewMotionAnim = SequenceArray.Last();
		NewMotionAnim.ParentMotionDataAsset = ActiveMotionDataAsset;
		NewMotionAnim.AnimId = SequenceArray.Num() - 1;

		for(const FAnimNotifyEvent& CopyNotify : MotionSequence.Tags)
		{
			CopyPasteNotify(NewMotionAnim, CopyNotify);
		}
	}
	
	for(FMotionComposite& MotionComposite : CopiedComposites)
	{
		TArray<FMotionComposite>& CompositeArray = ActiveMotionDataAsset->SourceComposites;
		
		CompositeArray.Emplace(FMotionComposite(&MotionComposite));
		FMotionComposite& NewMotionComposite = CompositeArray.Last();
		NewMotionComposite.ParentMotionDataAsset = ActiveMotionDataAsset;
		NewMotionComposite.AnimId = CompositeArray.Num() - 1;

		for(FAnimNotifyEvent CopyNotify : MotionComposite.Tags)
		{
			CopyPasteNotify(NewMotionComposite, CopyNotify);
		}
	}
	
	for(FMotionBlendSpace& MotionBlendSpace : CopiedBlendSpaces)
	{
		TArray<FMotionBlendSpace>& BlendSpaceArray = ActiveMotionDataAsset->SourceBlendSpaces;
		
		BlendSpaceArray.Emplace(FMotionBlendSpace(&MotionBlendSpace));
		FMotionBlendSpace& NewMotionBlendSpace = BlendSpaceArray.Last();
		NewMotionBlendSpace.ParentMotionDataAsset = ActiveMotionDataAsset;
		NewMotionBlendSpace.AnimId = BlendSpaceArray.Num() - 1;

		for(FAnimNotifyEvent CopyNotify : MotionBlendSpace.Tags)
		{
			CopyPasteNotify(NewMotionBlendSpace, CopyNotify);
		}
	}
	
	ActiveMotionDataAsset->bIsProcessed = false;
	ActiveMotionDataAsset->MarkPackageDirty();
}

void FMotionPreProcessToolkit::ClearCopyClipboard()
{
	CopiedSequences.Empty();
	CopiedComposites.Empty();
	CopiedBlendSpaces.Empty();
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

	//Delete All Sequences
	for (int32 AnimIndex = ActiveMotionDataAsset->GetSourceAnimCount() - 1; AnimIndex > -1; --AnimIndex)
	{
		DeleteAnimSequence(AnimIndex);
	}

	//Delete All Composites
	for (int32 AnimIndex = ActiveMotionDataAsset->GetSourceCompositeCount() - 1; AnimIndex > -1; --AnimIndex)
	{
		DeleteComposite(AnimIndex);
	}

	//Delete All BlendSpaces
	for (int32 AnimIndex = ActiveMotionDataAsset->GetSourceBlendSpaceCount() - 1; AnimIndex > -1; --AnimIndex)
	{
		DeleteBlendSpace(AnimIndex);
	}
	
	AnimationTreePtr.Get()->RebuildAnimTree();
}

void FMotionPreProcessToolkit::ClearBlendSpaceList()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Clear Source BlendSpace List",
		"Are you sure you want to remove all blend spaces from the anim list?"))
		== EAppReturnType::No)
	{
		return;
	}
	SetPreviewAnimationNull();
	AnimDetailsView->SetObject(nullptr, true);

	//Delete All BlendSpaces
	for (int32 AnimIndex = ActiveMotionDataAsset->GetSourceBlendSpaceCount() - 1; AnimIndex > -1; --AnimIndex)
	{
		DeleteBlendSpace(AnimIndex);
	}
	
	AnimationTreePtr.Get()->RebuildAnimTree();
}

void FMotionPreProcessToolkit::ClearCompositeList()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Clear Source Composite List",
		"Are you sure you want to remove all composites from the anim list?"))
		== EAppReturnType::No)
	{
		return;
	}

	SetPreviewAnimationNull();
	AnimDetailsView->SetObject(nullptr, true);

	//Delete All Composites
	for (int32 AnimIndex = ActiveMotionDataAsset->GetSourceCompositeCount() - 1; AnimIndex > -1; --AnimIndex)
	{
		DeleteComposite(AnimIndex);
	}
	
	AnimationTreePtr.Get()->RebuildAnimTree();
}

void FMotionPreProcessToolkit::AddNewAnimSequences(TArray<UAnimSequence*> SequenceList)
{
	SAnimTree* AnimTree = AnimationTreePtr.Get();
	for (int32 i = 0; i < SequenceList.Num(); ++i)
	{
		UAnimSequence* AnimSequence = SequenceList[i];

		if (AnimSequence)
		{
			ActiveMotionDataAsset->AddSourceAnim(AnimSequence);
		
			if(AnimTree)
			{
				const int32 AnimId = ActiveMotionDataAsset->SourceMotionAnims.Num() - 1;
				AnimTree->AddAnimSequence(AnimSequence, AnimId);
			}
		}
	}
}
void FMotionPreProcessToolkit::AddNewBlendSpaces(TArray<UBlendSpace*> BlendSpaceList)
{
	SAnimTree* AnimTree = AnimationTreePtr.Get();
	for (int32 i = 0; i < BlendSpaceList.Num(); ++i)
	{
		UBlendSpace* BlendSpace = BlendSpaceList[i];

		if (BlendSpace)
		{
			ActiveMotionDataAsset->AddSourceBlendSpace(BlendSpace);

			if(AnimTree)
			{
				const int32 AnimId = ActiveMotionDataAsset->SourceBlendSpaces.Num() - 1;
				AnimTree->AddBlendSpace(BlendSpace, AnimId);
			}
		}
	}
}

void FMotionPreProcessToolkit::AddNewComposites(TArray<UAnimComposite*> CompositeList)
{
	SAnimTree* AnimTree = AnimationTreePtr.Get();
	for (int32 i = 0; i < CompositeList.Num(); ++i)
	{
		UAnimComposite* Composite = CompositeList[i];

		if (Composite)
		{
			ActiveMotionDataAsset->AddSourceComposite(Composite);

			if(AnimTree)
			{
				const int32 AnimId = ActiveMotionDataAsset->SourceComposites.Num() - 1;
				AnimTree->AddAnimComposite(Composite, AnimId);
			}
		}
	}
}

void FMotionPreProcessToolkit::SelectPreviousAnim()
{
	if (CurrentAnimIndex < 1)
	{
		switch (CurrentAnimType)
		{
			case EMotionAnimAssetType::Sequence:
			{
				SetCurrentAnimation(ActiveMotionDataAsset->GetSourceBlendSpaceCount() - 1, EMotionAnimAssetType::BlendSpace);
			} break;
			case EMotionAnimAssetType::BlendSpace:
			{
				SetCurrentAnimation(ActiveMotionDataAsset->GetSourceCompositeCount() - 1, EMotionAnimAssetType::Composite);
			} break;
			case EMotionAnimAssetType::Composite:
			{
				SetCurrentAnimation(ActiveMotionDataAsset->GetSourceAnimCount() - 1, EMotionAnimAssetType::Sequence);
			} break;
		default: break;
		}
	}
	else
	{
		SetCurrentAnimation(CurrentAnimIndex - 1, CurrentAnimType);
	}
}

void FMotionPreProcessToolkit::SelectNextAnim()
{
	switch (CurrentAnimType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			if (CurrentAnimIndex >= ActiveMotionDataAsset->GetSourceAnimCount() - 1)
			{
				SetCurrentAnimation(0, EMotionAnimAssetType::Composite);
				return;
			}
		} break;
		case EMotionAnimAssetType::BlendSpace:
		{
			if(CurrentAnimIndex >= ActiveMotionDataAsset->GetSourceBlendSpaceCount() - 1)
			{
				SetCurrentAnimation(0, EMotionAnimAssetType::Sequence);
				return;
			}
		} break;
		case EMotionAnimAssetType::Composite:
		{
			if(CurrentAnimIndex >= ActiveMotionDataAsset->GetSourceCompositeCount() -1)
			{
				SetCurrentAnimation(0, EMotionAnimAssetType::BlendSpace);
				return;
			}
		} break;
	default: break;
	}

	SetCurrentAnimation(CurrentAnimIndex + 1, CurrentAnimType);
}

FString FMotionPreProcessToolkit::GetSkeletonName() const
{
	USkeleton* Skeleton = GetSkeleton();

	if(!Skeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get skeleton name as the motion match config does not have a skeleton set."));
		return FString();
	}
	
	return Skeleton->GetName();
}

USkeleton* FMotionPreProcessToolkit::GetSkeleton() const
{
	if(!ActiveMotionDataAsset
		|| !ActiveMotionDataAsset->MotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get skeleton motion match config could not be found."));
		return nullptr;
	}

	
	return ActiveMotionDataAsset->MotionMatchConfig->GetSourceSkeleton();
}

void FMotionPreProcessToolkit::SetSkeleton(USkeleton* Skeleton) const
{
	if(!ActiveMotionDataAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to set skeleton as the ActiveMotionDataAsset was null"));
	}
	
	ActiveMotionDataAsset->SetSkeleton(Skeleton);
}

bool FMotionPreProcessToolkit::AnimationAlreadyAdded(const FName SequenceName) const
{
	const int32 SequenceCount = ActiveMotionDataAsset->GetSourceAnimCount();

	for (int32 i = 0; i < SequenceCount; ++i)
	{
		const FMotionAnimSequence& MotionAnim = ActiveMotionDataAsset->GetEditableSourceAnimAtIndex(i);

		if (MotionAnim.Sequence != nullptr && MotionAnim.Sequence->GetFName() == SequenceName)
		{
			return true;
		}
	}

	const int32 BlendSpaceCount = ActiveMotionDataAsset->GetSourceBlendSpaceCount();

	for(int32 i = 0; i < BlendSpaceCount; ++i)
	{
		const FMotionBlendSpace& MotionBlendSpace = ActiveMotionDataAsset->GetEditableSourceBlendSpaceAtIndex(i);

		if (MotionBlendSpace.BlendSpace != nullptr && MotionBlendSpace.BlendSpace->GetFName() == SequenceName)
		{
			return true;
		}
	}

	return false;
}

bool FMotionPreProcessToolkit::IsValidAnim(const int32 AnimIndex) const
{
	if (ActiveMotionDataAsset->IsValidSourceAnimIndex(AnimIndex)
		&& ActiveMotionDataAsset->GetSourceAnimAtIndex(AnimIndex).Sequence)
	{
			return true;
	}

	return false;
}

bool FMotionPreProcessToolkit::IsValidAnim(const int32 AnimIndex, const EMotionAnimAssetType AnimType) const
{
	switch(AnimType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			if (ActiveMotionDataAsset->IsValidSourceAnimIndex(AnimIndex)
				&& ActiveMotionDataAsset->GetSourceAnimAtIndex(AnimIndex).Sequence)
			{
				return true;
			}
		} break;
		case EMotionAnimAssetType::BlendSpace:
		{
			if (ActiveMotionDataAsset->IsValidSourceBlendSpaceIndex(AnimIndex)
				&& ActiveMotionDataAsset->GetSourceBlendSpaceAtIndex(AnimIndex).BlendSpace)
			{
				return true;
			}
		} break;
		case EMotionAnimAssetType::Composite:
		{
			if (ActiveMotionDataAsset->IsValidSourceCompositeIndex(AnimIndex)
				&& ActiveMotionDataAsset->GetSourceCompositeAtIndex(AnimIndex).AnimComposite)
			{
				return true;
			}
		} break;
	}

	return false;
}

bool FMotionPreProcessToolkit::IsValidBlendSpace(const int32 BlendSpaceIndex) const
{
	if (ActiveMotionDataAsset->IsValidSourceBlendSpaceIndex(BlendSpaceIndex)
		&& ActiveMotionDataAsset->GetSourceBlendSpaceAtIndex(BlendSpaceIndex).BlendSpace)
	{
		return true;
	}

	return false;
}

bool FMotionPreProcessToolkit::IsValidComposite(const int32 CompositeIndex) const
{
	if (ActiveMotionDataAsset->IsValidSourceCompositeIndex(CompositeIndex)
		&& ActiveMotionDataAsset->GetSourceCompositeAtIndex(CompositeIndex).AnimComposite)
	{
		return true;
	}

	return false;
}

bool FMotionPreProcessToolkit::SetPreviewAnimation(FMotionAnimAsset& MotionAnimAsset) const
{
	ViewportPtr->SetupAnimatedRenderComponent();

	UDebugSkelMeshComponent* DebugMeshComponent = GetPreviewSkeletonMeshComponent();

#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
	if (!DebugMeshComponent || !DebugMeshComponent->GetSkinnedAsset())
#else
	if (!DebugMeshComponent || !DebugMeshComponent->SkeletalMesh)
#endif
	{
		return false;
	}

	MotionTimelinePtr->SetAnimation(&MotionAnimAsset, DebugMeshComponent);

	UAnimationAsset* AnimAsset = MotionAnimAsset.AnimAsset;
	if(AnimAsset)
	{
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		if (AnimAsset->GetSkeleton() == DebugMeshComponent->GetSkinnedAsset()->GetSkeleton())
#else
		if (AnimAsset->GetSkeleton() == DebugMeshComponent->SkeletalMesh->GetSkeleton())
#endif
		{
			DebugMeshComponent->EnablePreview(true, AnimAsset);
			DebugMeshComponent->SetAnimation(AnimAsset);
			DebugMeshComponent->SetPlayRate(MotionAnimAsset.PlayRate);
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
}

void FMotionPreProcessToolkit::SetPreviewAnimationNull() const
{
	UDebugSkelMeshComponent* DebugMeshComponent = GetPreviewSkeletonMeshComponent();

#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
	if (!DebugMeshComponent || !DebugMeshComponent->GetSkinnedAsset())
#else
	if (!DebugMeshComponent || !DebugMeshComponent->SkeletalMesh)
#endif
	{
		return;
	}

	MotionTimelinePtr->SetAnimation(nullptr, DebugMeshComponent);

	DebugMeshComponent->EnablePreview(true, nullptr);
}

UDebugSkelMeshComponent* FMotionPreProcessToolkit::GetPreviewSkeletonMeshComponent() const
{
	UDebugSkelMeshComponent* PreviewComponent = ViewportPtr->GetPreviewComponent();
	
	return PreviewComponent;
}

bool FMotionPreProcessToolkit::SetPreviewComponentSkeletalMesh(USkeletalMesh* SkeletalMesh) const
{
	UDebugSkelMeshComponent* PreviewSkelMeshComponent = GetPreviewSkeletonMeshComponent();

	if (!PreviewSkelMeshComponent)
	{
		return false;
	}

	if (SkeletalMesh)
	{
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
		USkeletalMesh* PreviewSkelMesh = PreviewSkelMeshComponent->GetSkeletalMeshAsset();
#else
		USkeletalMesh* PreviewSkelMesh = PreviewSkelMeshComponent->SkeletalMesh;
#endif

		if (PreviewSkelMesh)
		{
			if (PreviewSkelMesh->GetSkeleton() != SkeletalMesh->GetSkeleton())
			{
				SetPreviewAnimationNull();
				PreviewSkelMeshComponent->SetSkeletalMesh(SkeletalMesh, true);
				ViewportPtr->OnFocusViewportToSelection();
				return false;
			}
			else
			{
				PreviewSkelMeshComponent->SetSkeletalMesh(SkeletalMesh, false);
				ViewportPtr->OnFocusViewportToSelection();
				return true;
			}
		}
		
		SetPreviewAnimationNull();

		PreviewSkelMeshComponent->SetSkeletalMesh(SkeletalMesh, true);
		ViewportPtr->OnFocusViewportToSelection();
	}
	else
	{
		SetPreviewAnimationNull();
		PreviewSkelMeshComponent->SetSkeletalMesh(nullptr, true);
	}

	return false;
}

void FMotionPreProcessToolkit::SetCurrentFrame(int32 NewIndex)
{
	const int32 TotalLengthInFrames = GetTotalFrameCount();
	const int32 ClampedIndex = FMath::Clamp<int32>(NewIndex, 0, TotalLengthInFrames);
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
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to pre-process motion matching animation data. The Motion Data asset is null."));
		return;
	}

	if (!ActiveMotionDataAsset->CheckValidForPreProcess())
	{
		return;
	}

	ActiveMotionDataAsset->Modify();
	ActiveMotionDataAsset->PreProcess();
	ActiveMotionDataAsset->MarkPackageDirty();
}

void FMotionPreProcessToolkit::OpenPickAnimsDialog()
{
	if (!ActiveMotionDataAsset)
	{
		return;
	}

	if (!ActiveMotionDataAsset->MotionMatchConfig)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Cannot Add Anims With Null MotionMatchConfig",
			"Not 'MotionMatchConfig' has been set for this anim data. Please set it in the details panel before picking anims."));

		return;
	}

	if (ActiveMotionDataAsset->MotionMatchConfig->GetSourceSkeleton() == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Cannot Add Anims With Null Skeleton On MotionMatchConfig",
			"The MotionMatchConfig does not have a valid skeleton set. Please set up your 'MotionMatchConfig' with a valid skeleton."));

		return;
	}

	SAddNewAnimDialog::ShowWindow(AnimationTreePtr->MotionPreProcessToolkitPtr.Pin());
}

void FMotionPreProcessToolkit::CacheTrajectory()
{
	if (!ActiveMotionDataAsset)
	{
		return;
	}

	FMotionAnimAsset* MotionAnim = ActiveMotionDataAsset->GetSourceAnim(CurrentAnimIndex, CurrentAnimType);

	if (!MotionAnim)
	{
		CachedTrajectoryPoints.Empty();
		return;
	}

	CachedTrajectoryPoints.Empty(FMath::CeilToInt(MotionAnim->GetPlayLength() / 0.1f) + 1);

	//Step through the animation 0.1s at a time and record a trajectory point
	CachedTrajectoryPoints.Add(FVector(0.0f));

	MotionAnim->CacheTrajectoryPoints(CachedTrajectoryPoints);

	//Transform the cached trajectory for non-standard axis
	// UMotionMatchConfig* MMConfig = ActiveMotionDataAsset->MotionMatchConfig;
	// if(!MMConfig)
	// {
	// 	return;
	// }
	//
	// FTransform CorrectionTransform;
	// UMMBlueprintFunctionLibrary::TransformFromUpForwardAxis(CorrectionTransform, MMConfig->UpAxis, MMConfig->ForwardAxis);
	// for(int i = 0; i < CachedTrajectoryPoints.Num(); ++i)
	// {
	// 	CachedTrajectoryPoints[i] = CorrectionTransform.TransformPosition(CachedTrajectoryPoints[i]); 
	// }
}

void FMotionPreProcessToolkit::CopyPasteNotify(FMotionAnimAsset& MotionAnimAsset, const FAnimNotifyEvent& InNotify) const
{
	const int32 NewNotifyIndex = MotionAnimAsset.Tags.Add(FAnimNotifyEvent());
	FAnimNotifyEvent& NewEvent = MotionAnimAsset.Tags[NewNotifyIndex];
	
	NewEvent.NotifyName = InNotify.NotifyName;
	NewEvent.Guid = FGuid::NewGuid();

	UAnimSequenceBase* MotionSequence = Cast<UAnimSequenceBase>(MotionAnimAsset.AnimAsset);

	const float StartTime = InNotify.GetTriggerTime();
	const float EndTime = InNotify.GetEndTriggerTime();
	
	NewEvent.Link(MotionSequence, StartTime);
	NewEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(MotionSequence->CalculateOffsetForNotify(StartTime));
	NewEvent.EndTriggerTimeOffset = GetTriggerTimeOffsetForType(MotionSequence->CalculateOffsetForNotify(EndTime));
	NewEvent.TrackIndex = InNotify.TrackIndex;

	TSubclassOf<UObject> NotifyClass = nullptr;
	if(InNotify.Notify)
	{
		NotifyClass = InNotify.Notify.GetClass();
	}
	else if(InNotify.NotifyStateClass)
	{
		NotifyClass = InNotify.NotifyStateClass.GetClass();
	}
	
	if(NotifyClass)
	{
		UObject* AnimNotifyClass = NewObject<UObject>(ActiveMotionDataAsset, NotifyClass, NAME_None, RF_Transactional); //The outer object should probably be the MotionAnimData

		UTagPoint* TagPoint = Cast<UTagPoint>(AnimNotifyClass);
		UTagSection* TagSection = Cast<UTagSection>(AnimNotifyClass);

		if(TagPoint)
		{
			TagPoint->CopyTagData(Cast<UTagPoint>(InNotify.Notify));
		}
		else if(TagSection)
		{
			TagSection->CopyTagData(Cast<UTagSection>(InNotify.Notify));
		}
		
		NewEvent.NotifyStateClass = Cast<UAnimNotifyState>(AnimNotifyClass);
		NewEvent.Notify = Cast<UAnimNotify>(AnimNotifyClass);
		
		// Set default duration to 1 frame for AnimNotifyState.
		if (NewEvent.NotifyStateClass)
		{
			NewEvent.SetDuration(InNotify.GetDuration());
			NewEvent.EndLink.Link(Cast<UAnimSequenceBase>(MotionAnimAsset.AnimAsset), InNotify.EndLink.GetTime());
		}
	}
	else
	{
		NewEvent.Notify = NULL;
		NewEvent.NotifyStateClass = NULL;
	}

	if(NewEvent.Notify)
	{
		TArray<FAssetData> SelectedAssets;
		AssetSelectionUtils::GetSelectedAssets(SelectedAssets);

		for (TFieldIterator<FObjectProperty> PropIt(NewEvent.Notify->GetClass()); PropIt; ++PropIt)
		{
			if (PropIt->GetBoolMetaData(TEXT("ExposeOnSpawn")))
			{
				FObjectProperty* Property = *PropIt;
				const FAssetData* Asset = SelectedAssets.FindByPredicate([Property](const FAssetData& Other)
				{
					return Other.GetAsset()->IsA(Property->PropertyClass);
				});
			
				if (Asset)
				{
					uint8* Offset = (*PropIt)->ContainerPtrToValuePtr<uint8>(NewEvent.Notify);
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
					(*PropIt)->ImportText_Direct(*Asset->GetAsset()->GetPathName(), Offset,NewEvent.Notify,  0);
#else
					(*PropIt)->ImportText(*Asset->GetAsset()->GetPathName(), Offset, 0, NewEvent.Notify);
#endif
					break;
				}
			}
		}

		NewEvent.Notify->OnAnimNotifyCreatedInEditor(NewEvent);
	}
	else if(NewEvent.NotifyStateClass)
	{
		NewEvent.NotifyStateClass->OnAnimNotifyCreatedInEditor(NewEvent);
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
	if (AnimDetailsView.IsValid() && SelectedObjects.Num() > 0)
	{
		AnimDetailsView->SetObjects(SelectedObjects, true);
	}
	
}

bool FMotionPreProcessToolkit::HasCopiedAnims()
{
	if(CopiedSequences.Num() > 0 
		|| CopiedComposites.Num() > 0 
		|| CopiedBlendSpaces.Num() > 0)
	{
		return true;
	}
	
	return false;
}

#undef LOCTEXT_NAMESPACE
