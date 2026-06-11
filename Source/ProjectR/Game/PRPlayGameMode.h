// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "PRGameTypes.h"
#include "PRPlayGameMode.generated.h"

class APRPlayerController;
class APRPlayerState;
class USoundBase;
class UUserWidget;

// 인게임 GameMode. 호스트 프로세스에만 존재
// 로그인 승인, 게스트 캐릭터 페이로드 검증, 월드 상태 주입, 이탈 시 보상 커밋을 담당
UCLASS()
class PROJECTR_API APRPlayGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APRPlayGameMode();

	/*~ AGameModeBase Interface ~*/
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

public:
	// 월드 이벤트 반영. Waypoint 트리거·보스 처치 등에서 호출
	void ReportWaypointActivated(const FPRWaypointKey& WaypointKey);
	void ReportBossDefeated(FName BossId);

	// 접속 플레이어 캐릭터 페이로드를 검증하고 PlayerState에 주입
	// 반환값이 false면 호출측(PlayerController)이 ClientCharacterAccepted(false)로 거부 통지
	bool AcceptGuestCharacter(APRPlayerController* From, const FPRCharacterSaveData& Payload);

	// 보상 발생 이벤트에서 호출. 해당 오너 클라에게 즉시 Grant를 푸시
	// 같은 프레임 내 중복 지급은 호출자가 합산하여 1회로 보낸다
	void GrantRewardTo(APRPlayerController* Target, const FPRRewardGrant& Grant);

	// 플레이어 생존 상태가 바뀌었음을 알리고 전멸 여부를 평가한다.
	void NotifyPlayerSurvivalStateChanged(APRPlayerState* PlayerState);

	// 기본 웨이포인트 해금
	void UnlockDefaultWaypoints();

protected:
	// 페이로드 검증. 실패 시 OutReason에 사유 기록
	bool ValidateCharacterPayload(const FPRCharacterSaveData& Payload, FString& OutReason) const;

	// 파티 전원이 다운 또는 사망 상태인지 확인한다.
	void EvaluatePartyWipe();

	// 전멸 확정과 모든 전투 참여 플레이어 최종 사망 이벤트 전송
	void ConfirmPartyWipe();

	// 전멸 파티를 마지막 방문 Waypoint 기준으로 현재 월드에서 복구
	void RespawnParty();

	// 일반 스폰에 사용할 SpawnPoint 태그 결정
	FGameplayTag ResolvePlayerStartSpawnPointId() const;

	// 전멸 리스폰에 사용할 SpawnPoint 태그 결정
	FGameplayTag ResolvePartyRespawnSpawnPointId() const;

	// 현재 월드의 WorldId와 사용한 SpawnPointId GameState 기록
	void RecordCurrentWorldSpawnPoint(FGameplayTag SpawnPointId) const;

	// 현재 월드의 WorldId Registry 조회
	bool ResolveCurrentWorldId(FGameplayTag& OutWorldId) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|World")
	TArray<FPRWaypointKey> DefaultUnlockedWaypoints;

	// 호스트 시작 시 로드된 월드 세이브. InitGame에서 주입되어 GameState에 전달된다
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|World")
	FPRWorldSaveData HostWorldSave;

	// 페이로드 검증 상한 기본값. 추후 UPRGameplayConfig DataAsset으로 외부화
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Validation")
	int32 MaxCharacterLevel = 100;

	// 표시명 최대 길이
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Validation")
	int32 MaxDisplayNameLength = 24;

	// 전멸 이벤트 중복 발행을 막는 서버 플래그다.
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|Survival")
	bool bPartyWipeInProgress = false;

	// 전멸 확정 후 리스폰 지연
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Survival", meta = (ClampMin = "0.0"))
	float PartyWipeRespawnDelay = 5.0f;

	// 전멸 확정 시 전투 참여 클라이언트에게 한 번 재생할 효과음
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Survival|Audio")
	TObjectPtr<USoundBase> PartyWipeSound;

	// 전멸 확정 시 전투 참여 클라이언트에게 표시할 위젯
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Survival|UI")
	TSubclassOf<UUserWidget> PartyWipeWidgetClass;

	// 맵 이동 직후 최초 스폰에 사용할 SpawnPoint 태그
	FGameplayTag TravelSpawnPointId;

	// 전멸 리스폰 타이머 핸들
	FTimerHandle PartyWipeRespawnTimerHandle;
};
