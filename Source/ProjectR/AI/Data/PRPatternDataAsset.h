// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRPatternDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRPatternDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// 이 적이 사용할 패턴 선택 규칙 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	TArray<FPRPatternRule> PatternRules;
};
