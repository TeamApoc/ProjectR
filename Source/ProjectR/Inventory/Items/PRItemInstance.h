// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PRItemInstance.generated.h"

class UPRItemDataAsset;
enum class EPRInventoryChangeReason : uint8;

// 인벤토리가 소유하는 공용 Item 인스턴스의 최소 기반 클래스다
UCLASS(BlueprintType)
class PROJECTR_API UPRItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UPRItemInstance();

	/*~ UObject Interface ~*/
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~UPRItemInstance Interface ~*/
	virtual void InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount);
	
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

public:
	// 소유 인벤토리에 변경 이벤트를 전달
	void NotifyInventoryChanged(EPRInventoryChangeReason ChangeReason);
	
protected:
	// 보유 개수 복제 결과로 인벤토리 UI 갱신 신호를 발행
	UFUNCTION()
	virtual void OnRep_StackCount(const int32& OldStackCount);
	
protected:
	// 현재 보유 개수
	UPROPERTY(ReplicatedUsing = OnRep_StackCount, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	int32 StackCount = 0;
	
	// 소비 아이템 정적 데이터
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemDataAsset> ItemData = nullptr;

};
