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

	// Health 적용 피해량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 0.0f;

	// GroggyGauge 적용 피해량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 0.0f;

	// 해당 섹션 타격 Sweep 전방 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 0.0f;

	// 해당 섹션 타격 Sweep 반경
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRadius = 0.0f;
};

// Soldier_Armored 해머 패밀리 몽타주 섹션 이름 묶음
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSoldierArmoredHammerSectionNames
{
	GENERATED_BODY()

	// HammerCombo_01 시작 섹션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Combo1StartSection = NAME_None;

	// HammerCombo_02 연계 시작 섹션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Combo2EnterSection = NAME_None;

	// HammerCombo_01 직후 피니셔 섹션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Finisher1EnterSection = NAME_None;

	// HammerCombo_02 직후 피니셔 섹션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Finisher2EnterSection = NAME_None;
};

// Soldier_Armored 원본 거리/분기 규칙
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSoldierArmoredHammerFlowConfig
{
	GENERATED_BODY()

	// HammerCombo_02 연계 최대 유효 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0"))
	float Combo2MaxRange = 0.0f;

	// Finisher_01/02 연계 최대 유효 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0"))
	float FinisherMaxRange = 0.0f;

	// HammerCombo_01 윈도우에서 Finisher_01 직행 확률
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Finisher1Chance = 0.0f;
};

// Soldier_Armored 원본형 전투 흐름 조정용 DataAsset
UCLASS(BlueprintType)
class PROJECTR_API UPRSoldierArmoredCombatDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 해머 패밀리 몽타주 섹션 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredHammerSectionNames HammerSections;

	// 원본 거리와 확률 기반 콤보 분기 규칙
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredHammerFlowConfig HammerFlow;

	// HammerCombo_01 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredAttackHitConfig Combo1HitConfig;

	// HammerCombo_02 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredAttackHitConfig Combo2HitConfig;

	// Finisher_01 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredAttackHitConfig Finisher1HitConfig;

	// Finisher_02 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredAttackHitConfig Finisher2HitConfig;

	// SprintHammer 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Sprint")
	FPRSoldierArmoredAttackHitConfig SprintHammerHitConfig;
};
