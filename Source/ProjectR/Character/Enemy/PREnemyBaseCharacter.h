// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "PREnemyBaseCharacter.generated.h"

class UBehaviorTree;
class UBoxComponent;
class USphereComponent;
class UPRAbilitySystemComponent;
class UPRAttributeSet_Common;
class UPRAttributeSet_Enemy;
class UPRPatternDataAsset;
class UPRPerceptionConfig;
class UPREnemyCombatEventRelayComponent;
class UPREnemyThreatComponent;

UCLASS(Abstract)
class PROJECTR_API APREnemyBaseCharacter : public APRCharacterBase, public IPREnemyInterface
{
	GENERATED_BODY()

public:
	APREnemyBaseCharacter();

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const override;
	virtual EPRCharacterRole GetCharacterRole() const { return EPRCharacterRole::Enemy; }

public:
	virtual UPRAbilitySystemComponent* GetEnemyAbilitySystemComponent() const override;
	virtual UPREnemyThreatComponent* GetEnemyThreatComponent() const override;
	virtual UPRPatternDataAsset* GetPatternDataAsset() const override;
	virtual UPRPerceptionConfig* GetPerceptionConfig() const override;
	virtual UBehaviorTree* GetBehaviorTreeAsset() const override;
	virtual FVector GetHomeLocation() const override;

	const UPRAttributeSet_Common* GetCommonSet() const { return CommonSet; }
	const UPRAttributeSet_Enemy* GetEnemySet() const { return EnemySet; }
	const UBoxComponent* GetArmorCollision() const { return ArmorCollision; }
	const USphereComponent* GetWeakpointCollision() const { return WeakpointCollision; }

protected:
	void InitializeEnemyAbilitySystem();
	void BindGameplayTagEvents();

	UFUNCTION()
	void HandleDeath(AActor* InstigatorActor);

	UFUNCTION()
	void HandleGroggyStateChanged(bool bEntered);

	void HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleGroggyTagChanged(const FGameplayTag Tag, int32 NewCount);

protected:
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Ability")
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Common> CommonSet;

	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Enemy> EnemySet;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI")
	TObjectPtr<UPREnemyThreatComponent> ThreatComponent;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI")
	TObjectPtr<UPREnemyCombatEventRelayComponent> CombatEventRelayComponent;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UPRPatternDataAsset> PatternDataAsset;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UPRPerceptionConfig> PerceptionConfig;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Combat")
	TObjectPtr<UBoxComponent> ArmorCollision;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Combat")
	TObjectPtr<USphereComponent> WeakpointCollision;

	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|AI")
	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY()
	FPRAbilitySetHandles GrantedAbilitySetHandles;

	bool bGameplayTagEventsBound = false;
};
