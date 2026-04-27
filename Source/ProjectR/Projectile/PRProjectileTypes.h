// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "PRProjectileTypes.generated.h"

class APRProjectileBase;
DECLARE_DYNAMIC_DELEGATE_OneParam(FProjectileSpawnedDelegate, APRProjectileBase*, SpawnedProjectile);

// 투사체 스폰 파라미터. Client/Server 양측에서 동일 ID로 매칭하기 위한 정보 묶음
USTRUCT(BlueprintType)
struct FPRProjectileSpawnInfo
{
	GENERATED_BODY()

	// 투사체 식별자. 0은 무효
	uint32 ProjectileId = 0;

	// 스폰할 투사체 클래스
	TSubclassOf<APRProjectileBase> ProjectileClass;

	// 스폰 위치
	FVector SpawnLocation = FVector::ZeroVector;

	// 스폰 회전
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// 추가 스폰 옵션. 기본값은 호출자가 채워서 전달
	FActorSpawnParameters SpawnParameters;
};

// 클라이언트가 서버로 전달하는 투사체 스폰 정보. GAS TargetData로 송신
USTRUCT()
struct PROJECTR_API FPRTargetData_SpawnProjectile : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	// 클라이언트가 발급한 투사체 ID
	UPROPERTY()
	uint32 ProjectileId = 0;

	// 스폰 위치
	UPROPERTY()
	FVector_NetQuantize10 SpawnLocation = FVector_NetQuantize10::ZeroVector;

	// 스폰 회전
	UPROPERTY()
	FRotator SpawnRotation = FRotator::ZeroRotator;

	/*~ FGameplayAbilityTargetData Interface ~*/
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FPRTargetData_SpawnProjectile::StaticStruct();
	}
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FPRTargetData_SpawnProjectile{Id=%u}"), ProjectileId);
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRTargetData_SpawnProjectile> : public TStructOpsTypeTraitsBase2<FPRTargetData_SpawnProjectile>
{
	enum
	{
		WithNetSerializer = true
	};
};