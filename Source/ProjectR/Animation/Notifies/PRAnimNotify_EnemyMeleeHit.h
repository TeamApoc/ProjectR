// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_EnemyMeleeHit.generated.h"

// 공격 몽타주의 실제 타격 프레임에 배치하는 Notify다.
// 서버에서 현재 활성화된 EnemyMeleeAttack Ability를 찾아 타격 판정을 실행한다.
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
