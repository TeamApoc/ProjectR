// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "PRAttributeSet_Player.generated.h"

// 플레이어 전용 자원(스태미너·Mod 게이지·탄약) 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Player : public UAttributeSet
{
	GENERATED_BODY()

public:
	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	// Mod 특수기 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ModGauge, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData ModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, ModGauge)

	// 최대 Mod 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxModGauge, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData MaxModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, MaxModGauge)

	// 주무기 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryAmmo, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PrimaryAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PrimaryAmmo)

	// 주무기 탄창 용량
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxPrimaryAmmo, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData MaxPrimaryAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, MaxPrimaryAmmo)

	// 보조무기 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryAmmo, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData SecondaryAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, SecondaryAmmo)

	// 보조무기 탄창 용량
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSecondaryAmmo, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData MaxSecondaryAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, MaxSecondaryAmmo)

protected:
	UFUNCTION() 
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);
	UFUNCTION() 
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);
	UFUNCTION() 
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);
	UFUNCTION() 
	void OnRep_ModGauge(const FGameplayAttributeData& OldValue);
	UFUNCTION() 
	void OnRep_MaxModGauge(const FGameplayAttributeData& OldValue);
	UFUNCTION() 
	void OnRep_PrimaryAmmo(const FGameplayAttributeData& OldValue);
	UFUNCTION() 
	void OnRep_MaxPrimaryAmmo(const FGameplayAttributeData& OldValue);
	UFUNCTION() 
	void OnRep_SecondaryAmmo(const FGameplayAttributeData& OldValue);
	UFUNCTION() 
	void OnRep_MaxSecondaryAmmo(const FGameplayAttributeData& OldValue);
};
