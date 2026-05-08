// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRGA_PlayerHitReact.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class APRPlayerCharacter;

UENUM(BlueprintType)
enum class EPRPlayerHitReactType : uint8
{
	None,
	Weak,
	Strong,
	Down
};

enum class EPRPlayerDownHitReactPhase : uint8
{
	None,
	Start,
	FallLoop,
	Land,
	GetUp
};

// 플레이어 강인도 임계값 도달 이벤트를 받아 경직 몽타주와 행동 제한을 처리하는 Ability다.
UCLASS()
class PROJECTR_API UPRGA_PlayerHitReact : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_PlayerHitReact();

	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 피격 리액션 몽타주가 정상 종료되었을 때 입력 제한을 해제하고 Ability를 종료한다.
	UFUNCTION()
	void HandleHitReactMontageCompleted();

	// 피격 리액션 몽타주가 끊겼을 때 입력 제한을 해제하고 Ability를 종료한다.
	UFUNCTION()
	void HandleHitReactMontageInterrupted();

	// 플레이어가 다운 중 착지했을 때 Land 섹션 진입을 예약하거나 실행한다.
	void HandlePlayerLanded(const FHitResult& Hit);

	// 다운 몽타주의 섹션 전환을 감지하고 착지 여부에 따라 다음 섹션을 결정한다.
	void HandleDownMontageSectionChanged(UAnimMontage* Montage, FName SectionName, bool bLooped);

	// 이벤트 태그를 피격 리액션 타입으로 변환한다.
	EPRPlayerHitReactType ResolveHitReactType(const FGameplayEventData* TriggerEventData) const;

	// 리액션 타입에 맞는 몽타주를 선택한다.
	UAnimMontage* SelectHitReactMontage(EPRPlayerHitReactType HitReactType) const;

	// 현재 무기 타입에 맞는 약한 경직 몽타주를 선택한다.
	UAnimMontage* SelectWeakHitReactMontage() const;
	
	// 현재 무기 타입에 맞는 강한 경직 몽타주를 선택한다.
	UAnimMontage* SelectStrongHitReactMontage() const;

	// 리액션 타입에 맞춰 기존 플레이어 행동을 취소한다.
	void CancelActionsForHitReact(EPRPlayerHitReactType HitReactType);

	// 다운 리액션의 착지 이벤트 구독과 초기 상태를 설정한다.
	void StartDownHitReact(APRPlayerCharacter* PlayerCharacter);

	// 다운 리액션에서 사용하는 몽타주 섹션 연결과 전환 콜백을 설정한다.
	void ConfigureDownMontageFlow();

	// 다운 리액션 상태와 이벤트 구독을 정리한다.
	void ClearDownHitReact();

	// 현재 플레이어가 지면 위에 있는지 반환한다.
	bool IsPlayerMovingOnGround() const;

	// 다운 몽타주의 지정 섹션으로 이동한다.
	void JumpToDownSection(FName SectionName);

	// 다운 몽타주에 필요한 섹션들이 모두 있는지 확인한다.
	bool HasRequiredDownMontageSections(const UAnimMontage* Montage) const;

	// 강한 경직과 다운에서 몽타주 재생 동안 사용할 입력 제한 태그를 부여한다.
	void StartActionLock(EPRPlayerHitReactType HitReactType);

	// 입력 제한 태그를 제거한다.
	void ClearActionLock();

	// 피격 리액션 Ability를 한 번만 종료한다.
	void FinishHitReact(bool bWasCancelled);

protected:
	// 맨손 상태에서 사용할 약한 경직 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TObjectPtr<UAnimMontage> UnarmedWeakHitReactMontage;
	
	// 맨손 상태에서 사용할 강한 경직 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TObjectPtr<UAnimMontage> UnarmedStrongHitReactMontage;

	// 무기 타입별 약한 경직 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TMap<EPRWeaponType, TObjectPtr<UAnimMontage>> WeakHitReactMontages;
	
	// 무기 타입별 강한 경직 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TMap<EPRWeaponType, TObjectPtr<UAnimMontage>> StrongHitReactMontages;

	// 다운에서 사용할 섹션 기반 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage")
	TObjectPtr<UAnimMontage> DownHitReactMontage;

	// 다운 시작 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Down")
	FName DownStartSectionName = TEXT("Start");

	// 다운 낙하 반복 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Down")
	FName DownFallLoopSectionName = TEXT("FallLoop");

	// 다운 착지 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Down")
	FName DownLandSectionName = TEXT("Land");

	// 다운 기상 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Down")
	FName DownGetUpSectionName = TEXT("GetUp");

	// 피격 리액션 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.0f;

	// Ability 종료나 외부 취소 시 몽타주를 정지할 블렌드 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HitReact|Montage", meta = (ClampMin = "0.0"))
	float MontageStopBlendOutTime = 0.15f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveHitReactMontage;

	TWeakObjectPtr<APRPlayerCharacter> ActivePlayerCharacter;

	FDelegateHandle PlayerLandedDelegateHandle;

	EPRPlayerHitReactType ActiveHitReactType = EPRPlayerHitReactType::None;

	EPRPlayerDownHitReactPhase DownHitReactPhase = EPRPlayerDownHitReactPhase::None;

	bool bHitReactFinished = false;

	bool bDownLandRequested = false;

	bool bActionLockTagAdded = false;
};
