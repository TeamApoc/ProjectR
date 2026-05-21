// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteraction_Waypoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/World/PRWaypointActor.h"
#include "ProjectR/World/PRWaypointTags.h"

UPRInteraction_Waypoint::UPRInteraction_Waypoint()
{
	InteractionType = EPRInteractionType::Sustained;
	bRequiresRange = true;
}

void UPRInteraction_Waypoint::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	// 아래 코드는 서버에서만 실행됨
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(TravelCheckTimerHandle))
	{
		return;
	}
	
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		// 서버와 클라 모두에게 Trigger
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_Start);
	}

	if (TravelCheckDelay <= 0.0f)
	{
		// 즉시 이동
		CheckTravelCondition();
		return;
	}

	// Delay 이후 체크
	World->GetTimerManager().SetTimer(
		TravelCheckTimerHandle,
		this,
		&UPRInteraction_Waypoint::CheckTravelCondition,
		TravelCheckDelay,
		false);
}

void UPRInteraction_Waypoint::EndInteraction_Implementation(AActor* Interactor, bool bCanceled)
{
	Super::EndInteraction_Implementation(Interactor, bCanceled);
	
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		// bCanceled는 Interaction 어빌리티가 Cancel된 경우 true
		if (bCanceled)
		{
			// Waypoint 활성화 어빌리티를 취소
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(PRGameplayTags::Ability_Player_Waypoint);
			ASC->CancelAbilities(&CancelTags);
		}
		else
		{
			// Waypoint 활성화 어빌리티 내에서 손을 떼는 모션 재생 후 EndAbility 하기위해 이벤트 전송
			ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_End);
		}
	}
	
	GetWorld()->GetTimerManager().ClearTimer(TravelCheckTimerHandle);
}

void UPRInteraction_Waypoint::StartTravel(TSoftObjectPtr<UWorld> MapToTravel)
{
	if (MapToTravel.IsNull())
	{
		return;
	}
	
	constexpr float TravelDelay = 1.5f;
		
	// 클라이언트 FadeOut
	if (APRGameStateBase* GameState = GetWorld()->GetGameState<APRGameStateBase>())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			APRPlayerController* Controller = Cast<APRPlayerController>(PlayerState->GetOwner());
			if (!IsValid(Controller))
			{
				continue;
			}
				
			Controller->ClientStartMapTransition(TravelDelay, EPRMapTransitionType::MapTravel);
			
			// TODO: Travel을 안하고 UI를 닫은 경우 사망 플레이어를 웨이포인트 근처로 리스폰 시켜야 함
			if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(PlayerState))
			{
				FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(WaypointGE,1.0f,EffectContext);
				ASC->BP_ApplyGameplayEffectSpecToSelf(SpecHandle);
			}
			
			// TODO: 무적 상태 추가?
		}
	}
		
	// ServerTravel 시작
	GetWorld()->GetTimerManager().SetTimer(TravelDelayTimerHandle, FTimerDelegate::CreateWeakLambda(this,[this, MapToTravel]()
	{
		if (UPRGameInstance* GameInstance = GetWorld()->GetGameInstance<UPRGameInstance>())
		{
			if (const APRGameStateBase* GameState = GetWorld()->GetGameState<APRGameStateBase>())
			{
				// 월드 진행 상태 예약
				GameInstance->SetPendingWorldSaveData(GameState->MakeWorldSaveData());
			}

			// 목적지 Waypoint 예약
			GameInstance->SetPendingTravelWaypointId(ResolveTargetWaypointId());
			GameInstance->ServerTravelToMap(MapToTravel, false);
		}	
	}),TravelDelay,false);
}

void UPRInteraction_Waypoint::CheckTravelCondition()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	// 행동 가능한 인원이 모두 웨이포인트와 상호작용하는지 체크
	const int32 FightCapablePlayerCount = CountFightCapablePlayers();
	if (FightCapablePlayerCount > 0 && CountInteractingPlayers() == FightCapablePlayerCount)
	{
		// 활성 Waypoint 갱신
		if (APRGameStateBase* GameState = GetWorld()->GetGameState<APRGameStateBase>())
		{
			if (const APRWaypointActor* WaypointActor = Cast<APRWaypointActor>(GetOwner()))
			{
				// 활성 Waypoint 기록
				GameState->SetLastActiveWaypointId(WaypointActor->GetWaypointId());
			}
		}
	
		// TODO: 현재는 TargetMap을 Property에서 지정하지만, 추후 이동장소 선택 UI를 띄움
		StartTravel(TargetMap);
	}
}

FGameplayTag UPRInteraction_Waypoint::ResolveTargetWaypointId() const
{
	// 선택 UI 미구현 기본값
	return TargetWaypointId.IsValid() ? TargetWaypointId : PRWaypointTags::Waypoint_Default;
}

int32 UPRInteraction_Waypoint::CountInteractingPlayers() const
{
	const APRGameStateBase* GameState = GetWorld()->GetGameState<APRGameStateBase>();
	if (!IsValid(GameState))
	{
		return 0;
	}

	int32 InteractingPlayerCount = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant() || PRPlayerState->IsOutOfFight())
		{
			continue;
		}

		AController* Controller = Cast<AController>(PRPlayerState->GetOwner());
		if (!IsValid(Controller) && IsValid(PRPlayerState->GetPawn()))
		{
			Controller = PRPlayerState->GetPawn()->GetController();
		}

		const UPRInteractorComponent* InteractorComponent = IsValid(Controller)
			? Controller->FindComponentByClass<UPRInteractorComponent>()
			: nullptr;
		if (IsValid(InteractorComponent) && InteractorComponent->GetActiveAction() == this)
		{
			++InteractingPlayerCount;
		}
	}

	return InteractingPlayerCount;
}

int32 UPRInteraction_Waypoint::CountFightCapablePlayers() const
{
	UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return 0;
	}

	int32 FightCapablePlayerCount = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant() || PRPlayerState->IsOutOfFight())
		{
			continue;
		}

		++FightCapablePlayerCount;
	}

	return FightCapablePlayerCount;
}
