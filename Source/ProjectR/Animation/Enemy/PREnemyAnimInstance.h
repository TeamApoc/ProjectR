// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PREnemyAnimInstance.generated.h"

class UCharacterMovementComponent;
class APREnemyBaseCharacter;

// 적 ABP가 공통으로 읽는 애니메이션 변수들을 계산하는 AnimInstance다.
// 이동 속도와 GAS 상태 태그를 캐싱해 블루프린트 StateMachine을 단순하게 유지한다.
UCLASS()
class PROJECTR_API UPREnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	void RefreshOwnerReferences();

	// Velocity/XYSpeed/낙하 여부처럼 매 프레임 바뀌는 이동 값을 갱신한다.
	void UpdateMovementData();

	// ASC 태그를 읽어 Dead/Groggy 상태 플래그를 갱신한다.
	void UpdateStateFlags();

public:
	// 이 AnimInstance가 붙어 있는 적 캐릭터다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	TObjectPtr<APREnemyBaseCharacter> EnemyCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	float XYSpeed = 0.0f;

	// Locomotion StateMachine에서 Idle/Move 전환에 사용한다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bIsGroggy = false;

	// 아주 작은 속도 흔들림으로 Move 상태에 들어가는 것을 막기 위한 기준값이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	float MoveSpeedThreshold = 3.0f;
};
