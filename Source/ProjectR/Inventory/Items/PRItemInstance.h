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
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
