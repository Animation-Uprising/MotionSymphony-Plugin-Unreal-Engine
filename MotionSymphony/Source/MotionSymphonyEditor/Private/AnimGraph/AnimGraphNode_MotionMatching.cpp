// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "AnimGraphNode_MotionMatching.h"
#include "AnimationGraphSchema.h"

#define LOCTEXT_NAMESPACE "AnimGraphNode_MotionMatching"

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
	return LOCTEXT("NodeTooltip", "Motion Matching");
}

FText UAnimGraphNode_MotionMatching::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Motion Matching");
}

FString UAnimGraphNode_MotionMatching::GetNodeCategory() const
{
	return FString("Motion Matching");
}

void UAnimGraphNode_MotionMatching::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{

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