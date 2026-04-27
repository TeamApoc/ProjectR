// Copyright ProjectR. All Rights Reserved.

#include "PRAnimNotify_RefreshMotionWarpTarget.h"

#include "PRAnimNotifyMotionWarpTargetUtils.h"

FString UPRAnimNotify_RefreshMotionWarpTarget::GetNotifyName_Implementation() const
{
	return TEXT("PR Refresh Motion Warp Target");
}

void UPRAnimNotify_RefreshMotionWarpTarget::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	PRAnimNotifyMotionWarpTargetUtils::UpdateAttackTargetMotionWarp(MeshComp, bUseTargetLocation);
}
