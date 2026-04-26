// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PRSoldierArmoredCombatDataAsset.generated.h"

// Soldier_Armored 해머 콤보 현재 재생 구간
UENUM(BlueprintType)
enum class EPRSoldierArmoredHammerSection : uint8
{
	Combo1		UMETA(DisplayName = "Combo1"),
	Combo2		UMETA(DisplayName = "Combo2"),
	Finisher1	UMETA(DisplayName = "Finisher1"),
	Finisher2	UMETA(DisplayName = "Finisher2")
};

// Soldier_Armored 공격별 타격값
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSoldierArmoredAttackHitConfig
{
	GENERATED_BODY()

	// Health 적용 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 0.0f;

	// GroggyGauge 적용 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 0.0f;

	// 해당 섹션 공격 Sweep 전방 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 0.0f;

	// 해당 섹션 공격 Sweep 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRadius = 0.0f;

	// 공격 판정 기준으로 사용할 소켓 또는 본 이름이다.
	// None이면 캐릭터 정면 Sweep를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	FName AttackTraceSourceName = NAME_None;

	// 소켓 또는 본 기준 공격 판정 위치 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	FVector AttackTraceSourceOffset = FVector::ZeroVector;
};

// Soldier_Armored 해머 패밀리 몽타주 섹션 이름 묶음
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSoldierArmoredHammerSectionNames
{
	GENERATED_BODY()

	// HammerCombo_01 시작 섹션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Combo1StartSection = NAME_None;

	// HammerCombo_02 진입 섹션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Combo2EnterSection = NAME_None;

	// Finisher_01 진입 섹션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Finisher1EnterSection = NAME_None;

	// Finisher_02 진입 섹션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Finisher2EnterSection = NAME_None;
};

// Soldier_Armored 원본 거리/분기 규칙
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSoldierArmoredHammerFlowConfig
{
	GENERATED_BODY()

	// HammerCombo_02 분기 최대 유효 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0"))
	float Combo2MaxRange = 0.0f;

	// Finisher_01/02 분기 최대 유효 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0"))
	float FinisherMaxRange = 0.0f;

	// HammerCombo_01 윈도우에서 Finisher_01 진행 확률이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Finisher1Chance = 0.0f;
};

// Soldier_Armored 전투 이동 오프셋 규칙
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSoldierArmoredOffsetMoveConfig
{
	GENERATED_BODY()

	// 타겟 기준 목표 지점의 최소 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float MinOffsetDistance = 300.0f;

	// 타겟 기준 목표 지점의 최대 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float MaxOffsetDistance = 420.0f;

	// 현재 타겟-자기 자신 축에서 좌우로 벌어질 최소 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MinOffsetAngleDegrees = 35.0f;

	// 현재 타겟-자기 자신 축에서 좌우로 벌어질 최대 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MaxOffsetAngleDegrees = 70.0f;

	// 왼쪽 오프셋 사용 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	bool bAllowLeft = true;

	// 오른쪽 오프셋 사용 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	bool bAllowRight = true;

	// NavMesh 투영 시 사용할 탐색 범위다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FVector NavProjectionExtent = FVector(160.0f, 160.0f, 200.0f);
};

// Soldier_Armored 원본형 전투 흐름 조정용 DataAsset
UCLASS(BlueprintType)
class PROJECTR_API UPRSoldierArmoredCombatDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 해머 패밀리 몽타주 섹션 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredHammerSectionNames HammerSections;

	// 원본 거리와 확률 기반 콤보 분기 규칙이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredHammerFlowConfig HammerFlow;

	// HammerCombo_01 타격값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredAttackHitConfig Combo1HitConfig;

	// HammerCombo_02 타격값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredAttackHitConfig Combo2HitConfig;

	// Finisher_01 타격값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredAttackHitConfig Finisher1HitConfig;

	// Finisher_02 타격값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredAttackHitConfig Finisher2HitConfig;

	// SprintHammer 타격값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Sprint")
	FPRSoldierArmoredAttackHitConfig SprintHammerHitConfig;

	// Combat_Strafe 목표 지점 생성 규칙이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FPRSoldierArmoredOffsetMoveConfig CombatStrafeConfig;

	// Combat_Reposition 목표 지점 생성 규칙이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Movement")
	FPRSoldierArmoredOffsetMoveConfig CombatRepositionConfig;
};
