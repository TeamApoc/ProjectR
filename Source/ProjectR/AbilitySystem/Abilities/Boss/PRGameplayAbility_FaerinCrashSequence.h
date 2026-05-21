// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinStagedMontagePattern.h"
#include "PRGameplayAbility_FaerinCrashSequence.generated.h"

// Faerin 원작 Crash / CrashDrop 계열의 DoCrashAoE 이벤트 피해를 처리하는 패턴 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinCrashSequence : public UPRGameplayAbility_FaerinStagedMontagePattern
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinCrashSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// Crash 실행마다 중복 피해 상태를 초기화한 뒤 staged montage 패턴을 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Crash 종료 시 이벤트 피해 캐시를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 원작 DoCrashAoE CharacterEvent를 수신하면 주변 피해를 적용한다.
	virtual void NativeOnStageCharacterEvent(const FPRFaerinStagedMontageStage& Stage, FName EventName) override;

	// Crash AoE 중심점을 계산한다.
	FVector ResolveCrashAreaCenter() const;

	// Crash AoE 반경 안의 유효 대상에게 피해를 적용한다.
	void ApplyCrashAreaDamage();

protected:
	// 원작 Crash AoE를 발생시키는 CharacterEvent 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	FName CrashAreaEventName = TEXT("DoCrashAoE");

	// Crash AoE 중심에 더할 보스 로컬 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	FVector CrashAreaLocalOffset = FVector::ZeroVector;

	// Crash AoE 피해 반경이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash", meta = (ClampMin = "0.0"))
	float CrashDamageRadius = 450.0f;

	// Enemy AttackPower에 곱할 Crash 피해 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash", meta = (ClampMin = "0.0"))
	float CrashDamageMultiplier = 1.5f;

	// Crash가 부여할 강인도 피해다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash", meta = (ClampMin = "0.0"))
	float CrashPoiseDamage = 25.0f;

	// Crash AoE 대상 탐색에 사용할 채널이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	TEnumAsByte<ECollisionChannel> CrashOverlapChannel = ECC_Pawn;

	// 하나의 Ability 실행에서 DoCrashAoE가 여러 번 들어와도 한 번만 피해를 적용할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash")
	bool bApplyCrashDamageOncePerActivation = true;

	// Crash AoE 반경을 에디터 디버그로 표시할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|Debug")
	bool bDrawDebugCrashArea = false;

	// Crash AoE 디버그 표시 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Crash|Debug",
		meta = (EditCondition = "bDrawDebugCrashArea", ClampMin = "0.0"))
	float CrashDebugDrawDuration = 1.5f;

private:
	TSet<TWeakObjectPtr<AActor>> DamagedActorsThisCrash;
	bool bCrashDamageApplied = false;
};
