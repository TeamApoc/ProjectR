// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRInteractionAction.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"


AActor* UPRInteractionAction::GetOwner() const
{
	return GetTypedOuter<AActor>();
}

APawn* UPRInteractionAction::GetPawn(AActor* Interactor) const
{
	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		return Pawn;
	}
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller->GetPawn();
	}
	return nullptr;
}

AController* UPRInteractionAction::GetController(AActor* Interactor) const
{
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller;
	}
	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		return Pawn->GetController();
	}
	return nullptr;
}

bool UPRInteractionAction::CanInteract_Implementation(AActor* Interactor) const
{
	// 기본 구현: 항상 실행 가능. 하위 클래스에서 조건을 재정의한다.
	return true;
}

void UPRInteractionAction::Execute_Implementation(AActor* Interactor)
{
	// 기본 구현: 없음. 하위 클래스에서 실제 행동을 구현한다.
	// 유지형인 경우 활성 상태로 전환
	if (ShouldSustained())
	{
		bIsActive = true;
	}
}

void UPRInteractionAction::EndInteraction_Implementation()
{
	// 기본 구현: 활성 상태 해제만 수행. 하위 클래스에서 정리 로직 추가
	bIsActive = false;
}

void UPRInteractionAction::OnHoldStart_Implementation(AActor* Interactor)
{
	// 기본 구현: 없음. 하위 클래스에서 클라측 피드백 구현
}

void UPRInteractionAction::OnHoldCanceled_Implementation(AActor* Interactor)
{
	// 기본 구현: 없음. 하위 클래스에서 클라측 피드백 구현
}

void UPRInteractionAction::OnHoldFinished_Implementation(AActor* Interactor)
{
	// 기본 구현: 없음. 하위 클래스에서 클라측 피드백 구현
}

int32 UPRInteractionAction::GetPriority_Implementation() const
{
	return Priority;
}
