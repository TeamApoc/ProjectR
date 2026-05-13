// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRAnimNotify_ConsumableCommit.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/PRGameplayTags.h"

FString UPRAnimNotify_ConsumableCommit::GetNotifyName_Implementation() const
{
	return TEXT("PR Consumable Commit");
}

void UPRAnimNotify_ConsumableCommit::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		// 실제 효과 적용과 스택 소모는 서버 Ability 인스턴스만 처리한다.
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = PRGameplayTags::Event_Player_ConsumableCommit;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		PRGameplayTags::Event_Player_ConsumableCommit,
		Payload);
}
