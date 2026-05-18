// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/AbilitySystem/Abilities/Player/PRFireAbilityTypes.h"
#include "PRGA_Mod.generated.h"

class UPRWeaponManagerComponent;
class APRWeaponActor;

// ========= Mod Base  =============
class UPRItemInstance_Weapon;
class UAbilitySystemComponent;
enum class EPRWeaponSlotType : uint8;

// 모드 어빌리티의 공통 비용 처리 방식
UENUM(BlueprintType)
enum class EPRModCostPolicyType : uint8
{
	None,
	Stack,
	GaugeDuration
};

// 플레이어 모드 스킬 어빌리티 베이스다.
// 데미지와 그로기 데미지는 SetByCaller로 전달하며, DamageGE_FromMod ExecCalc를 사용한다.
UCLASS(Abstract)
class PROJECTR_API UPRGA_Mod : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_Mod();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// ========= Mod Base  =============
	// 모드 공통 비용 조건을 검사한다
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 모드 공통 비용을 적용한다
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

public:
	// 총구 위치. CurrentWeapon이 유효하지 않으면 아바타 정면 폴백 위치 반환
	UFUNCTION(BlueprintPure)
	virtual FVector GetMuzzleLocation() const;
	
	// 총구 위치, 회전
	UFUNCTION(BlueprintPure)
	virtual FTransform GetMuzzleTransform() const;

	// 로컬 화면 Viewpoint(카메라 위치/회전) 추출. 로컬 컨트롤러에서만 호출
	virtual FPRFireViewpoint GetFireViewpoint() const;

	// 카메라에서 조준 방향으로 트레이스해 조준 끝점(월드 좌표) 산출. 적중 없으면 최대 거리 끝점
	virtual FVector ResolveAimPoint(const FPRFireViewpoint& View, float InMaxTraceDistance) const;

protected:
	// 모드 스킬 데미지 GE를 타겟에 적용, Damage와 GroggyDamage를 SetByCaller로 전달
	void ApplyDamage(AActor* TargetActor, float Damage, float GroggyDamage = 0.0f, const FHitResult* HitResult = nullptr);

	// ========= Mod Base  =============
	// 모드 비용 처리 방식을 설정한다
	void SetModCostPolicy(EPRModCostPolicyType InModCostType);

protected:
	// 카메라 조준 트레이스 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod")
	float MaxTraceDistance = 20000.f;

	// 조준 트레이스 충돌 채널
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod")
	TEnumAsByte<ECollisionChannel> AimTraceChannel = PRCollisionChannels::ECC_Combat;

	// 카메라 조준 트레이스 디버그 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Debug")
	bool bDrawCameraTrace = false;

	// 디버그 라인 지속 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Debug")
	float DebugDrawDuration = 1.0f;

	// 활성 무기 캐시
	mutable TWeakObjectPtr<APRWeaponActor> CurrentWeapon;
	
	// 무기 매니저 캐시
	TWeakObjectPtr<UPRWeaponManagerComponent> CachedWeaponManager;

	// ========= Mod Base  =============
	// 지속시간형 모드 효과의 유지 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Cost", meta = (ClampMin = "0.0"))
	float ModDuration = 0.0f;

private:
	// ========= Mod Base  =============
	bool TryGetCurrentModCostContext(const FGameplayAbilityActorInfo* ActorInfo, EPRWeaponSlotType& OutSlotType, UAbilitySystemComponent*& OutASC, UPRItemInstance_Weapon*& OutWeaponInstance) const;
	// 스택 코스트 확인
	bool HasModStackCost(const FGameplayAbilityActorInfo* ActorInfo) const;
	// 게이지가 가득 찬 상태인지 확인
	bool HasFullModGaugeCost(const FGameplayAbilityActorInfo* ActorInfo) const;
	// 현재 슬롯의 지속시간형 Mod 게이지 비용이 적용 중인지 확인
	bool HasActiveModGaugeLock(const FGameplayAbilityActorInfo* ActorInfo) const;
	// 스택 소모 코스트 적용
	void ApplyModStackCost(const FGameplayAbilityActorInfo* ActorInfo) const;
	void ApplyModGaugeDurationCost(const FGameplayAbilityActorInfo* ActorInfo) const;
	FGameplayAttribute GetModGaugeAttribute(EPRWeaponSlotType SlotType) const;
	FGameplayAttribute GetMaxModGaugeAttribute(EPRWeaponSlotType SlotType) const;
	FGameplayAttribute GetModStackAttribute(EPRWeaponSlotType SlotType) const;
	FGameplayAttribute GetMaxModStackAttribute(EPRWeaponSlotType SlotType) const;

	// 모드 비용 처리 방식
	EPRModCostPolicyType ModCostPolicy = EPRModCostPolicyType::None;

};
