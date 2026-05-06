// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "PRBossPortalActor.generated.h"

class APRProjectileBase;
class UGameplayEffect;

// 포털 하나의 내부 생명주기 상태다.
UENUM(BlueprintType)
enum class EPRBossPortalState : uint8
{
	None			UMETA(DisplayName = "None"),
	Telegraphing	UMETA(DisplayName = "Telegraphing"),
	Active			UMETA(DisplayName = "Active"),
	Firing			UMETA(DisplayName = "Firing"),
	Paused			UMETA(DisplayName = "Paused"),
	Expiring		UMETA(DisplayName = "Expiring"),
	Expired			UMETA(DisplayName = "Expired")
};

// 보스 포털 패턴의 공용 Helper Actor다.
// 기존 텔레그래프/활성/만료 생명주기를 유지하면서 타겟락, 반복 발사, 일시정지를 함께 관리한다.
UCLASS(Blueprintable)
class PROJECTR_API APRBossPortalActor : public APRBossPatternActor
{
	GENERATED_BODY()

public:
	APRBossPortalActor();

	virtual void InitializeBossPatternActor(APRBossBaseCharacter* InOwnerBoss, AActor* InPatternTarget) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void RequestPatternActorExpire() override;
	virtual void CancelPatternActor() override;

	// 현재 포털이 발사/위험 상태인지 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	bool IsPortalActive() const { return bPortalActive; }

	// 현재 포털 생명주기 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	EPRBossPortalState GetPortalState() const { return PortalState; }

	// 포털이 고정해 둔 발사 타겟을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	AActor* GetLockedPortalTarget() const { return LockedTarget; }

	// 서버에서 포털 발사 타겟을 고정한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void SetAndLockPortalTarget(AActor* InTarget);

	// 포털 투사체에 전달할 GE Spec을 설정한다.
	void SetPortalProjectileEffectSpec(const FGameplayEffectSpecHandle& InEffectSpec);

	// 서버에서 포털 텔레그래프를 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void StartPortalTelegraph();

	// 서버에서 포털을 활성화한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ActivatePortal();

	// 서버에서 포털을 만료 처리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ExpirePortal();

	// 서버에서 포털을 강제 만료 처리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ForceExpirePortal();

	// 서버에서 포털 발사를 일시정지한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void PausePortal();

	// 서버에서 일시정지된 포털 발사를 재개한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void UnpausePortal();

	// 서버에서 포털 투사체를 1회 발사한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void FirePortalProjectile();

	// 포털 발사 타이머를 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ClearPortalFireTimer();

	// 포털 투사체 스폰 Transform을 계산한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	bool BuildProjectileSpawnTransform(FTransform& OutTransform) const;

	// 현재 타겟 기준 조준 방향을 계산한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	FVector CalculateProjectileAimDirection() const;

	// 지금 포털 투사체를 발사할 수 있는지 확인한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	bool CanFirePortalProjectile() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 모든 클라이언트에서 텔레그래프 시작 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalTelegraphStarted();

	// 모든 클라이언트에서 포털 활성 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalActivated();

	// 모든 클라이언트에서 포털 만료 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalExpired();

	// 모든 클라이언트에서 포털 발사 시작 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalFireStarted();

	// 모든 클라이언트에서 포털 투사체 생성 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalProjectileSpawned(APRProjectileBase* SpawnedProjectile);

	// 모든 클라이언트에서 포털 발사 일시정지 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalPaused();

	// 모든 클라이언트에서 포털 발사 재개 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalUnpaused();

	// 모든 클라이언트에서 포털 발사 완료 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalFireSequenceCompleted();

	// 포털 텔레그래프 시작 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalTelegraphStarted();

	// 포털 활성 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalActivated();

	// 포털 만료 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalExpired();

	// 포털 발사 시작 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalFireStarted();

	// 포털 투사체 생성 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalProjectileSpawned(APRProjectileBase* SpawnedProjectile);

	// 포털 일시정지 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalPaused();

	// 포털 재개 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalUnpaused();

	// 포털 발사 완료 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalFireSequenceCompleted();

protected:
	// 다음 포털 발사를 예약한다.
	void ScheduleNextPortalFire();

	// 포털 발사 시퀀스를 완료 처리한다.
	void CompletePortalFireSequence();

	// 포털 생명주기 타이머를 모두 정리한다.
	void ClearPortalLifecycleTimers();

	// 포털 투사체에 적용할 GE Spec을 만든다.
	FGameplayEffectSpecHandle BuildProjectileEffectSpec() const;

protected:
	// BeginPlay에서 자동으로 텔레그래프를 시작할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bAutoStartPortal = false;

	// 텔레그래프 시작 후 실제 활성까지 걸리는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0"))
	float ActivationDelay = 0.75f;

	// 활성 후 만료까지 유지되는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0"))
	float ActiveDuration = 14.0f;

	// 만료 이벤트 후 Actor를 제거할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bDestroyWhenExpired = true;

	// 발사 완료 후 포털을 만료할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire")
	bool bDestroyAfterFireComplete = true;

	// Pause 중에도 발사를 허용할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire")
	bool bCanFireWhilePaused = false;

	// 포털이 발사할 투사체 클래스다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	TSubclassOf<APRProjectileBase> ProjectileClass;

	// 활성 후 첫 발사까지 걸리는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire", meta = (ClampMin = "0.0"))
	float InitialFireDelay = 1.0f;

	// 반복 발사 간격이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire", meta = (ClampMin = "0.0"))
	float FireInterval = 4.75f;

	// 포털 하나가 발사할 최대 투사체 수다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Fire", meta = (ClampMin = "0"))
	int32 MaxProjectilesToFire = 3;

	// 투사체 속도 보정값이다. 0이면 투사체 BP 기본값을 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileSpeedOverride = 3500.0f;

	// 타겟 이동 예측 보정 계수다. 원작 값 5는 0.05배 보정으로 해석한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileTargetLead = 5.0f;

	// 추적 투사체 사용 여부다. 1차에서는 스폰 정책 구분용 플래그로만 보관한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	bool bUseTrackingProjectile = true;

	// 추적 투사체 Homing 가속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile", meta = (ClampMin = "0.0", EditCondition = "bUseTrackingProjectile"))
	float ProjectileHomingAcceleration = 12000.0f;

	// 투사체 스폰 위치의 로컬 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	FVector ProjectileSpawnLocalOffset = FVector::ZeroVector;

	// 투사체 스폰 회전 추가 보정값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Projectile")
	FRotator ProjectileRotationOffset = FRotator::ZeroRotator;

	// 포털 투사체에 적용할 피해 GE다. 비어 있으면 AbilitySystemRegistry의 적 피해 GE를 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Damage")
	TSubclassOf<UGameplayEffect> ProjectileDamageEffectClass;

	// 포털 투사체 피해 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal|Damage", meta = (ClampMin = "0.0"))
	float ProjectileAttackMultiplier = 1.0f;

	// 현재 포털 내부 생명주기 상태다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	EPRBossPortalState PortalState = EPRBossPortalState::None;

	// 포털이 고정한 발사 타겟이다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	TObjectPtr<AActor> LockedTarget;

	// 현재 포털 활성 상태다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bPortalActive = false;

private:
	FTimerHandle PortalActivationTimerHandle;
	FTimerHandle PortalExpireTimerHandle;
	FTimerHandle PortalFireTimerHandle;

	FGameplayEffectSpecHandle ProjectileEffectSpecHandle;
	int32 CurrentProjectileFireCount = 0;
	uint32 NextPortalProjectileId = 1;
};
