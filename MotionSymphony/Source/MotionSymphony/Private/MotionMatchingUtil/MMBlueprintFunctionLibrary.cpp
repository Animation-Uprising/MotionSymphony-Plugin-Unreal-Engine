// Fill out your copyright notice in the Description page of Project Settings.


#include "MMBlueprintFunctionLibrary.h"
#include "Camera/CameraComponent.h"

FVector UMMBlueprintFunctionLibrary::GetInputVectorRelativeToCamera(FVector InputVector, UCameraComponent* CameraComponent)
{
	FRotator CameraProjectedRotation = CameraComponent->GetComponentToWorld().GetRotation().Rotator();
	CameraProjectedRotation.Roll = 0.0f;
	CameraProjectedRotation.Pitch = 0.0f;

	return CameraProjectedRotation.RotateVector(InputVector);
}
