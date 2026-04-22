// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PREnemyCombatEventRelayComponent.generated.h"

class UAbilitySystemComponent;

// ASC의 상태 태그 변화를 GameplayEvent로 바꿔주는 서버 전용 릴레이 컴포넌트다.
// Groggy/Death/PhaseTransition Ability는 이 이벤트를 트리거로 실행된다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyCombatEventRelayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREnemyCombatEventRelayComponent();

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void HandleBossPhaseChanged(EPRFaerinPhase OldPhase, EPRFaerinPhase NewPhase);

	// State 태그가 붙는 순간만 이벤트로 변환한다. 태그 제거 시점은 Ability 종료 흐름에서 처리한다.
	void HandleGroggyTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Owner ASC로 GameplayEvent를 보내서 AbilityTriggers가 반응하도록 한다.
	void SendEventToOwner(const FGameplayTag& EventTag, float EventMagnitude = 0.0f) const;

protected:
	// BeginPlay 중복 바인딩을 막는다.
	bool bEventsBound = false;
};
