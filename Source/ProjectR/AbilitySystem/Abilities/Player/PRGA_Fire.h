// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRFireAbilityTypes.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_Fire.generated.h"

struct FPRProjectileSpawnInfo;
class APRProjectileBase;
class APRWeaponActor;
class UAnimMontage;      
class UPRWeaponDataAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogFire, Log, All);

/**
 * 플레이어 사격 어빌리티 베이스
 */
UCLASS(Abstract)
class PROJECTR_API UPRGA_Fire : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_Fire();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

public:
	// 총구 위치 
	UFUNCTION(BlueprintPure)
	virtual FVector GetMuzzleLocation() const;

	// 로컬 화면 Viewpoint 추출
	virtual FPRFireViewpoint GetFireViewpoint() const;

	// 1차 트레이스: 카메라에서 조준 방향으로 트레이스해 조준 끝점(월드 좌표)을 산출한다
	virtual FVector ResolveAimPoint(const FPRFireViewpoint& View, float InMaxTraceDistance) const;

	// 2차 트레이스: 총구에서 조준 끝점으로 트레이스해 실제 발사 히트 결과를 산출한다
	virtual FHitResult PerformMuzzleTrace(const FVector& MuzzleLoc, const FVector& AimPoint) const;

	// per-shot 서버 보고 (unreliable). 유실 허용
	UFUNCTION(Server, Unreliable)
	void Server_ReportShot(FPRFireShotPayload Payload);

	// 발사 1회 처리. 로컬에서 트레이스 + 디버그라인 + 서버 보고 흐름을 수행한다
	UFUNCTION(BlueprintCallable)
	virtual void FireHitScan();

	// 투사체 1개 발사. 결과는 OnProjectileSpawnSuccess/Failed 가상 핸들러로 통지
	UFUNCTION(BlueprintCallable)
	virtual void FireProjectile(TSubclassOf<APRProjectileBase> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation);
	
	
	// 조준 기준 투사체 발사 트랜스폼 계산. 1차 카메라 트레이스로 조준점을 구하고, 총구->조준점 방향을 회전으로 환산
	// 위치는 총구. 조준점이 총구 뒤쪽이거나 거리가 너무 짧으면 카메라 정면 방향으로 폴백
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FTransform GetProjectileLaunchTransform() const;
	
protected:
	// 04.28 김동석 추가, // strength, speed는 삭제
	// 연속 발사 횟수를 초기화한다
	void ResetConsecutiveShots();

	// 서버가 샷을 확정 처리. 데미지 적용 (현재는 로그만)
	virtual void ServerConfirmShot(const FPRFireShotPayload& Payload);

	// 데미지 적용. Payload의 HitResult를 바탕으로 GE를 생성해 타겟 ASC에 적용한다
	virtual void ApplyDamageFromShot(const FPRFireShotPayload& Payload);
	
	// 현재 활성 무기 데이터를 반환한다                           
	UPRWeaponDataAsset* GetActiveWeaponData() const;              
                                                              
	// 지정한 무기 몽타주를 현재 어빌리티 컨텍스트에서 재생한다   
	void PlayWeaponMontage(UAnimMontage* Montage, float PlayRate);

	// 플레이어 무기 데미지 GE를 타겟에 적용한다. HitResult를 EffectContext에 포함시킨다
	void ApplyDamage(AActor* TargetActor, const FHitResult* HitResult = nullptr);

	// AbilityTask가 투사체 스폰 성공 시 호출. 파생 클래스에서 추가 처리(VFX/SFX 등) 오버라이드 용도
	UFUNCTION()
	virtual void OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile);

	// AbilityTask가 투사체 스폰 실패/예측 거부 시 호출. 파생 클래스에서 후속 처리 오버라이드 용도
	UFUNCTION()
	virtual void OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile);

	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile);
	
	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile);
protected:
	// 트레이스 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire")
	float MaxTraceDistance = 20000.f;

	// 사격 트레이스에 사용할 충돌 채널
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire")
	TEnumAsByte<ECollisionChannel> FireTraceChannel = PRCollisionChannels::ECC_Combat;

	// 카메라 트레이스(1차, 시안색) 디버그 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Debug")
	bool bDrawCameraTrace = true;

	// 총구 트레이스(2차, 빨강) 디버그 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Debug")
	bool bDrawMuzzleTrace = true;

	// 디버그 라인 지속 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Debug")
	float DebugDrawDuration = 1.0f;
	
	// 활성 무기 캐시 (활성화 시 1회 획득)
	TWeakObjectPtr<APRWeaponActor> CurrentWeapon;

	// 로컬 단조 증가 ShotID (1부터)
	uint32 NextShotId = 0;

	// 현재 입력 유지 중 누적된 연속 발사 횟수
	int32 ConsecutiveShots = 0;
};
