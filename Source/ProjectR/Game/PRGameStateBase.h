// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameStateBase.h"
#include "PRGameTypes.h"
#include "PRGameStateBase.generated.h"

class APRPlayerCharacter;

// 체크포인트 활성화 시 발행
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCheckpointActivatedSignature, FName, CheckpointId);

// 보스 처치 시 발행
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossDefeatedSignature, FName, BossId);

// 월드 진행 상태의 복제 컨테이너. 서버가 권위를 가지며 모든 클라이언트에 동일 상태로 복제
UCLASS()
class PROJECTR_API APRGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 현재 활성 체크포인트 조회
	FName GetActiveCheckpoint() const { return ActiveCheckpoint; }

	// 마지막 활성 Waypoint 태그 조회
	FGameplayTag GetLastActiveWaypointId() const { return LastActiveWaypointId; }

	// 보스 처치 여부 조회
	bool IsBossDefeated(FName BossId) const;

	// 현재 세션에 등록된 PlayerState의 Pawn을 PRPlayerCharacter로 캐스팅해 수집. 캐스팅 실패나 미possess 상태는 제외
	TArray<APRPlayerCharacter*> GetPlayerCharacters() const;

	// 서버 전용. 월드 세이브 스냅샷을 GameState에 주입. 호스트 맵 로드 직후 GameMode가 호출
	void InitializeFromWorldSave(const FPRWorldSaveData& WorldSave);

	// 현재 월드 진행 상태 저장 데이터
	FPRWorldSaveData MakeWorldSaveData() const;

	// 서버 전용. 체크포인트 활성화 반영
	void SetActiveCheckpoint(FName CheckpointId);

	// 서버 전용. 마지막 활성 Waypoint 태그 반영
	void SetLastActiveWaypointId(FGameplayTag WaypointId);

	// 서버 전용. 마지막 활성 Waypoint 태그 초기화
	void ClearLastActiveWaypointId();

	// 서버 전용. 보스 처치 반영
	void MarkBossDefeated(FName BossId);

protected:
	// ActiveCheckpoint 복제 콜백. 클라이언트에서 UI·리스폰 지점 갱신 트리거
	UFUNCTION()
	void OnRep_ActiveCheckpoint(FName OldCheckpoint);

public:
	// 체크포인트 활성화 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnCheckpointActivatedSignature OnCheckpointActivated;

	// 보스 처치 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnBossDefeatedSignature OnBossDefeated;

protected:
	// 현재 활성 체크포인트. 변경 시 OnRep으로 UI/리스폰 지점 갱신
	UPROPERTY(ReplicatedUsing = OnRep_ActiveCheckpoint)
	FName ActiveCheckpoint;

	// 이번 세션에서 해금된 체크포인트 집합
	UPROPERTY(Replicated)
	TArray<FName> UnlockedCheckpoints;

	// 마지막 활성 Waypoint 태그
	UPROPERTY(Replicated)
	FGameplayTag LastActiveWaypointId;

	// 처치된 보스 ID. 재도전 트리거·컷신 분기용
	UPROPERTY(Replicated)
	TArray<FName> DefeatedBosses;

	// 호스트 기준 월드 세이브 버전. 디버그·불일치 진단용
	UPROPERTY(Replicated)
	EPRSaveVersion WorldSaveVersion = EPRSaveVersion::None;
};
