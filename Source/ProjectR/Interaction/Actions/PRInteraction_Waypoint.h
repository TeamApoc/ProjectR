// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRInteraction_MapTravelBase.h"
#include "TimerManager.h"
#include "PRInteraction_Waypoint.generated.h"

class APRPlayerController;
class UGameplayEffect;
class UPRWorldDataAsset;
enum class EPRMapTransitionType : uint8;

// 호스트 UI 선택 결과로 목적지가 정해지는 웨이포인트 상호작용
UCLASS()
class PROJECTR_API UPRInteraction_Waypoint : public UPRInteraction_MapTravelBase
{
	GENERATED_BODY()

public:
	UPRInteraction_Waypoint();

	// 호스트 선택 웨이포인트 목적지 이동 요청
	void RequestWaypointTravel(APRPlayerController* RequestingController, FSoftObjectPath WorldDataAssetPath, FGameplayTag WaypointId);

	// 호스트 웨이포인트 Travel UI 닫힘 처리
	void CancelWaypointTravel(APRPlayerController* RequestingController);

	// 목적지 선택 UI 입력 대기 여부 반환
	bool IsWaitingForWaypointTravelSelection() const { return bWaitingForWaypointTravelSelection; }

protected:
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void EndInteraction_Implementation(AActor* Interactor, bool bCanceled) override;

	/*~ UPRInteraction_PartySyncBase Interface ~*/
	// Waypoint 파티 동기화 조건 충족 시 호스트 Travel UI 표시
	virtual void HandlePartySyncConditionMet() override;

	// Travel UI 대기 또는 맵 이동 중 중복 파티 동기화 차단 여부
	virtual bool IsPartySyncActionLocked() const override;

	// Waypoint 상호작용 시작 이벤트 전송
	virtual void NotifyPartySyncInteractionStarted(AActor* Interactor) override;

	// Waypoint 상호작용 종료 이벤트 전송
	virtual void NotifyPartySyncInteractionEnded(AActor* Interactor, bool bCanceled) override;

	// 전체 플레이어 상호작용 잠금
	void LockPlayerInteraction();

	// 전체 플레이어 상호작용 잠금 해제
	void UnlockPlayerInteraction();

private:
	// Waypoint 활성 상태와 체크포인트 상태 갱신
	void RecordWaypointActivation();

	// 전원 FadeOut 이후 호스트 Travel UI 표시 예약
	void ScheduleWaypointTravelUI();

	// FadeOut 종료 후 호스트 Travel UI 표시
	void OpenWaypointTravelUIAfterFadeOut();

	// 호스트 Travel UI 열기
	bool OpenWaypointTravelUI();

	// 모든 플레이어 맵 전환 연출 알림
	void NotifyMapTransitionToAllPlayers(EPRMapTransitionType TransitionType) const;

	// 서버 월드에서 호스트 컨트롤러 조회
	APRPlayerController* FindHostPlayerController() const;

	// 목적지 월드 데이터 에셋과 웨이포인트 ID 검증
	bool ValidateWaypointTravelRequest(FSoftObjectPath WorldDataAssetPath, FGameplayTag WaypointId, UPRWorldDataAsset*& OutWorldDataAsset) const;

	// 모든 플레이어 Waypoint 취소 처리
	void BroadcastWaypointCancelEventToAllPlayers() const;

private:
	// 호스트 UI 목적지 선택 대기 상태
	bool bWaitingForWaypointTravelSelection = false;

	// 호스트 Travel UI 표시 전 FadeOut 대기 상태
	bool bWaitingForTravelUIFadeOut = false;

	// 전체 플레이어 상호작용 차단 태그 적용 상태
	bool bInteractionLocked = false;

	// 호스트 Travel UI 표시 예약 타이머
	FTimerHandle TravelUIOpenTimerHandle;

	// Waypoint 이동 직전 자원 충전 또는 회복 GameplayEffect
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Waypoint", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> WaypointGameplayEffect;

	// 호스트 Travel UI 표시 전 FadeOut 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Waypoint", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float TravelUIFadeDuration = 1.6f;
};
