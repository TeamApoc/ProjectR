// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_EnemyComboWindow.generated.h"

// 콤보 몽타주 섹션 분기 지점용 Notify
// 실제 분기 판단은 서버 Ability의 GameplayEvent 처리 기준
UCLASS(meta = (DisplayName = "PR Enemy Combo Window"))
class PROJECTR_API UPRAnimNotify_EnemyComboWindow : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
