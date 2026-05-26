// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "PRPenitentBarrierActor.generated.h"

class APRPenitentCharacter;
class UAbilitySystemComponent;
class UPRAbilitySystemComponent;
class UPRAttributeSet_Common;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;

// Penitent 소환 배리어 액터
UCLASS()
class PROJECTR_API APRPenitentBarrierActor : public AActor, public IAbilitySystemInterface, public IPRCombatInterface
{
	GENERATED_BODY()

public:
	// 배리어 컴포넌트와 기본 체력 초기화
	APRPenitentBarrierActor();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ IAbilitySystemInterface ~*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*~ IPRCombatInterface ~*/
	virtual EPRTeam GetTeam() const override { return EPRTeam::Enemy; }
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) override;

public:
	// 소환 직후 부착 상태 초기화
	void InitializeAttachedBarrier(APRPenitentCharacter* OwnerPenitent);

	// 발사 피해 GE Spec 설정
	void SetBarrierDamageEffectSpec(const FGameplayEffectSpecHandle& InEffectSpecHandle);

	// 발사 상태 전환
	void FireBarrier(const FVector& LaunchDirection);

	// 배리어 제거 요청
	void DestroyBarrier();

	// 배리어 피해 활성 여부 반환
	bool IsDamageEnabled() const { return bDamageEnabled; }

protected:
	// 발사 후 충돌 처리
	UFUNCTION()
	void HandleBarrierHit(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	// 피해 대상 적용
	void ApplyBarrierDamage(AActor* TargetActor, const FHitResult& Hit);

	// 체력 기본값 적용
	void InitializeBarrierHealth();

protected:
	// 충돌 루트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Barrier")
	TObjectPtr<USphereComponent> SphereComponent;

	// 시각 메시
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Barrier")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 발사 이동
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Barrier")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	// 배리어 ASC
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Ability")
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	// 공통 체력 AttributeSet
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Common> CommonSet;

	// 기본 체력
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Barrier", meta = (ClampMin = "0.0"))
	float BarrierHealth = 150.0f;

	// 발사 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Barrier", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 1800.0f;

	// 발사 후 수명
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Barrier", meta = (ClampMin = "0.0"))
	float FiredLifeSpan = 5.0f;

	// 발사 전 충돌 프로필
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Barrier")
	FName AttachedCollisionProfileName = TEXT("NoCollision");

	// 발사 후 충돌 프로필
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Barrier")
	FName FiredCollisionProfileName = TEXT("Projectile");

private:
	// 소유 Penitent
	UPROPERTY()
	TObjectPtr<APRPenitentCharacter> OwningPenitent;

	// 발사 피해 GE Spec
	FGameplayEffectSpecHandle DamageEffectSpecHandle;

	bool bDamageEnabled = false;
};
