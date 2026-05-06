// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRInteractableComponent.generated.h"

class UPRInteractionAction;
class UPRInteractorComponent;
class UMeshComponent;

/**
 * 상호작용 가능한 오브젝트에 부착하는 컴포넌트.
 * UPRInteractionAction 목록을 보유하며, OnInteract 시 조건을 만족하는
 * 가장 높은 우선순위의 행동을 선택하여 실행한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRInteractableComponent();

	/*~ UActorComponent Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** 포커스 진입 시 호출. 소유 액터의 모든 메시에 오버레이 머티리얼 적용 */
	virtual void OnFocus(bool bIsInRange);

	/** 포커스 해제 시 호출. 소유 액터의 모든 메시에서 오버레이 머티리얼 제거 */
	virtual void OnUnfocus();

	/** 상호작용 시 호출. 조건을 만족하는 최고 우선순위 행동을 실행한다 */
	virtual void OnInteract(AActor* Interactor, int32 ActionIndex);

	/** 조건을 만족하는 최고 우선순위 Action 선택 (실행은 안 함). 없으면 nullptr */
	UPRInteractionAction* SelectBestAction(AActor* Interactor) const;

	/** 실행 가능한 행동이 하나라도 있는지 여부 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool HasAvailableAction(AActor* Interactor) const;

	/** 활성 유지형 상호작용을 종료한다 */
	void EndActiveInteraction();

	/** 현재 활성 Action 반환 (nullptr이면 비활성) */
	UPRInteractionAction* GetActiveAction() const { return ActiveAction; }

	/** Index로부터 Action 반환 (nullptr이면 InValid Index) */
	UPRInteractionAction* FindActionByIndex(int32 InActionIndex) const;
	
	int32 FindActionIndex(UPRInteractionAction* InAction) const;
	
	/** 현재 사용 중인 Interactor 반환 (없으면 nullptr) */
	AActor* GetCurrentInteractor() const { return CurrentInteractor; }

	/** 현재 사용 중 여부 반환 (배타 점유 사용 시에만 의미 있음) */
	bool IsBeingInteracted() const { return ::IsValid(CurrentInteractor); }

	/** 배타 점유 사용 여부 반환 */
	bool IsExclusive() const { return bExclusiveInteraction; }

	/** 주어진 Interactor가 점유 정책상 진입 가능한지 여부 반환 */
	bool CanBeInteractedBy(AActor* Interactor) const;

	/** 배타 점유 시 현재 Interactor를 기록 */
	void AcquireExclusive(AActor* Interactor);

	/** 배타 점유 해제 */
	void ReleaseExclusive();

protected:
	UFUNCTION()
	void OnRep_CurrentInteractor();

public:
	// 이 컴포넌트에 등록된 상호작용 행동 목록
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Interaction")
	TArray<TObjectPtr<UPRInteractionAction>> InteractionActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText FallbackDisplayName;

	// 동시에 한 명만 상호작용 가능 여부. true면 CurrentInteractor로 점유 추적
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bExclusiveInteraction = false;

private:
	// 현재 포커스 상태 여부
	bool bIsFocused = false;

	// 포커스 진입 전 메시별 RenderCustomDepth 원래 값 (복원용)
	UPROPERTY()
	TMap<TObjectPtr<UMeshComponent>, bool> SavedCustomDepthStates;

	// 현재 사용 중인 Interactor (없으면 nullptr)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentInteractor)
	TObjectPtr<AActor> CurrentInteractor;

	// 현재 활성 유지형 Action (서버 전용 추적값)
	UPROPERTY()
	TObjectPtr<UPRInteractionAction> ActiveAction;
};