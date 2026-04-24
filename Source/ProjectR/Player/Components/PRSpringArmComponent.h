// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "PRSpringArmComponent.generated.h"

/**
* 카메라 붐의 길이(TargetArmLength)와 오프셋(SocketOffset) 보간을 전담합니다.
* 캐릭터의 Tick을 비활성화한 상태에서도 매 프레임 자연스러운 카메라 이동을 처리합니다.
 */
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()
	
public:
	UPRSpringArmComponent();

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	bool bIsAimingOverride = false;
	float AimTargetArmLength = 150.0f;
	FVector AimTargetOffset = FVector::ZeroVector;
	FVector AimSocketOffset = FVector::ZeroVector;

protected:
	/** 보간 속도 설정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	float ArmLengthInterpSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Camera")
	float SocketOffsetInterpSpeed = 5.0f;
};
