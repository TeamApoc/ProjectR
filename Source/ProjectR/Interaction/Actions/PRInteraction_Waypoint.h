// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_Waypoint.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRInteraction_Waypoint : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	UPRInteraction_Waypoint();

	/*~ UPRInteractionAction Interface ~*/
	virtual void Execute_Implementation(AActor* Interactor) override;
	virtual void EndInteraction_Implementation(AActor* Interactor, bool bCanceled) override;

protected:
	// 목표 맵 이동 실행
	void StartTravel(TSoftObjectPtr<UWorld> MapToTravel);

private:
	// 이동 조건 검사
	void CheckTravelCondition();

	// 상호작용 플레이어 수 반환
	int32 CountInteractingPlayers() const;

	// 행동 가능 플레이어 수 반환
	int32 CountFightCapablePlayers() const;

	// 이동 대상 Waypoint 태그 반환
	FGameplayTag ResolveTargetWaypointId() const;

protected:
	// 이동 판정 지연
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Crystal", meta = (ClampMin = "0.0"))
	float TravelCheckDelay = 2.0f;
	
	// TODO: UI 선택으로 이전
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Crystal", meta = (ClampMin = "0.0"))
	TSoftObjectPtr<UWorld> TargetMap;

	// TODO: UI 선택으로 이전
	// 목적지 맵 진입 Waypoint 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Crystal")
	FGameplayTag TargetWaypointId;
	
	// 상호작용시 자원 충전 GE
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|Crystal")
	TSubclassOf<UGameplayEffect> WaypointGE;
	
private:
	FTimerHandle TravelCheckTimerHandle;
	FTimerHandle TravelDelayTimerHandle;
};
