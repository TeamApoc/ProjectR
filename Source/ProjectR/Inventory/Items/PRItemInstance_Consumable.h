// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRItemInstance.h"
#include "PRItemInstance_Consumable.generated.h"

class UPRConsumableDataAsset;

// 인벤토리가 소유하는 소비 아이템 1종의 보유 개수와 사용 처리를 담당한다
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRItemInstance_Consumable : public UPRItemInstance
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 소비 아이템 데이터와 초기 보유 개수를 연결한다
	void InitializeConsumableItem(UPRConsumableDataAsset* InConsumableData, int32 InitialStackCount);

	// 소비 아이템을 사용한다
	virtual bool UseItem(AActor* UserActor);

	// 현재 연결된 소비 아이템 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRConsumableDataAsset* GetConsumableData() const { return ConsumableData; }

	// 현재 보유 개수를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	int32 GetStackCount() const { return StackCount; }

	// 보유 개수가 1 이상인지 확인한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	bool HasAnyStack() const;

	// 보상, 픽업, 상점 구매로 보유 개수를 증가시킨다
	bool AddStack(int32 AddCount);

	// 판매, 보정, 디버그 등 사용 외 경로로 보유 개수를 감소시킨다
	bool RemoveStack(int32 RemoveCount);

protected:
	// 사용 가능한 상태인지 확인한다
	virtual bool CanUseItem(AActor* UserActor) const;

private:
	// 보유 개수 복제 결과로 인벤토리 UI 갱신 신호를 발행한다
	UFUNCTION()
	void OnRep_StackCount();

public:
	// 소비 아이템 정적 데이터
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UPRConsumableDataAsset> ConsumableData = nullptr;

	// 현재 보유 개수
	UPROPERTY(ReplicatedUsing = OnRep_StackCount, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	int32 StackCount = 0;
};
