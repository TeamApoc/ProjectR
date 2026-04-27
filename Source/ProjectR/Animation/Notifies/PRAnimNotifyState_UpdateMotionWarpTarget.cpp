// Copyright ProjectR. All Rights Reserved.

#include "PRAnimNotifyState_UpdateMotionWarpTarget.h"

#include "PRAnimNotifyMotionWarpTargetUtils.h"

FString UPRAnimNotifyState_UpdateMotionWarpTarget::GetNotifyName_Implementation() const
{
	return TEXT("PR Update Motion Warp Target");
}

void UPRAnimNotifyState_UpdateMotionWarpTarget::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	PRAnimNotifyMotionWarpTargetUtils::UpdateAttackTargetMotionWarp(MeshComp, bUseTargetLocation);
}

void UPRAnimNotifyState_UpdateMotionWarpTarget::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	PRAnimNotifyMotionWarpTargetUtils::UpdateAttackTargetMotionWarp(MeshComp, bUseTargetLocation);
}

void UPRAnimNotifyState_UpdateMotionWarpTarget::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}
