// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRInteractableComponent.h"
#include "PRInteractionAction.h"
#include "Components/MeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

UPRInteractableComponent::UPRInteractableComponent()
{
	SetIsReplicatedByDefault(true);
}

void UPRInteractableComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentInteractor);
}

void UPRInteractableComponent::OnRep_CurrentInteractor()
{
	// TODO : 클라측 피드백 (사용 중 표시 등)
}

void UPRInteractableComponent::OnFocus(bool bIsInRange)
{
	if (bIsFocused)
	{
		return;
	}
	bIsFocused = true;

	TArray<UMeshComponent*> Meshes;
	UPRGameplayStatics::GetAllMeshComponents(GetOwner(), Meshes);

	SavedCustomDepthStates.Reset();
	for (UMeshComponent* Mesh : Meshes)
	{
		if (IsValid(Mesh))
		{
			// 원래 RenderCustomDepth 상태 저장
			SavedCustomDepthStates.Add(TObjectPtr<UMeshComponent>(Mesh), Mesh->bRenderCustomDepth);

			Mesh->SetRenderCustomDepth(true);
			Mesh->SetCustomDepthStencilValue(bIsInRange ? PRStencilValues::Interaction : PRStencilValues::Highlight);
		}
	}
}

void UPRInteractableComponent::OnUnfocus()
{
	bIsFocused = false;

	TArray<UMeshComponent*> Meshes;
	UPRGameplayStatics::GetAllMeshComponents(GetOwner(), Meshes);

	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh))
		{
			continue;
		}

		Mesh->SetCustomDepthStencilValue(PRStencilValues::Default);

		// 원래 상태로 복원. 저장된 값이 없으면 false로 초기화
		const bool* bWasEnabled = SavedCustomDepthStates.Find(TObjectPtr<UMeshComponent>(Mesh));
		Mesh->SetRenderCustomDepth(bWasEnabled ? *bWasEnabled : false);
	}

	SavedCustomDepthStates.Reset();
}

void UPRInteractableComponent::OnInteract(AActor* Interactor, int32 ActionIndex)
{
	UPRInteractionAction* TargetAction = FindActionByIndex(ActionIndex);
	if (!IsValid(TargetAction))
	{
		return;
	}

	TargetAction->Execute(Interactor);

	// 유지형 Action이면 활성 Action으로 추적
	if (TargetAction->ShouldSustained())
	{
		ActiveAction = TargetAction;
	}
}

UPRInteractionAction* UPRInteractableComponent::SelectBestAction(AActor* Interactor) const
{
	UPRInteractionAction* BestAction = nullptr;
	int32 BestPriority = INT32_MIN;

	for (UPRInteractionAction* Action : InteractionActions)
	{
		if (!IsValid(Action))
		{
			continue;
		}

		if (!Action->CanInteract(Interactor))
		{
			continue;
		}

		const int32 ActionPriority = Action->GetPriority();
		if (ActionPriority > BestPriority)
		{
			BestPriority = ActionPriority;
			BestAction = Action;
		}
	}

	return BestAction;
}


void UPRInteractableComponent::EndActiveInteraction()
{
	if (IsValid(ActiveAction))
	{
		ActiveAction->EndInteraction();
		ActiveAction = nullptr;
	}

	// 배타 점유 해제 (Sustained 종료 동시에 점유도 풀림)
	ReleaseExclusive();
}

UPRInteractionAction* UPRInteractableComponent::FindActionByIndex(int32 InActionIndex) const
{
	if (InteractionActions.IsValidIndex(InActionIndex))
	{
		return InteractionActions[InActionIndex];
	}
	return nullptr;
}

int32 UPRInteractableComponent::FindActionIndex(UPRInteractionAction* InAction) const
{
	int32 Index = -1;
	InteractionActions.Find(InAction, Index);
	return Index;
}

bool UPRInteractableComponent::HasAvailableAction(AActor* Interactor) const
{
	for (UPRInteractionAction* Action : InteractionActions)
	{
		if (IsValid(Action) && Action->CanInteract(Interactor))
		{
			return true;
		}
	}
	return false;
}

bool UPRInteractableComponent::CanBeInteractedBy(AActor* Interactor) const
{
	if (!bExclusiveInteraction)
	{
		return true;
	}

	// 배타 점유 모드: 비점유 또는 본인 점유 시에만 진입 가능
	return !IsValid(CurrentInteractor) || CurrentInteractor == Interactor;
}

void UPRInteractableComponent::AcquireExclusive(AActor* Interactor)
{
	if (bExclusiveInteraction)
	{
		CurrentInteractor = Interactor;
	}
}

void UPRInteractableComponent::ReleaseExclusive()
{
	if (bExclusiveInteraction)
	{
		CurrentInteractor = nullptr;
	}
}
