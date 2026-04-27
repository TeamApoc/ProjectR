// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USkeletalMeshComponent;

namespace PRAnimNotifyMotionWarpTargetUtils
{
	// AttackTarget 워프 타겟을 현재 타겟 기준으로 갱신한다.
	bool UpdateAttackTargetMotionWarp(USkeletalMeshComponent* MeshComp, bool bUseTargetLocation);
}
