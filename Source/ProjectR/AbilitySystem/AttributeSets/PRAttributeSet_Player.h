// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRAttributeSet_Player.generated.h"

class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;

// 플레이어 전용 자원과 무기 슬롯 자원 정본을 관리하는 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Player : public UAttributeSet
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
public:
	// 스태미너
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData Stamina;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, Stamina)

	// 최대 스태미너
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData MaxStamina;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, MaxStamina)

	// 초당 스태미너 회복량
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData StaminaRegenRate;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, StaminaRegenRate)

	// 치명타 확률
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalHitChance, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData CriticalHitChance;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, CriticalHitChance)

	// 치명타 피해 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamageMultiplier, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData CriticalDamageMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, CriticalDamageMultiplier)

protected:
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CriticalHitChance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CriticalDamageMultiplier(const FGameplayAttributeData& OldValue);


};
