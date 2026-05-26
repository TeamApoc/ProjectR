// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "PRGrowthTypes.generated.h"

// 특성 투자 대상 능력치 종류
UENUM(BlueprintType)
enum class EPRTraitStatType : uint8
{
	// 최대 체력
	MaxHealth,
	// 방어력
	Armor,
	// 이동속도
	MovementSpeed,
	// 플레이어 공격력
	AttackPower,
	// 최대 스태미너
	MaxStamina,
	// 치명타 확률
	CriticalHitChance,
	// 치명타 피해 배율
	CriticalDamageMultiplier
};

// 능력치별 특성 투자 포인트 내역
USTRUCT(BlueprintType)
struct PROJECTR_API FPRTraitInvestmentInfo
{
	GENERATED_BODY()

public:
	// 최대 체력 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHealth = 0;

	// 방어력 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Armor = 0;

	// 이동속도 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MovementSpeed = 0;

	// 플레이어 공격력 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AttackPower = 0;

	// 최대 스태미너 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStamina = 0;

	// 치명타 확률 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CriticalHitChance = 0;

	// 치명타 피해 배율 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CriticalDamageMultiplier = 0;
};

// 레벨별 누적 필요 경험치 Row
USTRUCT(BlueprintType)
struct PROJECTR_API FPRLevelExperienceRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 해당 레벨에 도달하기 위한 누적 경험치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 RequiredTotalExperience = 0;

	// 해당 레벨업으로 지급할 특성 포인트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 TraitPointReward = 2;
};

// 특성별 투자 공식 Row
USTRUCT(BlueprintType)
struct PROJECTR_API FPRTraitStatRuleRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 적용할 특성 종류
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPRTraitStatType TraitType = EPRTraitStatType::MaxHealth;

	// 보너스를 적용할 대상 Attribute
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute TargetAttribute;

	// 포인트당 증가량
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ValuePerPoint = 0.0f;

	// 최대 투자 포인트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 MaxPoint = 0;
};
