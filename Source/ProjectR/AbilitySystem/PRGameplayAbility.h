// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "PRGameplayAbility.generated.h"

// 프로젝트 공통 어빌리티 베이스. 모든 어빌리티는 이 클래스를 상속
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                                 const FGameplayAbilityActorInfo* ActorInfo,
	                                 const FGameplayTagContainer* SourceTags,
	                                 const FGameplayTagContainer* TargetTags,
	                                 FGameplayTagContainer* OptionalRelevantTags) const override;

public:
	// ActivationPolicy 조회. ASC OnGiveAbility 및 입력 라우터에서 사용
	EPRAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	// InputTag 조회
	const FGameplayTag& GetInputTag() const { return InputTag; }

protected:
	// 본 어빌리티의 활성화 정책
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ability")
	EPRAbilityActivationPolicy ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;

	// 플레이어 입력 바인딩 태그. ServerInvoked/OnGiven은 비워둠
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ability", meta = (Categories = "Input.Ability"))
	FGameplayTag InputTag;
};
