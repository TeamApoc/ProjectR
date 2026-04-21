// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "PRCombatTypes.generated.h"

class AActor;
class UObject;

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

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Combat")
	EPRDamageRegionType RegionType = EPRDamageRegionType::None;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	TObjectPtr<AActor> EffectCauser = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	TObjectPtr<UObject> SourceObject = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	FHitResult HitResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat")
	FGameplayTag SourceAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AbilityLevel = 1.0f;

	bool HasValidPayload() const
	{
		return Damage > 0.0f || GroggyDamage > 0.0f;
	}
};

namespace PRCombatSetByCaller
{
	static const FName Damage(TEXT("Data.Damage"));
	static const FName GroggyDamage(TEXT("Data.GroggyDamage"));
}
