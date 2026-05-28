// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "ProjectR/AI/Components/PRFaerinCombatDirectorComponent.h"
#include "PRGameplayAbility_FaerinNearTeleportApproach.generated.h"

class AAIController;
class ACharacter;
class APREnemyBaseCharacter;
class UNiagaraSystem;

// Faerin 3페이즈 이전 접근 루프에서 사용하는 근거리 텔레포트 접근 Ability다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinNearTeleportApproach : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinNearTeleportApproach();

	/*~ UGameplayAbility Interface ~*/
public:
	// Director가 넘긴 요청을 읽고 사라짐 후 자기 주변 재등장 접근을 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 진행 중인 타이머와 임시 숨김/충돌 상태를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 소유 Faerin Director에서 이번 근거리 텔레포트 요청 데이터를 가져온다.
	bool ResolveTeleportRequest(APREnemyBaseCharacter* EnemyCharacter);

	// 사라짐 상태로 전환하고 재등장 타이머를 예약한다.
	void BeginDisappear(ACharacter* BossCharacter);

	// 자기 주변 EQS 목적지를 계산한 뒤 보스를 재등장시킨다.
	void ReappearNearSelf();

	// 현재 보스 위치 기준 EQS 결과로 재등장 위치와 회전을 계산한다.
	bool ResolveReappearTransform(FVector& OutLocation, FRotator& OutRotation) const;

	// 근거리 텔레포트 위치 선정 EQS를 실행한다.
	bool ResolveEQSReappearLocation(FVector& OutLocation) const;

	// 보스 몸에 연결되는 근거리 텔레포트 순간 Niagara를 모든 클라이언트에 요청한다.
	void SpawnBodyNiagara(UNiagaraSystem* NiagaraSystem) const;

	// 보스의 숨김/충돌 상태를 원래 전투 가능 상태로 되돌린다.
	void RestoreBossPresentation(ACharacter* BossCharacter) const;

	// 현재 Ability를 종료한다.
	void FinishNearTeleport(bool bWasCancelled);

protected:
	// 사라지는 순간 보스 몸에 붙일 디졸브 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	TObjectPtr<UNiagaraSystem> DisappearDissolveNiagaraSystem;

	// 사라지는 순간 보스 몸에 붙일 텔레포트 인 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	TObjectPtr<UNiagaraSystem> TeleportInNiagaraSystem;

	// 재등장 순간 보스 몸에 붙일 텔레포트 아웃 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	TObjectPtr<UNiagaraSystem> TeleportOutNiagaraSystem;

	// 재등장 순간 보스 몸에 붙일 역방향 디졸브 Niagara다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	TObjectPtr<UNiagaraSystem> ReappearDissolveNiagaraSystem;

	// 몸 Niagara를 붙일 소켓 이름이다. 비워 두면 Mesh 루트에 붙는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|VFX")
	FName BodyNiagaraAttachSocketName = NAME_None;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCombatDirectorComponent> ActiveDirectorComponent;

	FPRFaerinNearTeleportRequest ActiveRequest;
	FTimerHandle ReappearTimerHandle;
	FVector DisappearLocation = FVector::ZeroVector;
	bool bOriginalActorCollisionEnabled = true;
	bool bNearTeleportFinished = false;
};
