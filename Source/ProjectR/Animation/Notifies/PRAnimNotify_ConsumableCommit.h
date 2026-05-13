// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_ConsumableCommit.generated.h"

// 소비 아이템 사용 몽타주의 효과 적용 프레임에 배치하는 Notify다
UCLASS(meta = (DisplayName = "PR Consumable Commit"))
class PROJECTR_API UPRAnimNotify_ConsumableCommit : public UAnimNotify
{
	GENERATED_BODY()

public:
	/*~ UAnimNotify Interface ~*/
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
