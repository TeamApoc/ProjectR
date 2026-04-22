// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_EnemyMeleeHit.generated.h"

// 공격 몽타주의 실제 타격 프레임에 배치하는 Notify다.
// Ability 인스턴스를 직접 찾지 않고 GameplayEvent만 보내 판정 흐름을 GAS 쪽에 맡긴다.
UCLASS(meta = (DisplayName = "PR Enemy Melee Hit"))
class PROJECTR_API UPRAnimNotify_EnemyMeleeHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
