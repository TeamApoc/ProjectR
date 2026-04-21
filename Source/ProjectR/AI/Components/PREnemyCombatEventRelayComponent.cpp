// Copyright ProjectR. All Rights Reserved.

#include "PREnemyCombatEventRelayComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPREnemyCombatEventRelayComponent::UPREnemyCombatEventRelayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPREnemyCombatEventRelayComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || bEventsBound)
	{
		// 상태 이벤트는 서버 ASC에서만 발생시킨다.
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
	{
		// 태그가 붙는 순간을 GameplayEvent로 변환해 AbilityTriggers가 반응하게 한다.
		ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPREnemyCombatEventRelayComponent::HandleGroggyTagChanged);
		ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPREnemyCombatEventRelayComponent::HandleDeadTagChanged);
	}

	if (APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(OwnerActor))
	{
		// 보스 페이즈 변경도 동일한 이벤트 통로를 사용한다.
		BossCharacter->OnPhaseChanged.AddDynamic(this, &UPREnemyCombatEventRelayComponent::HandleBossPhaseChanged);
	}

	bEventsBound = true;
}

void UPREnemyCombatEventRelayComponent::HandleBossPhaseChanged(EPRFaerinPhase OldPhase, EPRFaerinPhase NewPhase)
{
	SendEventToOwner(PRGameplayTags::Event_Ability_PhaseTransition, static_cast<float>(NewPhase));
}

void UPREnemyCombatEventRelayComponent::HandleGroggyTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		SendEventToOwner(PRGameplayTags::Event_Ability_GroggyEntered);
	}
}

void UPREnemyCombatEventRelayComponent::HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		SendEventToOwner(PRGameplayTags::Event_Ability_Death);
	}
}

void UPREnemyCombatEventRelayComponent::SendEventToOwner(const FGameplayTag& EventTag, float EventMagnitude) const
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;
	Payload.EventMagnitude = EventMagnitude;

	// AbilityTriggers는 Actor로 전달된 GameplayEvent를 기준으로 발동한다.
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
}
