// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRFaeRoyalArcherCharacter.generated.h"

class UPRFaeRoyalArcherCombatDataAsset;

// Fae Royal Archer 전용 캐릭터 클래스다.
// 공중 이동 기본값과 궁수 전용 데이터 접근을 제공한다.
UCLASS()
class PROJECTR_API APRFaeRoyalArcherCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	// Fae Royal Archer 기본 CharacterID와 비행 이동 값을 초기화한다.
	APRFaeRoyalArcherCharacter();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;

public:
	// 궁수 전용 전투 데이터 자산을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|RoyalArcher")
	UPRFaeRoyalArcherCombatDataAsset* GetRoyalArcherCombatData() const;

	// 화살 발사 소켓이 없을 때 사용할 fallback 본 이름을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|RoyalArcher")
	FName GetProjectileFallbackSocketName() const { return ProjectileFallbackSocketName; }

protected:
	// CharacterMovement에 Royal Archer의 기본 비행 이동 정책을 적용한다.
	void ApplyRoyalArcherMovementDefaults();

protected:
	// BeginPlay 시 Flying 이동 모드로 시작할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Flight")
	bool bStartInFlyingMode = true;

	// 기본 공중 이동 최대 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float DefaultMaxFlySpeed = 420.0f;

	// Flying 이동 중 제동 감속 값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float DefaultBrakingDecelerationFlying = 800.0f;

	// Flying 이동 중 회전 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Flight", meta = (ClampMin = "0.0"))
	float DefaultFlightRotationYawRate = 540.0f;

	// ProjectileSocket이 아직 없을 때 단발 사격 기준으로 사용할 본 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|RoyalArcher|Projectile")
	FName ProjectileFallbackSocketName = TEXT("Bone_FA_Weapon_Arrow");
};
