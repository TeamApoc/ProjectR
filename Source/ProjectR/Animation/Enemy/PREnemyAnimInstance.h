// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PREnemyAnimInstance.generated.h"

class UCharacterMovementComponent;
class APREnemyBaseCharacter;

UCLASS()
class PROJECTR_API UPREnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	void RefreshOwnerReferences();
	void UpdateMovementData();
	void UpdateStateFlags();

public:
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	TObjectPtr<APREnemyBaseCharacter> EnemyCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	float XYSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation")
	bool bIsGroggy = false;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	float MoveSpeedThreshold = 3.0f;
};
