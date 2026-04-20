// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "PRAbilitySystemComponent.generated.h"

struct FPRAbilitySetHandles;
class UPRAbilitySet;
class UPRAbilitySystemRegistry;

// 프로젝트 공통 ASC. AbilitySet 부여/해제, 속성 초기화, 입력 라우팅, AI 활성화 헬퍼 제공
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;

	/*~ UAbilitySystemComponent Interface ~*/
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

public:
	// 어빌리티 활성화 직후 발행 (Ability 첫 태그 기준)
	UPROPERTY(BlueprintAssignable)
	FPRAbilityActivatedSignature OnAbilityActivated;

	// 어빌리티 종료 시 발행. bCancelled == true 면 Cancel 경로
	UPROPERTY(BlueprintAssignable)
	FPRAbilityEndedSignature OnAbilityEnded;

	// AbilitySet 일괄 부여. 서버 전용. OutHandles에 Clear용 대칭 핸들 누적
	void GiveAbilitySet(const UPRAbilitySet* AbilitySet, FPRAbilitySetHandles& OutHandles);

	// 이전 부여 시 받은 핸들로 ClearAbility · RemoveActiveGameplayEffect 후 Reset
	void ClearAbilitySetByHandles(FPRAbilitySetHandles& Handles);

	// Registry 기반 Row 리플렉션 주입으로 속성 초기화. 서버 전용. 1회 호출
	bool InitializeAttributesFromRegistry(const UPRAbilitySystemRegistry* Registry,
	                                       EPRCharacterRole Role, FName RowName);

	// 플레이어 입력 Pressed. DynamicSpecSourceTags에 InputTag 있는 Spec들을 Pressed/Held 목록에 추가
	void AbilityInputPressed(const FGameplayTag& InputTag);

	// Released 콜백
	void AbilityInputReleased(const FGameplayTag& InputTag);

	// PlayerController Tick 말미에서 호출. 3개 Handle 목록 소비
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	// 폰 변경·일시정지 등에서 3개 Handle 목록을 모두 비움
	void ClearAbilityInput();

	// AbilityTag(= AbilityTags 매칭)로 서버에서 활성화. BT Task용
	bool TryActivateAbilityOnServer(const FGameplayTag& AbilityTag,
	                                 FGameplayAbilitySpecHandle& OutHandle);

	// 활성화 가능 여부 사전 검사. 실패 시 Fail.* 태그 누적
	bool CanActivateAbilityByTag(const FGameplayTag& AbilityTag,
	                              FGameplayTagContainer& OutFailureTags) const;

protected:
	// 어빌리티 종료 시 델리게이트 브로드캐스트
	UFUNCTION()
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);

	// 어빌리티 활성화 시 델리게이트 브로드캐스트
	void HandleAbilityActivated(UGameplayAbility* ActivatedAbility);

protected:
	// 이번 프레임 Pressed 후보 Spec Handle
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// 입력이 계속 눌려있는 Spec Handle
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	// 이번 프레임 Released된 Spec Handle
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
};
