// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PRDestructableInterface.generated.h"

class AActor;

// 파괴 가능 대상 피해 수신 컨텍스트
USTRUCT(BlueprintType)
struct PROJECTR_API FPRDestructableDamageReceiveContext
{
	GENERATED_BODY()

	// 공격 주체
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Destructable")
	TWeakObjectPtr<AActor> Instigator;

	// 피해량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Destructable")
	float DamageAmount = 0.0f;
};

// 파괴 가능 대상 피해 적용 컨텍스트
USTRUCT(BlueprintType)
struct PROJECTR_API FPRDestructableDamageAppliedContext
{
	GENERATED_BODY()

	// 공격 주체
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Destructable")
	TWeakObjectPtr<AActor> Instigator;

	// 피해량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Destructable")
	float DamageAmount = 0.0f;
};

// 파괴 가능 대상 UInterface
UINTERFACE(MinimalAPI, Blueprintable)
class UPRDestructableInterface : public UInterface
{
	GENERATED_BODY()
};

// 파괴 가능 대상 공통 인터페이스
class PROJECTR_API IPRDestructableInterface
{
	GENERATED_BODY()

public:
	// 피해 수신 진입점
	virtual bool ReceiveDamageContext(const FPRDestructableDamageReceiveContext& Context) { return false; }
};
