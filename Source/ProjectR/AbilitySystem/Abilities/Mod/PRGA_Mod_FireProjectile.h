// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRGA_Mod.h"
#include "PRGA_Mod_FireProjectile.generated.h"

class UNiagaraSystem;
class APRProjectileBase;

// 투사체를 발사하는 모드 스킬 어빌리티 베이스.
// Activate 시점에 카메라 조준 방향으로 총구에서 투사체를 1발 스폰.
// 서버 권위 투사체에 한해 모드 데미지 GE Spec을 부여하여 충돌 시 모드 GE 적용 흐름과 연결한다
UCLASS(Abstract)
class PROJECTR_API UPRGA_Mod_FireProjectile : public UPRGA_Mod
{
	GENERATED_BODY()

public:
	UPRGA_Mod_FireProjectile();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 투사체 1발 발사. AbilityTask 통해 예측 클라이언트/권위 서버 스폰 흐름 진행
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Mod|Projectile")
	void FireProjectile(FVector SpawnLocation, FRotator SpawnRotation);

	// AbilityTask가 투사체 스폰 성공 시 호출. 서버 권위에 한해 모드 데미지 GE Spec 부여
	UFUNCTION()
	virtual void OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile);

	// AbilityTask가 투사체 스폰 실패/예측 거부 시 호출. 후속 정리 처리
	UFUNCTION()
	virtual void OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile);

	// 투사체 스폰 성공 시 BP 후처리(VFX/SFX 등)
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|Mod|Projectile")
	void K2_OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile);

	// 투사체 스폰 실패 시 BP 후처리
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|Mod|Projectile")
	void K2_OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile);

protected:
	// 발사할 투사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Projectile")
	TSubclassOf<APRProjectileBase> ProjectileClass;
	
	// 총구 이펙트 (Optional)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Projectile")
	TObjectPtr<UNiagaraSystem> MuzzleVFX;
	
	// 모드 스킬 기본 데미지. SetByCaller로 GE Spec에 전달
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Projectile", meta = (ClampMin = "0.0"))
	float Damage = 0.f;

	// 모드 스킬 그로기 데미지. SetByCaller로 GE Spec에 전달
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Projectile", meta = (ClampMin = "0.0"))
	float GroggyDamage = 0.f;
};
