// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_MapTravelBase.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/World/PRSpawnPointTags.h"

UPRInteraction_MapTravelBase::UPRInteraction_MapTravelBase()
{
	InteractionType = EPRInteractionType::Sustained;
	bRequiresRange = true;
}

void UPRInteraction_MapTravelBase::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

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

	NotifyTravelInteractionStarted(Interactor);

	if (TravelCheckDelay <= 0.0f)
	{
		// 즉시 이동 조건 검사
		CheckTravelCondition();
		return;
	}

	// 지연 후 이동 조건 검사
	World->GetTimerManager().SetTimer(
		TravelCheckTimerHandle,
		this,
		&UPRInteraction_MapTravelBase::CheckTravelCondition,
		TravelCheckDelay,
		false);
}

void UPRInteraction_MapTravelBase::EndInteraction_Implementation(AActor* Interactor, bool bCanceled)
{
	Super::EndInteraction_Implementation(Interactor, bCanceled);

	NotifyTravelInteractionEnded(Interactor, bCanceled);
	
	if (UWorld* World = GetWorld())
	{
		// 이동 판정 대기 취소
		World->GetTimerManager().ClearTimer(TravelCheckTimerHandle);
	}
}

void UPRInteraction_MapTravelBase::OnTravelConditionMet()
{
	// 기본 구현 없음
}

void UPRInteraction_MapTravelBase::NotifyTravelInteractionStarted(AActor* Interactor)
{
	if (IsValid(Interactor))
	{
		UE_LOG(LogTemp,Warning,TEXT("%s: Travel Interaction Started"),*Interactor->GetName());	
	}
}

void UPRInteraction_MapTravelBase::NotifyTravelInteractionEnded(AActor* Interactor, bool bCanceled)
{
	if (IsValid(Interactor))
	{
		UE_LOG(LogTemp,Warning,TEXT("%s: Travel Interaction Ended"),*Interactor->GetName());	
	}
}

FGameplayTag UPRInteraction_MapTravelBase::ResolveTargetSpawnPointId() const
{
	// 선택 UI 미구현 기본값
	return TargetSpawnPointId.IsValid() ? TargetSpawnPointId : PRSpawnPointTags::SpawnPoint_Default;
}

void UPRInteraction_MapTravelBase::StartTravel(TSoftObjectPtr<UWorld> MapToTravel)
{
	if (MapToTravel.IsNull())
	{
		return;
	}
	
	constexpr float TravelDelay = 1.5f;
		
	// 클라이언트 페이드아웃
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
			
			if (IsValid(TravelGameplayEffect.Get()))
			{
				if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(PlayerState))
				{
					// 이동 직전 자원 회복 효과 적용
					FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
					FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(TravelGameplayEffect, 1.0f, EffectContext);
					ASC->BP_ApplyGameplayEffectSpecToSelf(SpecHandle);
				}
			}
		}
	}
		
	// ServerTravel 시작 예약
	GetWorld()->GetTimerManager().SetTimer(TravelDelayTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, MapToTravel]()
	{
		if (UPRGameInstance* GameInstance = GetWorld()->GetGameInstance<UPRGameInstance>())
		{
			if (const APRGameStateBase* GameState = GetWorld()->GetGameState<APRGameStateBase>())
			{
				// 월드 진행 상태 예약
				GameInstance->SetPendingWorldSaveData(GameState->MakeWorldSaveData());
			}

			// 목적지 SpawnPoint 예약
			GameInstance->SetPendingTravelSpawnPointId(ResolveTargetSpawnPointId());
			GameInstance->ServerTravelToMap(MapToTravel, false);
		}	
	}), TravelDelay, false);
}

void UPRInteraction_MapTravelBase::CheckTravelCondition()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	// 행동 가능한 인원 전체의 상호작용 유지 여부 검사
	const int32 FightCapablePlayerCount = CountFightCapablePlayers();
	if (FightCapablePlayerCount > 0 && CountInteractingPlayers() == FightCapablePlayerCount)
	{
		OnTravelConditionMet();
		StartTravel(TargetMap);
	}
}

int32 UPRInteraction_MapTravelBase::CountInteractingPlayers() const
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

int32 UPRInteraction_MapTravelBase::CountFightCapablePlayers() const
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
