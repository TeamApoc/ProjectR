// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRWeaponTypes.generated.h"

class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;

UENUM(BlueprintType)
enum class EPRWeaponType : uint8
{
	None,
	Rifle,
	GrenadeLauncher,
	BoltAction,
	Pistol
};

UENUM(BlueprintType)
enum class EPRWeaponSlotType : uint8
{
	None,
	Primary,
	Secondary
};

UENUM(BlueprintType)
enum class EPRArmedState : uint8
{
	Armed,
	Unarmed
};

UENUM(BlueprintType)
enum class EPRWeaponFireModeState : uint8
{
	BaseFire,
	ModFire
};

UENUM(BlueprintType)
enum class EPRWeaponActionFailReason : uint8
{
	None,
	InvalidSource,
	NoMagazineAmmo,
	NoReserveAmmo,
	BlockedByState,
	InvalidRuntimeState
};

UENUM(BlueprintType)
enum class EPRWeaponModFailReason : uint8
{
	None,
	MissingMod,
	NoGauge,
	NoStack,
	AlreadyActive,
	BlockedByState
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponSlotResourceDelta
{
	GENERATED_BODY()

public:
	// 탄창 잔탄 변화량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	int32 MagazineDelta = 0;

	// 예비 탄약 변화량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	int32 ReserveDelta = 0;

	// Mod 게이지 변화량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	float ModGaugeDelta = 0.0f;

	// Mod 스택 변화량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	int32 ModStackDelta = 0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponSlotResourceState
{
	GENERATED_BODY()

public:
	// 어느 슬롯의 자원 상태인지 식별한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;

	// 현재 탄창 잔탄
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	int32 MagazineAmmo = 0;

	// 현재 예비 탄약
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	int32 ReserveAmmo = 0;

	// 현재 Mod 게이지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	float ModGauge = 0.0f;

	// 현재 Mod 스택
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	int32 ModStack = 0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponVisualInfo
{
	GENERATED_BODY()

public:
	// 현재 공개 상태가 어느 슬롯인지 식별한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;

	// 슬롯에 장착된 공개 무기 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponDataAsset> WeaponData = nullptr;

	// 슬롯에 장착된 공개 Mod 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponModDataAsset> ModData = nullptr;

public:
	// 현재 공개 비주얼 상태가 비어 있는지 확인한다
	bool IsEmpty() const
	{
		return SlotType == EPRWeaponSlotType::None || !WeaponData;
	}

	// 슬롯 타입을 유지한 채 공개 비주얼 상태를 초기화한다
	void Reset(EPRWeaponSlotType InSlotType = EPRWeaponSlotType::None)
	{
		SlotType = InSlotType;
		WeaponData = nullptr;
		ModData = nullptr;
	}

	// 두 공개 비주얼 상태가 같은 무기와 같은 Mod를 가리키는지 확인한다
	bool operator==(const FPRWeaponVisualInfo& Other) const
	{
		return SlotType == Other.SlotType
			&& WeaponData == Other.WeaponData
			&& ModData == Other.ModData;
	}

	// 두 공개 비주얼 상태가 다른지 확인한다
	bool operator!=(const FPRWeaponVisualInfo& Other) const
	{
		return !(*this == Other);
	}
};
