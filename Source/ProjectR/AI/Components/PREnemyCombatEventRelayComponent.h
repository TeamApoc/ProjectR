// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PREnemyCombatEventRelayComponent.generated.h"

class UAbilitySystemComponent;

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

	void HandleGroggyTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void SendEventToOwner(const FGameplayTag& EventTag, float EventMagnitude = 0.0f) const;

protected:
	bool bEventsBound = false;
};
