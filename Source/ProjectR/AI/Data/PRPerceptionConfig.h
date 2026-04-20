// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "PRPerceptionConfig.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRPerceptionConfig : public UDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// 기본 시야 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0"))
	float SightRadius = 1500.0f;

	// 시야 상실 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0"))
	float LoseSightRadius = 1800.0f;

	// 전방 시야 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngle = 90.0f;

	// 청각 감지 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0"))
	float HearingRange = 800.0f;

	// 지각 정보 유지 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0"))
	float StimulusMaxAge = 5.0f;

	// LOS 확인에 사용할 Trace 채널이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception")
	TEnumAsByte<ECollisionChannel> LOSCheckTraceChannel = ECC_Visibility;
};
