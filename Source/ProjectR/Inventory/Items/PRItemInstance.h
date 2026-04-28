// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PRItemInstance.generated.h"

// 인벤토리가 소유하는 공용 Item 인스턴스의 최소 기반 클래스다
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UPRItemInstance();

	/*~ UObject Interface ~*/
public:
	virtual void PostInitProperties() override;
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Item 식별자를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Item")
	FGuid GetItemId() const { return ItemId; }

protected:
	// Item 식별자가 비어 있으면 새 식별자를 발급한다
	void EnsureItemId();

public:
	// Item 고유 식별자
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Item")
	FGuid ItemId;
};
