// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRFireAbilityTypes.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_Fire.generated.h"

class APRWeaponActor;
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
	// 총구 위치 반환
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
	UFUNCTION()
	virtual void FireOneShot();

protected:
	// 서버가 샷을 확정 처리. 데미지 적용 (현재는 로그만)
	virtual void ServerConfirmShot(const FPRFireShotPayload& Payload);

	// 데미지 적용. 현재 단계에서는 로그만 남긴다
	virtual void ApplyDamageFromShot(const FPRFireShotPayload& Payload);

protected:
	// 트레이스 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire")
	float MaxTraceDistance = 20000.f;

	// 사격 트레이스에 사용할 충돌 채널
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire")
	TEnumAsByte<ECollisionChannel> FireTraceChannel = PRCollisionChannels::ECC_Combat;

	// 1발당 발사 시 발생하는 반동 강도 (Event.Player.Recoil 페이로드) // TODO: WeaponData로 이전
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Recoil")
	float RecoilStrength = 1.0f;

	// 1발당 발사 시 반동 회복/적용 속도 (Event.Player.Recoil 페이로드) // TODO: WeaponData로 이전
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Recoil")
	float RecoilSpeed = 10.0f;

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
};
