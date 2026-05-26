// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_PenitentBarrierSummon.generated.h"

class APRPenitentBarrierActor;

// Penitent 배리어 소환 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentBarrierSummon : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	// Ability 태그와 차단 태그 초기화
	UPRGameplayAbility_PenitentBarrierSummon();

	/*~ UGameplayAbility Interface ~*/
	// 배리어 소환 가능 여부 확인
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	// 배리어 액터 생성과 부착 실행
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 생성할 배리어 액터 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier")
	TSubclassOf<APRPenitentBarrierActor> BarrierActorClass;

	// 배리어 부착 소켓 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier")
	FName BarrierAttachSocketName = TEXT("Barrier");

	// 소켓이 없을 때 사용할 전방 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier")
	FVector BarrierSpawnOffset = FVector(120.0f, 0.0f, 80.0f);
};
