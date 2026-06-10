// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "ProjectR/AbilitySystem/Abilities/Mod/PRGA_Mod_HasDuration.h"
#include "PRGA_Mod_SummonBarrier.generated.h"

class APRGroundBoxProjectileBase;
struct FGameplayEffectRemovalInfo;

// 배리어를 소환하고 재입력 시 발사하는 모드 어빌리티
UCLASS()
class PROJECTR_API UPRGA_Mod_SummonBarrier : public UPRGA_Mod_HasDuration
{
	GENERATED_BODY()

public:
	// 배리어 소환 모드 기본값 초기화
	UPRGA_Mod_SummonBarrier();

public:
	/*~ UGameplayAbility Interface ~*/
	// 활성 배리어 보유 시 비용 검사를 우회한다
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 활성 배리어 보유 시 추가 비용을 적용하지 않는다
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 입력 상태에 따라 소환 또는 발사를 실행한다
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 어빌리티 회수 시 활성 배리어를 정리한다
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	/*~ UPRGA_Mod_HasDuration Interface ~*/
	// 지속시간 비용 확정 후 서버 배리어를 생성한다
	virtual void OnDurationStarted_Implementation() override;

protected:
	// 활성 배리어 보유 여부 확인
	bool HasActiveBarrier() const;

	// 발사 입력 가능 여부 확인
	bool HasLaunchActivationWindow() const;

	// 서버 배리어 생성
	APRGroundBoxProjectileBase* SpawnBarrier(const FGameplayAbilityActorInfo* ActorInfo);

	// 활성 배리어 발사
	bool LaunchActiveBarrier(const FGameplayAbilityActorInfo* ActorInfo);

	// 배리어 제거 요청
	void RequestActiveBarrierEnd();

	// 런타임 바인딩 정리
	void CleanupBarrierRuntime();

	// 비용 GE 제거 이벤트 등록
	void BindDurationCostRemovalEvent();

	// 비용 GE 제거 이벤트 해제
	void UnbindDurationCostRemovalEvent();

	// 생존 태그 이벤트 등록
	void BindSurvivalTagEvents();

	// 생존 태그 이벤트 해제
	void UnbindSurvivalTagEvents();

	// 발사 방향 계산
	FVector ResolveLaunchDirection(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 비용 GE 제거 처리
	void HandleDurationCostRemoved(const FGameplayEffectRemovalInfo& RemovalInfo);

	// 생존 태그 변경 처리
	void HandleSurvivalTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 배리어 소멸 처리
	UFUNCTION()
	void HandleBarrierDestroyed(APRGroundBoxProjectileBase* DestroyedBarrier);

protected:
	// 생성할 배리어 액터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Barrier")
	TSubclassOf<APRGroundBoxProjectileBase> BarrierActorClass;

	// 플레이어 기준 생성 위치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Barrier")
	FVector SpawnOffset = FVector(180.0f, 0.0f, 0.0f);

	// 배리어 접촉 피해
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Barrier|Damage", meta = (ClampMin = "0.0"))
	float BarrierDamage = 0.0f;

	// 배리어 접촉 그로기 피해
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Barrier|Damage", meta = (ClampMin = "0.0"))
	float BarrierGroggyDamage = 0.0f;

	// 배리어 체력 오버라이드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Barrier|Health", meta = (ClampMin = "0.0"))
	float BarrierMaxHealth = 0.0f;

	// 배리어 발사 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Barrier|Launch", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 1800.0f;

private:
	// 현재 서버에서 유지 중인 배리어
	UPROPERTY(Transient)
	TObjectPtr<APRGroundBoxProjectileBase> ActiveBarrier;

	// 활성 지속시간 비용 GE 핸들
	FActiveGameplayEffectHandle ActiveDurationCostHandle;

	// 비용 GE 제거 이벤트 바인딩 핸들
	FDelegateHandle DurationCostRemovedDelegateHandle;

	// 사망 태그 이벤트 바인딩 핸들
	FDelegateHandle DeadTagEventHandle;

	// 다운 태그 이벤트 바인딩 핸들
	FDelegateHandle DownTagEventHandle;

	// 발사 요청 여부
	bool bLaunchRequested = false;
};
