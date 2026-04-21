// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "PRPerceptionConfig.generated.h"

// 몬스터별 AI Perception 값을 에디터에서 조정하기 위한 데이터 에셋이다.
// AIController가 Possess 시점에 읽어서 Sight/Hearing 설정에 반영한다.
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

	// 타겟을 놓치기 전까지 허용하는 시야 반경이다. SightRadius보다 작으면 검증에서 실패한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0"))
	float LoseSightRadius = 1800.0f;

	// 정면 기준 감지 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngle = 90.0f;

	// 청각 자극을 감지할 수 있는 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0"))
	float HearingRange = 800.0f;

	// 감지 정보가 유효하다고 보는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception", meta = (ClampMin = "0.0"))
	float StimulusMaxAge = 5.0f;

	// LOS 확인에 사용할 Trace 채널이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Perception")
	TEnumAsByte<ECollisionChannel> LOSCheckTraceChannel = ECC_Visibility;
};
