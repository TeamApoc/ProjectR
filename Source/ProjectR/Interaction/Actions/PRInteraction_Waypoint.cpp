// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_Waypoint.h"

#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/World/PRWorldDataAsset.h"
#include "ProjectR/World/PRWaypointActor.h"
#include "UObject/ConstructorHelpers.h"

UPRInteraction_Waypoint::UPRInteraction_Waypoint()
{
	// Waypoint 파티 동기화 기본 설정
	static ConstructorHelpers::FObjectFinder<UBlueprint> WaypointGameplayEffectBlueprint(
		TEXT("/Script/Engine.Blueprint'/Game/0_BP/Player/GE/GE_Waypoint.GE_Waypoint'"));
	if (WaypointGameplayEffectBlueprint.Succeeded()
		&& IsValid(WaypointGameplayEffectBlueprint.Object)
		&& IsValid(WaypointGameplayEffectBlueprint.Object->GeneratedClass)
		&& WaypointGameplayEffectBlueprint.Object->GeneratedClass->IsChildOf(UGameplayEffect::StaticClass()))
	{
		// 기본 Waypoint 회복 효과 클래스
		WaypointGameplayEffect = WaypointGameplayEffectBlueprint.Object->GeneratedClass;
	}
}

void UPRInteraction_Waypoint::RequestWaypointTravel(APRPlayerController* RequestingController, FSoftObjectPath WorldDataAssetPath, FGameplayTag WaypointId)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!IsValid(RequestingController) || !RequestingController->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: invalid host controller"));
		return;
	}

	UPRWorldDataAsset* WorldDataAsset = nullptr;
	if (!ValidateWaypointTravelRequest(WorldDataAssetPath, WaypointId, WorldDataAsset))
	{
		return;
	}

	// UI 선택 대기 종료
	bWaitingForWaypointTravelSelection = false;
	bWaitingForTravelUIFadeOut = false;
	if (UWorld* World = GetWorld())
	{
		// Travel UI 오픈 예약 정리
		World->GetTimerManager().ClearTimer(TravelUIOpenTimerHandle);
	}
	RequestingController->ClearPendingWaypointTravelInteraction(this);
	UnlockPlayerInteraction();

	// 선택 노드 목적지 이동과 Waypoint 전용 효과 적용
	StartTravelToSpawnPoint(WorldDataAsset->MapAsset, WaypointId, WaypointGameplayEffect);
}

void UPRInteraction_Waypoint::CancelWaypointTravel(APRPlayerController* RequestingController)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!IsValid(RequestingController) || !RequestingController->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel cancel rejected: invalid host controller"));
		return;
	}

	if (!bWaitingForWaypointTravelSelection)
	{
		return;
	}

	// UI 선택 대기 취소
	bWaitingForWaypointTravelSelection = false;
	bWaitingForTravelUIFadeOut = false;
	if (UWorld* World = GetWorld())
	{
		// Travel UI 오픈 예약 정리
		World->GetTimerManager().ClearTimer(TravelUIOpenTimerHandle);
	}
	RequestingController->ClearPendingWaypointTravelInteraction(this);
	BroadcastWaypointCancelEventToAllPlayers();
	NotifyMapTransitionToAllPlayers(EPRMapTransitionType::CancelTravel);
	ClearPartySyncWaitingMessages();
	UnlockPlayerInteraction();
}

bool UPRInteraction_Waypoint::CanInteract_Implementation(AActor* Interactor) const
{
	if (auto ASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor))
	{
		return !ASC->HasMatchingGameplayTag(PRGameplayTags::State_Block_Interaction);
	}

	return Super::CanInteract_Implementation(Interactor);
}

void UPRInteraction_Waypoint::EndInteraction_Implementation(AActor* Interactor, bool bCanceled)
{
	if (!bWaitingForTravelUIFadeOut && !bWaitingForWaypointTravelSelection)
	{
		// UI 전환 외부 종료에 따른 잠금 해제
		UnlockPlayerInteraction();
	}

	Super::EndInteraction_Implementation(Interactor, bCanceled);
}

void UPRInteraction_Waypoint::HandlePartySyncConditionMet()
{
	if (bWaitingForTravelUIFadeOut || bWaitingForWaypointTravelSelection)
	{
		return;
	}

	RecordWaypointActivation();
	ClearPartySyncWaitingMessages();

	// 호스트 목적지 선택 대기
	LockPlayerInteraction();
	ScheduleWaypointTravelUI();
}

bool UPRInteraction_Waypoint::IsPartySyncActionLocked() const
{
	// Travel UI 전환 중 상호작용 입력 해제에 따른 중복 종료 피드백 차단
	return Super::IsPartySyncActionLocked() || bWaitingForTravelUIFadeOut || bWaitingForWaypointTravelSelection;
}

void UPRInteraction_Waypoint::RecordWaypointActivation()
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

void UPRInteraction_Waypoint::NotifyPartySyncInteractionStarted(AActor* Interactor)
{
	Super::NotifyPartySyncInteractionStarted(Interactor);
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		// Waypoint 활성화 시작 이벤트 전송
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_Start);
	}
}

void UPRInteraction_Waypoint::NotifyPartySyncInteractionEnded(AActor* Interactor, bool bCanceled)
{
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		// Waypoint 상호작용 종료 이벤트 전송
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_End);
	}
	Super::NotifyPartySyncInteractionEnded(Interactor, bCanceled);
}

void UPRInteraction_Waypoint::LockPlayerInteraction()
{
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(PlayerState));
		if (!IsValid(ASC))
		{
			continue;
		}

		ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Interaction);
	}

	bInteractionLocked = true;
}

void UPRInteraction_Waypoint::UnlockPlayerInteraction()
{
	if (!bInteractionLocked)
	{
		return;
	}

	for (APlayerState* PlayerState : GetPlayerArray())
	{
		UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(PlayerState));
		if (!IsValid(ASC))
		{
			continue;
		}

		ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Interaction);
	}

	bInteractionLocked = false;
}

void UPRInteraction_Waypoint::ScheduleWaypointTravelUI()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		bWaitingForTravelUIFadeOut = false;
		UnlockPlayerInteraction();
		return;
	}

	bWaitingForTravelUIFadeOut = true;

	// 전원 FadeOut 알림
	NotifyMapTransitionToAllPlayers(EPRMapTransitionType::MapTravel);

	World->GetTimerManager().ClearTimer(TravelUIOpenTimerHandle);
	if (TravelUIFadeDuration <= 0.0f)
	{
		OpenWaypointTravelUIAfterFadeOut();
		return;
	}

	// FadeOut 종료 후 호스트 UI 표시 예약
	World->GetTimerManager().SetTimer(
		TravelUIOpenTimerHandle,
		this,
		&UPRInteraction_Waypoint::OpenWaypointTravelUIAfterFadeOut,
		TravelUIFadeDuration,
		false);
}

void UPRInteraction_Waypoint::OpenWaypointTravelUIAfterFadeOut()
{
	if (!bWaitingForTravelUIFadeOut)
	{
		return;
	}

	bWaitingForTravelUIFadeOut = false;
	bWaitingForWaypointTravelSelection = OpenWaypointTravelUI();
	if (!bWaitingForWaypointTravelSelection)
	{
		// UI 오픈 실패 후 Fade 복구
		NotifyMapTransitionToAllPlayers(EPRMapTransitionType::CancelTravel);
		UnlockPlayerInteraction();
	}
}

bool UPRInteraction_Waypoint::OpenWaypointTravelUI()
{
	APRPlayerController* HostController = FindHostPlayerController();
	if (!IsValid(HostController))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel UI open failed: host controller not found"));
		return false;
	}

	// ActiveAction 해제 이후 서버 RPC 대상 복구용 참조 등록
	HostController->SetPendingWaypointTravelInteraction(this);

	// 호스트 로컬 클라이언트 UI 열기
	HostController->ClientOpenWaypointTravelUI();
	return true;
}

void UPRInteraction_Waypoint::NotifyMapTransitionToAllPlayers(EPRMapTransitionType TransitionType) const
{
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		APRPlayerController* Controller = Cast<APRPlayerController>(PlayerState->GetOwner());
		if (!IsValid(Controller) && IsValid(PlayerState->GetPawn()))
		{
			Controller = Cast<APRPlayerController>(PlayerState->GetPawn()->GetController());
		}

		if (!IsValid(Controller))
		{
			continue;
		}

		// 로컬 맵 전환 페이드 알림
		Controller->ClientNotifyMapTransition(TravelUIFadeDuration, TransitionType);
	}
}

APRPlayerController* UPRInteraction_Waypoint::FindHostPlayerController() const
{
	for (APlayerState* PlayerState :GetPlayerArray())
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(PlayerState->GetOwner());
		if (!IsValid(PlayerController) && IsValid(PlayerState->GetPawn()))
		{
			PlayerController = Cast<APRPlayerController>(PlayerState->GetPawn()->GetController());
		}

		if (IsValid(PlayerController) && PlayerController->IsLocalController())
		{
			// 리슨 서버 호스트 컨트롤러
			return PlayerController;
		}
	}

	return nullptr;
}

bool UPRInteraction_Waypoint::ValidateWaypointTravelRequest(FSoftObjectPath WorldDataAssetPath, FGameplayTag WaypointId, UPRWorldDataAsset*& OutWorldDataAsset) const
{
	OutWorldDataAsset = nullptr;

	if (!bWaitingForWaypointTravelSelection)
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: UI selection is not pending"));
		return false;
	}

	if (!WorldDataAssetPath.IsValid() || !WaypointId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: invalid destination data"));
		return false;
	}

	// UI 임시 에셋 경로 서버 로드
	UPRWorldDataAsset* WorldDataAsset = Cast<UPRWorldDataAsset>(WorldDataAssetPath.TryLoad());
	if (!IsValid(WorldDataAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: world data asset load failed"));
		return false;
	}

	if (WorldDataAsset->MapAsset.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: target map is null"));
		return false;
	}

	if (!WorldDataAsset->HasWaypointNode(WaypointId))
	{
		UE_LOG(LogTemp, Warning, TEXT("WaypointTravel rejected: waypoint node not found %s"), *WaypointId.ToString());
		return false;
	}

	OutWorldDataAsset = WorldDataAsset;
	return true;
}

void UPRInteraction_Waypoint::BroadcastWaypointCancelEventToAllPlayers() const
{
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(PlayerState));
		if (!IsValid(ASC))
		{
			continue;
		}

		// 상호작용 어빌리티 취소
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Interaction);
		ASC->CancelAbilities(&CancelTags);

		// Waypoint 종료 이벤트 전파
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Waypoint_End);
	}
}
