// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_Waypoint.h"

#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/World/PRWaypointActor.h"

UPRInteraction_Waypoint::UPRInteraction_Waypoint()
{
	InteractionType = EPRInteractionType::Sustained;
	bRequiresRange = true;
}

void UPRInteraction_Waypoint::OnTravelConditionMet()
{
	APRGameStateBase* GameState = GetWorld()->GetGameState<APRGameStateBase>();
	const APRWaypointActor* WaypointActor = Cast<APRWaypointActor>(GetOwner());
	if (!IsValid(GameState) || !IsValid(WaypointActor))
	{
		return;
	}

	const FGameplayTag SpawnPointId = WaypointActor->GetSpawnPointId();

	// 활성 Waypoint 기록
	GameState->SetLastActiveWaypointId(SpawnPointId);

	// 전멸 리스폰 기준 체크포인트 기록
	GameState->SetActiveCheckpoint(SpawnPointId);
}

void UPRInteraction_Waypoint::NotifyTravelInteractionStarted(AActor* Interactor)
{
	Super::NotifyTravelInteractionStarted(Interactor);
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		// Waypoint 활성화 시작 이벤트 전송
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_Start);
	}
}

void UPRInteraction_Waypoint::NotifyTravelInteractionEnded(AActor* Interactor, bool bCanceled)
{
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		if (bCanceled)
		{
			// Waypoint 활성화 어빌리티 취소
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(PRGameplayTags::Ability_Player_Waypoint);
			ASC->CancelAbilities(&CancelTags);
		}
		else
		{
			// Waypoint 상호작용 종료 이벤트 전송
			ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_End);
		}
	}
	Super::NotifyTravelInteractionEnded(Interactor, bCanceled);
}
