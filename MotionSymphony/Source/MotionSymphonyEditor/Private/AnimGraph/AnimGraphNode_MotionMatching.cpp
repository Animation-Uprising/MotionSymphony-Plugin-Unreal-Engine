// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraphNode_MotionMatching.h"
#include "AnimationGraphSchema.h"
#include "EditorCategoryUtils.h"
#include "Animation/AnimComposite.h"
#include "Animation/AnimSequence.h"
#include "Kismet2/CompilerResultsLog.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "GraphEditorActions.h"
#include "ARFilter.h"
#include "AssetRegistryModule.h"
#include "BlueprintActionFilter.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Animation/AnimComposite.h"

#define LOCTEXT_NAMESPACE "MoSymphNodes"

UAnimGraphNode_MotionMatching::UAnimGraphNode_MotionMatching(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FLinearColor UAnimGraphNode_MotionMatching::GetNodeTitleColor() const
{
	return FLinearColor::Green;
}

FText UAnimGraphNode_MotionMatching::GetTooltipText() const
{
	if(!Node.MotionData)
	{
		return LOCTEXT("NodeTooltip", "Motion Matching");
	}

	//Additive not supported for motion matching node
	return GetTitleGivenAssetInfo(FText::FromString(Node.MotionData->GetPathName()), false);
}

FText UAnimGraphNode_MotionMatching::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return  LOCTEXT("MotionMatchNullTitle", "Motion Matching");

	// UEdGraphPin* MotionDataPin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_MotionMatching, MotionData));
	//
	// if(MotionDataPin && !MotionDataPin->IsPendingKill())
	// {
	// 	return GetNodeTitleHelper(TitleType, MotionDataPin, LOCTEXT("PlayerDesc", "Motion Matching"),
	// 		[](UAnimationAsset* InAsset)
	// 		{
	// 			return FText::GetEmpty();
	// 		});
	// }

}

FString UAnimGraphNode_MotionMatching::GetNodeCategory() const
{
	return FString("Motion Symphony");
}

void UAnimGraphNode_MotionMatching::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton,  FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);

	Node.GetEvaluateGraphExposedInputs();

	//Check that the Motion Data has been set
	UMotionDataAsset* MotionDataToCheck = Node.MotionData;
	UEdGraphPin* MotionDataPin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_MotionMatching, MotionData));
	if(MotionDataPin && !MotionDataToCheck)
	{
		MotionDataToCheck = Cast<UMotionDataAsset>(MotionDataPin->DefaultObject);
	}

	
	if(!MotionDataToCheck)
	{
		bool bHasMotionDataBinding = false;
		if(MotionDataPin)
		{
			if(FAnimGraphNodePropertyBinding* BindingPtr = PropertyBindings.Find(MotionDataPin->GetFName()))
			{
				bHasMotionDataBinding = true;
			}
		}
		
		if(!MotionDataPin || MotionDataPin->LinkedTo.Num() == 0 && !bHasMotionDataBinding)
		{
			MessageLog.Error(TEXT("@@ references an unknown MotionDataAsset."), this);
			return;
		}
	}
	else if(SupportsAssetClass(MotionDataToCheck->GetClass()) == EAnimAssetHandlerType::NotSupported)
	{
		MessageLog.Error(*FText::Format(LOCTEXT("UnsupportedAssetError", "@@ is trying to play a {0} as a sequence, which is not allowed."),
			MotionDataToCheck->GetClass()->GetDisplayNameText()).ToString(), this);
		return;
	}
	else
	{
		//Check that the skeleton matches
		USkeleton* MotionDataSkeleton = MotionDataToCheck->GetSkeleton();
		if (!MotionDataSkeleton || !MotionDataSkeleton->IsCompatible(ForSkeleton))
		{
			MessageLog.Error(TEXT("@@ references motion data that uses incompatible skeleton @@"), this, MotionDataSkeleton);
			return;
		}
	}
	
	bool ValidToCompile = true;

	//Check that the Motion Data is valid
	if (!MotionDataToCheck->IsSetupValid())
	{
		MessageLog.Error(TEXT("@@ MotionDataAsset setup is not valid."), this);
		ValidToCompile = false;
	}

	//Check that all sequences are valid
	if (!MotionDataToCheck->AreSequencesValid())
	{
		MessageLog.Error(TEXT("@@ MotionDataAsset contains sequences that are invalid or null."), this);
		ValidToCompile = false;
	}

	//Check that the Motion Data has been pre-processed (if not, ask to process it now)
	if (ValidToCompile)
	{
		if (!MotionDataToCheck->bIsProcessed)
		{
			MessageLog.Warning(TEXT("@@ MotionDataAsset has not been pre-processed. Pre-processing during animation graph compilation is not optimised."), this);
			
			if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Motion Data has not been pre-processed.",
				"The motion data set for this motion matching node has not been pre-processed. Do you want to pre-process it now (fast / un-optimised)?"))
				!= EAppReturnType::Yes)
			{
				MessageLog.Error(TEXT("@@ Cannot compile motion matching node with un-processed motion data."), this);
			}
		}
	}
}

void UAnimGraphNode_MotionMatching::PreloadRequiredAssets()
{
	Super::PreloadRequiredAssets();

	PreloadObject(Node.MotionData);
	
	if(Node.MotionData)
	{

		PreloadObject(Node.MotionData->MotionMatchConfig);
		PreloadObject(Node.MotionData->PreprocessCalibration);
		PreloadObject(Node.MotionData->MirroringProfile);

		for (FMotionAnimSequence& MotionAnim : Node.MotionData->SourceMotionAnims)
		{
			PreloadObject(MotionAnim.Sequence);
		}

		for (FMotionComposite& MotionComposite : Node.MotionData->SourceComposites)
		{
			PreloadObject(MotionComposite.AnimComposite);
		}
		
		for (FMotionBlendSpace& MotionBlendSpace : Node.MotionData->SourceBlendSpaces)
		{
			PreloadObject(MotionBlendSpace.BlendSpace);
		}
	}
}

void UAnimGraphNode_MotionMatching::BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog)
{
	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	AnimBlueprint->FindOrAddGroup(Node.GetGroupName());
	
	//Pre-Process the pose data here
	//if(Node.MotionData && !Node.MotionData->bIsProcessed)
	//{
	//	bool CacheOptimize = Node.MotionData->bOptimize;
	//	Node.MotionData->bOptimize = false;

	//	Node.MotionData->PreProcess(); //Must be the basic type of pre-processing

	//	UE_LOG(LogTemp, Warning, TEXT("Warning: Motion Matching node data was pre-processed during animation graph compilation. The data is not optimised."))

	//	Node.MotionData->bOptimize = CacheOptimize;
	//}
}

bool UAnimGraphNode_MotionMatching::DoesSupportTimeForTransitionGetter() const
{
	return false;
}

UAnimationAsset* UAnimGraphNode_MotionMatching::GetAnimationAsset() const
{
	return Node.MotionData;
}

const TCHAR* UAnimGraphNode_MotionMatching::GetTimePropertyName() const
{
	return TEXT("InternalTimeAccumulator");
}

UScriptStruct* UAnimGraphNode_MotionMatching::GetTimePropertyStruct() const
{
	return FAnimNode_MotionMatching::StaticStruct();
}

void UAnimGraphNode_MotionMatching::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets) const
{
	if (Node.MotionData != nullptr)
	{
		for (FMotionAnimSequence& MotionAnim : Node.MotionData->SourceMotionAnims)
		{
			if (MotionAnim.Sequence == nullptr)
				continue;

			MotionAnim.Sequence->HandleAnimReferenceCollection(AnimationAssets, true);
		}
	}
}

void UAnimGraphNode_MotionMatching::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& AnimAssetReplacementMap)
{
	HandleAnimReferenceReplacement(Node.MotionData, AnimAssetReplacementMap);
}

void UAnimGraphNode_MotionMatching::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
	NodeSpawner->DefaultMenuSignature.MenuName = FText::FromString(TEXT("Motion Matching"));
	NodeSpawner->DefaultMenuSignature.Tooltip = FText::FromString(TEXT("Animation synthesis via motion matching."));
	ActionRegistrar.AddBlueprintAction(NodeSpawner);
}

EAnimAssetHandlerType UAnimGraphNode_MotionMatching::SupportsAssetClass(const UClass* AssetClass) const
{
	if (AssetClass->IsChildOf(UMotionDataAsset::StaticClass()))
	{
		return EAnimAssetHandlerType::Supported;
	}
	else
	{
		return EAnimAssetHandlerType::NotSupported;
	}
}

void UAnimGraphNode_MotionMatching::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (!Context->bIsDebugging)
	{
		/*FToolMenuSection& Section = Menu->AddSection("AnimGraphNodeMotionMatching", NSLOCTEXT("MoSymphNodes", "MotionMatchHeading", "Motion Matching"));
		Section.AddMenuEntry(FGraphEditorCommands::Get().OpenRelatedAsset);*/
	}
}

void UAnimGraphNode_MotionMatching::SetAnimationAsset(UAnimationAsset* Asset)
{
	if (UMotionDataAsset* MotionDataAsset = Cast<UMotionDataAsset>(Asset))
	{
		Node.MotionData = MotionDataAsset;
	}
}
void UAnimGraphNode_MotionMatching::OnProcessDuringCompilation(IAnimBlueprintCompilationContext& InCompilationContext, IAnimBlueprintGeneratedClassCompiledData& OutCompiledData)
{
	//Node.GetEvaluateGraphExposedInputs();
}

FText UAnimGraphNode_MotionMatching::GetTitleGivenAssetInfo(const FText& AssetName, bool bKnownToBeAdditive)
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("AssetName"), AssetName);

	return FText::Format(LOCTEXT("MotionMatchNodeTitle", "Motion Matching \n {AssetName}"), Args);
}

FString UAnimGraphNode_MotionMatching::GetControllerDescription() const
{
	return TEXT("Motion Matching Animation Node");
}

//Node Output Pin(Output is in Component Space, Change at own RISK!)
//void UAnimGraphNode_MotionMatching::CreateOutputPins()
//{
//	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();
//	CreatePin(EGPD_Output, Schema->PC_Struct, TEXT(""), FPoseLink::StaticStruct(), /*bIsArray=*/ false, /*bIsReference=*/ false, TEXT("Pose"));
//}

#undef LOCTEXT_NAMESPACE