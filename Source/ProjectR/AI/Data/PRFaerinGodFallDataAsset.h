// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRFaerinGodFallDataAsset.generated.h"

class APRFaerinGodFallStaticSwordActor;
class UAnimMontage;
class UAnimSequenceBase;
class UGameplayEffect;
class UNiagaraSystem;

// God Fall 본체 시전 중 지정 시각에 몸에 붙여 재생할 Niagara cue다.
USTRUCT(BlueprintType)
struct FPRFaerinGodFallBodyNiagaraCue
{
	GENERATED_BODY()

	// 재생할 Niagara 시스템이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	// God Fall 시전 시작 후 이 Niagara를 재생할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX", meta = (ClampMin = "0.0"))
	float StartDelaySeconds = 0.0f;

	// 보스 Mesh에 붙일 소켓 이름이다. None이면 Mesh 기준으로 붙는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	FName AttachSocketName = NAME_None;

	// AttachSocket 기준 상대 위치 보정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	FVector LocationOffset = FVector::ZeroVector;

	// AttachSocket 기준 상대 회전 보정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// Niagara 상대 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	FVector Scale = FVector::OneVector;

	// 0보다 크면 해당 시간이 지난 뒤 강제로 정리한다. 0이면 God Fall 본체 연출 종료/취소 시 정리한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX", meta = (ClampMin = "0.0"))
	float LifeSeconds = 0.0f;
};

// Faerin God Fall의 맵 배치 검 Rig 전환, 본체 연출, StaticSword 충전/배정 루프를 관리하는 데이터다.
UCLASS(BlueprintType)
class PROJECTR_API UPRFaerinGodFallDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/*~ UPrimaryDataAsset Interface ~*/
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// 맵 배치 Faerin_Swords_Rig에서 StaticSword 원위치로 사용할 bone 이름 10개다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig")
	TArray<FName> SwordBoneNames;

	// Bone 위치마다 생성할 StaticSword Actor 클래스다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|StaticSword")
	TSubclassOf<APRFaerinGodFallStaticSwordActor> StaticSwordActorClass;

	// 맵 배치 rig가 검을 기울이는 동안 재생할 애니메이션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig|Animation")
	TObjectPtr<UAnimSequenceBase> RigChargeAnimation;

	// 맵 배치 rig가 검을 위로 끌어올릴 때 재생할 애니메이션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig|Animation")
	TObjectPtr<UAnimSequenceBase> RigTiltPullAnimation;

	// Rig Charge 애니메이션 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig|Animation", meta = (ClampMin = "0.01"))
	float RigChargePlayRate = 1.0f;

	// Rig TiltPull 애니메이션 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig|Animation", meta = (ClampMin = "0.01"))
	float RigTiltPullPlayRate = 1.0f;

	// Faerin 본체 Charge 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Animation")
	TObjectPtr<UAnimMontage> BossBodyChargeMontage;

	// Faerin 본체 TiltPull 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Animation")
	TObjectPtr<UAnimMontage> BossBodyTiltPullMontage;

	// Faerin 본체 Drop 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Animation")
	TObjectPtr<UAnimMontage> BossBodyDropMontage;

	// Faerin 본체 몽타주 공통 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Animation", meta = (ClampMin = "0.01"))
	float BossBodyMontagePlayRate = 1.0f;

	// Phase2 진입 시 God Fall 시전 위치까지 이동하는 속도다. Sprint가 아닌 일반 이동 연출이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float BossCastMoveSpeed = 560.0f;

	// God Fall 시전 위치 도착 판정 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float BossCastAcceptanceRadius = 80.0f;

	// Charge 동안 Faerin 본체가 상승할 높이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float BossChargeRiseHeight = 600.0f;

	// Charge 동안 상승과 360도 회전을 끝낼 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.01"))
	float BossChargeRiseSeconds = 10.58f;

	// Charge 동안 누적 회전할 yaw 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation")
	float BossChargeYawDegrees = 360.0f;

	// Drop 시작 후 Faerin 본체가 바닥 위치로 하강하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.01"))
	float BossDropSeconds = 1.3f;

	// 대기 후 Faerin이 실제로 지면까지 떨어지는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float BossFastDropSeconds = 0.12f;

	// Faerin 하강과 함께 모든 StaticSword가 아래로 한 번 돌진하는 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float EntrySwordDiveDistance = 700.0f;

	// Entry dive 후 StaticSword가 자기 위치로 복귀하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float EntrySwordDiveReturnSeconds = 0.35f;

	// Phase2 기준 검 충전 최소 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float Phase2SwordChargeMinSeconds = 8.0f;

	// Phase2 기준 검 충전 최대 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float Phase2SwordChargeMaxSeconds = 10.0f;

	// 검이 돌진하기 전 원위치에서 경고를 보여주는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float WarningSeconds = 1.0f;

	// 배정된 검이 자기 위치에서 플레이어 머리 위까지 이동하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float TargetOverheadMoveSeconds = 8.0f;

	// 배정된 검이 플레이어 머리 위를 추적할 때 매초 증가하는 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float TargetOverheadMoveAcceleration = 900.0f;

	// 검이 원위치에서 목표 지점까지 돌진하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.01"))
	float DropSeconds = 0.28f;

	// impact 이후 원위치로 복귀하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.01"))
	float RiseSeconds = 0.45f;

	// impact 상태를 유지해 피격/시각 연출을 보여줄 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float ImpactHoldSeconds = 0.12f;

	// StaticSword가 바닥에 충돌할 때 충돌 위치에 재생할 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordImpact")
	TObjectPtr<UNiagaraSystem> SwordImpactNiagaraSystem;

	// StaticSword 충돌 Niagara 회전 보정값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordImpact")
	FRotator SwordImpactNiagaraRotationOffset = FRotator::ZeroRotator;

	// StaticSword 충돌 Niagara 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordImpact")
	FVector SwordImpactNiagaraScale = FVector::OneVector;

	// 0보다 크면 충돌 Niagara를 해당 시간 뒤 강제로 정리한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordImpact", meta = (ClampMin = "0.0"))
	float SwordImpactNiagaraLifeSeconds = 2.0f;

	// God Fall 본체 시전 중 보스 몸에 붙여 재생할 Niagara cue 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|Body")
	TArray<FPRFaerinGodFallBodyNiagaraCue> BodyNiagaraCues;

	// Phase3에서 검 충전 시간과 경고 시간을 곱할 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.01"))
	float Phase3GodFallTimingScale = 0.65f;

	// Phase4에서 검 충전 시간과 경고 시간을 곱할 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.01"))
	float Phase4GodFallTimingScale = 0.45f;

	// 검 높이에서 아래로 내리는 지면 trace 길이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Target", meta = (ClampMin = "0.0"))
	float GroundTraceDownDistance = 3000.0f;

	// 지면 투영에 사용할 trace channel이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Target")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// impact overlap 판정에 사용할 channel이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage")
	TEnumAsByte<ECollisionChannel> ImpactOverlapChannel = ECC_Pawn;

	// impact 피해에 사용할 GameplayEffect다. 비워 두면 Registry의 Enemy damage GE를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage")
	TSubclassOf<UGameplayEffect> ImpactDamageEffectClass;

	// Enemy AttackPower에 곱할 impact 피해 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage", meta = (ClampMin = "0.0"))
	float ImpactDamageMultiplier = 1.15f;

	// impact가 플레이어에게 적용하는 강인도 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage", meta = (ClampMin = "0.0"))
	float ImpactPoiseDamage = 18.0f;

	// impact 피해 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage", meta = (ClampMin = "0.0"))
	float ImpactRadius = 180.0f;

	// PIE 검증용 debug draw 표시 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Debug")
	bool bDrawDebug = false;
};
