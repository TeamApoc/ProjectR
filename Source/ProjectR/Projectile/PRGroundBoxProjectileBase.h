// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "PRGroundBoxProjectileBase.generated.h"

class APRGroundBoxProjectileBase;
class APRPenitentCharacter;
class UAbilitySystemComponent;
class UBoxComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRGroundBoxSpawnedSignature, APRGroundBoxProjectileBase*, GroundBox);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRGroundBoxLaunchedSignature, APRGroundBoxProjectileBase*, GroundBox);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPRGroundBoxTargetHitSignature, APRGroundBoxProjectileBase*, GroundBox, AActor*, TargetActor, const FHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPRGroundBoxDamagedSignature, APRGroundBoxProjectileBase*, GroundBox, float, DamageAmount, float, HealthAfterDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRGroundBoxFadeRequestedSignature, APRGroundBoxProjectileBase*, GroundBox);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRGroundBoxDestroyedSignature, APRGroundBoxProjectileBase*, GroundBox);

USTRUCT(BlueprintType)
struct PROJECTR_API FPRGroundBoxLaunchParams
{
	GENERATED_BODY()

	// 공격 주체
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|GroundBox")
	TObjectPtr<AActor> SourceActor = nullptr;

	// 대상 피해 스펙
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|GroundBox")
	FGameplayEffectSpecHandle DamageEffectSpec;

	// 체력 오버라이드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|GroundBox", meta = (ClampMin = "0.0"))
	float OverrideMaxHealth = 0.0f;

	// 런치 방향
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|GroundBox")
	FVector LaunchDirection = FVector::ZeroVector;

	// 런치 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|GroundBox", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 0.0f;
};

// 지면 박스형 서버 권위 피해 액터
UCLASS()
class PROJECTR_API APRGroundBoxProjectileBase : public AActor, public IPRCombatInterface
{
	GENERATED_BODY()

public:
	// 컴포넌트와 기본값 초기화
	APRGroundBoxProjectileBase();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ IPRCombatInterface Interface ~*/
	virtual EPRTeam GetTeam() const override { return SourceTeam; }
	virtual bool ReceiveDamageContext(const FPRDamageAppliedContext& Context) override;
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) override;
	virtual bool IsDead() const override { return bDestroyRequested || CurrentHealth <= 0.0f; }

public:
	// 스폰 직후 서버 초기화
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|GroundBox")
	void InitGroundBoxProjectile(const FPRGroundBoxLaunchParams& Params);

	// 부착 소환 상태 초기화
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|GroundBox")
	void InitializeAttachedBarrier(APRPenitentCharacter* OwnerPenitent);

	// 이동 시작 처리
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|GroundBox")
	void LaunchGroundBoxProjectile(const FVector& LaunchDirection, float LaunchSpeed);

	// 대상 쿨다운 초기화
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|GroundBox")
	void ResetTargetCooldowns();

	// 명시 종료 요청
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|GroundBox")
	void RequestGroundBoxEnd();

	// 페이드 완료 제거
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|GroundBox")
	void FinishFadeAndDestroy();

	// 소유자 사망 처리
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|GroundBox")
	void HandleSourceOwnerDead();

	// 안전 수명 만료 처리
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|GroundBox")
	void HandleSafetyLifeTimeExpired();

protected:
	// 피해 박스 오버랩 처리
	UFUNCTION()
	void OnDamageDetectionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 환경 박스 오버랩 처리
	UFUNCTION()
	void OnBreakableDetectionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 피해 대상 필터
	bool CanApplyDamageToTarget(AActor* TargetActor, UAbilitySystemComponent* TargetAbilitySystemComponent) const;

	// 대상 피해 적용
	void ApplyDamageToTarget(AActor* TargetActor, UAbilitySystemComponent* TargetASC, const FHitResult& HitResult);

	// 기본 피해 스펙 생성
	FGameplayEffectSpecHandle BuildDefaultDamageEffectSpec(AActor* InSourceActor) const;

	// 쿨다운 검사
	bool IsTargetOnCooldown(AActor* TargetActor, UAbilitySystemComponent* TargetAbilitySystemComponent) const;

	// 쿨다운 기록
	void SetTargetCooldown(AActor* TargetActor, UAbilitySystemComponent* TargetAbilitySystemComponent);

	// 지면 스냅
	void SnapToGround(float DeltaSeconds, bool bInstant);

	// 벽 충돌 판정
	bool IsBlockingWallHit(const FHitResult& HitResult) const;

	// 소멸 처리
	void DestroyGroundBox();

	// 생성 연출 동기화
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleSpawned();

	// 런치 연출 동기화
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleLaunched();

	// 대상 피격 연출 동기화
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleTargetHit(AActor* TargetActor, const FHitResult& HitResult);

	// 본체 피격 연출 동기화
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleGroundBoxDamaged(float DamageAmount, float HealthAfterDamage);

	// 페이드 요청 동기화
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleFadeRequested();

	// 소멸 연출 동기화
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleDestroyed();

public:
	// 생성 연출 구독
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|GroundBox")
	FPRGroundBoxSpawnedSignature OnGroundBoxSpawned;

	// 런치 연출 구독
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|GroundBox")
	FPRGroundBoxLaunchedSignature OnGroundBoxLaunched;

	// 대상 피격 연출 구독
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|GroundBox")
	FPRGroundBoxTargetHitSignature OnGroundBoxTargetHit;

	// 본체 피격 연출 구독
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|GroundBox")
	FPRGroundBoxDamagedSignature OnGroundBoxDamaged;

	// 페이드 요청 연출 구독
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|GroundBox")
	FPRGroundBoxFadeRequestedSignature OnGroundBoxFadeRequested;

	// 소멸 연출 구독
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|GroundBox")
	FPRGroundBoxDestroyedSignature OnGroundBoxDestroyed;

protected:
	// 위치 기준 루트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|GroundBox")
	TObjectPtr<USceneComponent> Root;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|GroundBox")
	TObjectPtr<USceneComponent> TraceStartPoint;

	// 전투 대상 감지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|GroundBox")
	TObjectPtr<UBoxComponent> DamageDetectionBox;

	// 벽과 파괴 가능 환경물 감지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|GroundBox")
	TObjectPtr<UBoxComponent> BreakableDetectionBox;

	// 벽 시각 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|GroundBox")
	TObjectPtr<UStaticMeshComponent> WallMeshComponent;

	// 상시 시각 효과
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|GroundBox")
	TObjectPtr<UNiagaraComponent> AmbientVFXComponent;

	// 런치 이동
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|GroundBox")
	TObjectPtr<UProjectileMovementComponent> MovementComponent;

	// 최대 체력 기본값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|Health", meta = (ClampMin = "0.0"))
	float MaxHealth = 150.0f;

	// 현재 체력
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|Health")
	float CurrentHealth = 0.0f;

	// 기본 피해 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 기본 체력 피해
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|Damage", meta = (ClampMin = "0.0"))
	float DefaultDamageAmount = 35.0f;

	// 기본 그로기 피해
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|Damage", meta = (ClampMin = "0.0"))
	float DefaultGroggyDamageAmount = 0.0f;

	// 대상 쿨다운 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|Damage", meta = (ClampMin = "0.0"))
	float TargetCooldownTimeSeconds = 3.0f;

	// 안전 수명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|Lifetime", meta = (ClampMin = "0.0"))
	float MaxSafetyLifeTime = 30.0f;
	
	// Launch 후 수명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|Lifetime", meta = (ClampMin = "0.0"))
	float LaunchLifeTime = 2.5f;

	// 지면 스냅 추적 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|GroundSnap", meta = (ClampMin = "0.0"))
	float GroundSnapTraceDistance = 1000.0f;

	// 지면 스냅 허용 표면 법선
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|GroundSnap", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GroundNormalThreshold = 0.7f;

	// 지면 스냅 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|GroundSnap", meta = (ClampMin = "0.0"))
	float GroundSnapInterpSpeed = 20.0f;

	// 지면 스냅 완료 허용 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|GroundBox|GroundSnap", meta = (ClampMin = "0.0"))
	float GroundSnapTolerance = 1.0f;

private:
	// 소스 액터 약참조
	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	// 대상 피해 스펙
	FGameplayEffectSpecHandle DamageEffectSpecHandle;

	// ASC 기준 다음 피해 허용 시각
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, float> TargetASCNextAllowedTimes;

	// Actor 기준 다음 피해 허용 시각 폴백
	TMap<TWeakObjectPtr<AActor>, float> TargetActorNextAllowedTimes;

	// 안전 수명 타이머
	FTimerHandle SafetyLifeTimeTimerHandle;

	// 소스 팀 캐시
	EPRTeam SourceTeam = EPRTeam::Neutral;

	// 현재 피해 활성 상태
	bool bDamageEnabled = false;

	// 현재 런치 상태
	bool bLaunched = false;

	// 소멸 처리 중복 방지
	bool bDestroyRequested = false;
};
