// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/AbilitySystem/Data/PRStatRows.h"
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
class UPREnemyCombatDataAsset;
class UPREnemyCombatEventRelayComponent;
class UPREnemyThreatComponent;
struct FPREnemyMovePresentationConfig;

// 모든 일반 몬스터가 공유하는 베이스 캐릭터다.
// ASC/AttributeSet/Threat/BT/Perception을 소유하고, 서버 Possess 시 AbilitySet과 AI를 초기화한다.
UCLASS(Abstract)
class PROJECTR_API APREnemyBaseCharacter : public APRCharacterBase, public IPREnemyInterface
{
	GENERATED_BODY()

public:
	APREnemyBaseCharacter();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const override;
	virtual EPRCharacterRole GetCharacterRole() const { return EPRCharacterRole::Enemy; }

	/*~ IPRCombatInterface ~*/
	virtual EPRTeam GetTeam() const override { return EPRTeam::Enemy; }
	virtual FPRDamageRegionInfo GetDamageRegionInfo(FName BoneName) const override;

public:
	virtual UPRAbilitySystemComponent* GetEnemyAbilitySystemComponent() const override;
	virtual UPREnemyThreatComponent* GetEnemyThreatComponent() const override;
	virtual UPRPatternDataAsset* GetPatternDataAsset() const override;
	virtual UPREnemyCombatDataAsset* GetCombatDataAsset() const override;
	virtual UPRPerceptionConfig* GetPerceptionConfig() const override;
	virtual UBehaviorTree* GetBehaviorTreeAsset() const override;
	virtual FVector GetHomeLocation() const override;

	// EQS 기반 전투 이동 표현 문맥을 갱신한다.
	virtual void ApplyCombatMovePresentationContext(const FPREnemyMovePresentationConfig& PresentationConfig) override;

	// EQS 기반 전투 이동 표현 문맥을 초기화한다.
	virtual void ClearCombatMovePresentationContext() override;

	// 전투 이동 중 타겟 포커스 유지 문맥 여부를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Animation")
	bool ShouldMaintainCombatMoveFocus() const { return bMaintainCombatMoveFocus; }

	UFUNCTION(BlueprintPure, Category = "ProjectR|Animation")
	bool ShouldUseCombatMovePose() const { return bUseCombatMovePose; }

	// 전투 이동 중 AimOffset 사용 문맥 여부를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Animation")
	bool ShouldUseCombatAimOffset() const { return bUseCombatAimOffset; }

	UFUNCTION(BlueprintPure, Category = "ProjectR|Animation")
	bool ShouldUseCombatStrafeState() const { return bUseCombatStrafeState; }

	const UPRAttributeSet_Common* GetCommonSet() const { return CommonSet; }
	const UPRAttributeSet_Enemy* GetEnemySet() const { return EnemySet; }
	const UBoxComponent* GetArmorCollision() const { return ArmorCollision; }
	const USphereComponent* GetWeakpointCollision() const { return WeakpointCollision; }

protected:
	/*~ APRCharacterBase Interface ~*/
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists) override;

	// 서버에서 ASC ActorInfo, 초기 스탯, AbilitySet을 준비한다.
	void InitializeEnemyAbilitySystem();

	UFUNCTION()
	void HandleDeath(AActor* InstigatorActor);

	void HandleDeadTagChanged(bool bEntered);
	void HandleGroggyTagChanged(bool bEntered);

	// 전투 이동 프레젠테이션 적용 전 기존 이동 설정을 캐시한다.
	void CacheMovementPresentationDefaults();

	// 전투 이동 프레젠테이션 설정을 CharacterMovement에 적용한다.
	void ApplyMovementPresentationConfig(const FPREnemyMovePresentationConfig& PresentationConfig);

	// 캐시해 둔 기존 이동 설정을 복구한다.
	void RestoreMovementPresentationDefaults();

protected:
	// 적은 PlayerState가 아니라 캐릭터 자신이 ASC를 소유한다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Ability")
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Common> CommonSet;

	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Enemy> EnemySet;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI")
	TObjectPtr<UPREnemyThreatComponent> ThreatComponent;

	// 기존 BP 서브컴포넌트 참조 보호용 컴포넌트다. 상태 이벤트는 AttributeSet/BossBase가 직접 발행한다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI")
	TObjectPtr<UPREnemyCombatEventRelayComponent> CombatEventRelayComponent;

	// 이 몬스터가 Possess될 때 실행할 BT
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// 패턴 선택 규칙 데이터다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UPRPatternDataAsset> PatternDataAsset;

	// 적 전투 흐름과 공격 데이터를 담은 자산이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UPREnemyCombatDataAsset> CombatDataAsset;

	// 시야/청각 감지 설정 데이터다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UPRPerceptionConfig> PerceptionConfig;

	// Armor.* ComponentTag를 가진 피격 영역이다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Combat")
	TObjectPtr<UBoxComponent> ArmorCollision;

	// Weakpoint.* ComponentTag를 가진 피격 영역이다. 몬스터별로 필요 없으면 비활성화한다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Combat")
	TObjectPtr<USphereComponent> WeakpointCollision;

	// AI 복귀 기준 위치다. BeginPlay에서 현재 위치로 저장한다.
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|AI")
	FVector HomeLocation = FVector::ZeroVector;

	// 서버에서 부여한 Ability/GE를 수동 초기화할 때 추적하기 위한 핸들 묶음이다.
	UPROPERTY()
	FPRAbilitySetHandles GrantedAbilitySetHandles;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "ProjectR|Animation")
	bool bMaintainCombatMoveFocus = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "ProjectR|Animation")
	bool bUseCombatMovePose = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "ProjectR|Animation")
	bool bUseCombatAimOffset = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "ProjectR|Animation")
	bool bUseCombatStrafeState = false;

	bool bHasCachedMovementPresentation = false;
	float CachedMaxWalkSpeed = 0.0f;
	FRotator CachedRotationRate = FRotator::ZeroRotator;
	bool bCachedOrientRotationToMovement = true;
	bool bCachedUseControllerDesiredRotation = false;
	bool bCachedUseControllerRotationYaw = false;
	// 부위 매핑 조회용으로 보유한 StatRow 사본. InitializeEnemyAbilitySystem에서 채운다.
	FPREnemyStatRow CachedStatRow;
	bool bHasCachedStatRow = false;

	bool bGameplayTagEventsBound = false;
};
