// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_MapTravelBase.generated.h"

class UGameplayEffect;

UCLASS(Abstract)
class PROJECTR_API UPRInteraction_MapTravelBase : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	UPRInteraction_MapTravelBase();

	/*~ UPRInteractionAction Interface ~*/
	virtual void Execute_Implementation(AActor* Interactor) override;
	virtual void EndInteraction_Implementation(AActor* Interactor, bool bCanceled) override;

protected:
	// 이동 조건이 성립한 뒤 월드 진행 상태를 갱신하기 위한 후크
	virtual void OnTravelConditionMet();

	// 상호작용 시작 피드백을 하위 클래스에서 실행하기 위한 후크
	virtual void NotifyTravelInteractionStarted(AActor* Interactor);

	// 상호작용 종료 피드백을 하위 클래스에서 실행하기 위한 후크
	virtual void NotifyTravelInteractionEnded(AActor* Interactor, bool bCanceled);

	// 이동 대상 SpawnPoint 태그 반환
	FGameplayTag ResolveTargetSpawnPointId() const;

	// 목표 맵 이동 실행
	void StartTravel(TSoftObjectPtr<UWorld> MapToTravel);

private:
	// 이동 조건 검사
	void CheckTravelCondition();

	// 상호작용 플레이어 수 반환
	int32 CountInteractingPlayers() const;

	// 행동 가능 플레이어 수 반환
	int32 CountFightCapablePlayers() const;

protected:
	// 이동 판정 지연
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Travel", meta = (ClampMin = "0.0"))
	float TravelCheckDelay = 2.0f;
	
	// 이동 대상 맵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Travel", meta = (ClampMin = "0.0"))
	TSoftObjectPtr<UWorld> TargetMap;

	// 목적지 맵 진입 SpawnPoint 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Travel")
	FGameplayTag TargetSpawnPointId;
	
	// 상호작용 시 자원 충전 또는 회복에 사용할 GameplayEffect
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Travel")
	TSubclassOf<UGameplayEffect> TravelGameplayEffect;
	
private:
	FTimerHandle TravelCheckTimerHandle;
	FTimerHandle TravelDelayTimerHandle;
};
