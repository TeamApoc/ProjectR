// Copyright ProjectR. All Rights Reserved.

#include "PRAnimNotifyState_DodgeInputCancelWindow.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Animation/PRAnimInstance.h"

FString UPRAnimNotifyState_DodgeInputCancelWindow::GetNotifyName_Implementation() const
{
	return TEXT("PR Dodge Input Cancel Window");
}

void UPRAnimNotifyState_DodgeInputCancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	UPRAnimInstance* AnimInstance = Cast<UPRAnimInstance>(MeshComp->GetAnimInstance());
	if (!IsValid(AnimInstance))
	{
		return;
	}

	// 이 구간부터는 이동 입력을 포함한 플레이어 입력이 회피 종료 요청으로 해석된다.
	AnimInstance->SetDodgeInputCancelWindow(true, InputCancelBlendOutTime);
}

void UPRAnimNotifyState_DodgeInputCancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	UPRAnimInstance* AnimInstance = Cast<UPRAnimInstance>(MeshComp->GetAnimInstance());
	if (!IsValid(AnimInstance))
	{
		return;
	}

	// 구간이 끝나면 남은 입력은 더 이상 회피 스킵으로 사용하지 않는다.
	AnimInstance->SetDodgeInputCancelWindow(false);
}
