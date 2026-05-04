// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PRAnimNotifyState_DodgeInputCancelWindow.generated.h"

// 회피 몽타주 끝부분에서 입력으로 회피를 조기 종료할 수 있는 구간을 여는 Notify State다.
UCLASS(meta = (DisplayName = "PR Dodge Input Cancel Window"))
class PROJECTR_API UPRAnimNotifyState_DodgeInputCancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/*~ UAnimNotifyState Interface ~*/
	// 에디터에 표시할 Notify State 이름을 반환한다.
	virtual FString GetNotifyName_Implementation() const override;

	// 입력 취소 가능 구간을 시작한다.
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	// 입력 취소 가능 구간을 종료한다.
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// 입력 취소가 발생했을 때 회피 몽타주를 블렌드 아웃할 시간이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Dodge")
	float InputCancelBlendOutTime = 0.18f;
};
