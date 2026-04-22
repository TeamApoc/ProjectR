// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "PRCombatTypes.generated.h"

class AActor;
class UObject;

// HitResult가 어떤 특수 부위에 맞았는지 나타낸다.
// ComponentTag의 Armor.* / Weakpoint.* 접두사를 해석한 결과다.
UENUM(BlueprintType)
enum class EPRDamageRegionType : uint8
{
	None		UMETA(DisplayName = "None"),
	Armor		UMETA(DisplayName = "Armor"),
	Weakpoint	UMETA(DisplayName = "Weakpoint")
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDamageRegionInfo
{
	GENERATED_BODY()

	// 일반 부위, 갑옷, 약점 중 어디에 맞았는지 나타낸다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	EPRDamageRegionType RegionType = EPRDamageRegionType::None;

	// 실제 컴포넌트 태그 이름이다. 예: Armor.Torso, Weakpoint.Head
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	FName RegionTag = NAME_None;

	bool IsArmor() const
	{
		return RegionType == EPRDamageRegionType::Armor;
	}

	bool IsWeakpoint() const
	{
		return RegionType == EPRDamageRegionType::Weakpoint;
	}
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDamageContext
{
	GENERATED_BODY()

	// 피해를 발생시킨 Actor다. 일반적으로 Ability를 실행한 캐릭터다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	TObjectPtr<AActor> SourceActor = nullptr;

	// 피해를 받을 Actor다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	TObjectPtr<AActor> TargetActor = nullptr;

	// GE Context에 기록될 EffectCauser다. 투사체/무기 Actor가 있으면 여기에 들어간다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	TObjectPtr<AActor> EffectCauser = nullptr;

	// Ability 또는 데이터 에셋처럼 피해의 출처를 추적할 객체다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	TObjectPtr<UObject> SourceObject = nullptr;

	// 피격 지점과 피격 컴포넌트를 담는다. 갑옷/약점 판정의 입력값이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	FHitResult HitResult;

	// 어떤 Ability가 만든 피해인지 추적하기 위한 태그다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	FGameplayTag SourceAbilityTag;

	// Health에 적용할 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 0.0f;

	// 적 전용 GroggyGauge에 적용할 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 0.0f;

	// GE Spec Level로 전달할 값이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AbilityLevel = 1.0f;

	bool HasValidPayload() const
	{
		return Damage > 0.0f || GroggyDamage > 0.0f;
	}
};

namespace PRCombatSetByCaller
{
	// GE_Damage가 SetByCaller로 읽는 데이터 키다.
	static const FName Damage(TEXT("Data.Damage"));
	static const FName GroggyDamage(TEXT("Data.GroggyDamage"));
}
