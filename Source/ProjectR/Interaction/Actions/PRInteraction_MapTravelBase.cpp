// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_MapTravelBase.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/World/PRSpawnPointTags.h"

UPRInteraction_MapTravelBase::UPRInteraction_MapTravelBase()
{
	// 맵 이동 실행 기본 설정
}

bool UPRInteraction_MapTravelBase::IsPartySyncActionLocked() const
{
	return bTravelInProgress;
}

void UPRInteraction_MapTravelBase::StartTravelToSpawnPoint(TSoftObjectPtr<UWorld> MapToTravel, FGameplayTag SpawnPointId, TSubclassOf<UGameplayEffect> OptionalGameplayEffect)
{
	UWorld* World = GetWorld();
	// 목적지 데이터는 EntranceGate 또는 Waypoint 같은 하위 클래스 소유
	if (bTravelInProgress || MapToTravel.IsNull() || !IsValid(World))
	{
		return;
	}

	if (World->GetTimerManager().IsTimerActive(TravelDelayTimerHandle))
	{
		return;
	}

	constexpr float TravelDelay = 1.5f;
	const FGameplayTag ResolvedSpawnPointId = SpawnPointId.IsValid() ? SpawnPointId : PRSpawnPointTags::SpawnPoint_Default;
	bTravelInProgress = true;
	// 이동 확정 이후 파티 대기 판정과 안내 메시지 정리
	ClearPartySyncCheckTimer();
	ClearPartySyncWaitingMessages();

	// 클라이언트 페이드아웃 및 이동 직전 효과 처리
	if (APRGameStateBase* GameState = World->GetGameState<APRGameStateBase>())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			APRPlayerController* Controller = Cast<APRPlayerController>(PlayerState->GetOwner());
			if (!IsValid(Controller))
			{
				continue;
			}

			Controller->ClientStartMapTransition(TravelDelay, EPRMapTransitionType::MapTravel);

			if (IsValid(OptionalGameplayEffect.Get()))
			{
				if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(PlayerState))
				{
					// 이동 직전 선택 효과 적용
					FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
					FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(OptionalGameplayEffect, 1.0f, EffectContext);
					ASC->BP_ApplyGameplayEffectSpecToSelf(SpecHandle);
				}
			}
		}
	}

	// ServerTravel 시작 예약
	World->GetTimerManager().SetTimer(TravelDelayTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, MapToTravel, ResolvedSpawnPointId]()
	{
		UWorld* TimerWorld = GetWorld();
		if (!IsValid(TimerWorld))
		{
			bTravelInProgress = false;
			return;
		}

		if (UPRGameInstance* GameInstance = TimerWorld->GetGameInstance<UPRGameInstance>())
		{
			FPRWorldSaveData PendingWorldSaveData;
			bool bHasPendingWorldSaveData = false;
			if (const APRGameStateBase* GameState = TimerWorld->GetGameState<APRGameStateBase>())
			{
				// GameInstance를 통한 ServerTravel 직후 저장 예약용 월드 진행 상태 생성
				PendingWorldSaveData = GameState->MakeWorldSaveData();
				bHasPendingWorldSaveData = true;
			}

			if (!GameInstance->ServerTravelToMap(MapToTravel, false))
			{
				bTravelInProgress = false;
				return;
			}

			if (bHasPendingWorldSaveData)
			{
				// 다음 월드 초기화 단계에서 적용할 진행 상태 예약
				GameInstance->SetPendingWorldSaveData(PendingWorldSaveData);
			}

			// 다음 월드 SpawnPoint 선택 단계에서 사용할 목적지 태그 예약
			GameInstance->SetPendingTravelSpawnPointId(ResolvedSpawnPointId);
			return;
		}

		bTravelInProgress = false;
	}), TravelDelay, false);
}
