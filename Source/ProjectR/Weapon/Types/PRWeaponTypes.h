// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "PRWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EPRWeaponSlotType : uint8
{
	None,
	Primary,
	Secondary
};

UENUM(BlueprintType)
enum class EPRWeaponArmedState : uint8
{
	Armed,
	Unarmed
};

UENUM(BlueprintType)
enum class EPRWeaponCarryState : uint8
{
	Armed,
	Stowed
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRActiveWeaponSlot
{
	GENERATED_BODY()

public:
	// 현재 활성 슬롯 구분
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;

	// 현재 공개 중인 무기 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponDataAsset> WeaponData = nullptr;

public:
	// 현재 공개 상태가 비어 있는지 확인한다
	bool IsEmpty() const
	{
		return SlotType == EPRWeaponSlotType::None || !IsValid(WeaponData);
	}

	// 공개 상태를 초기화한다
	void Reset()
	{
		SlotType = EPRWeaponSlotType::None;
		WeaponData = nullptr;
	}

	// 두 공개 상태가 같은 무기를 가리키는지 확인한다
	bool operator==(const FPRActiveWeaponSlot& Other) const
	{
		return SlotType == Other.SlotType && WeaponData == Other.WeaponData;
	}

	// 두 공개 상태가 다른지 확인한다
	bool operator!=(const FPRActiveWeaponSlot& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponVisualSlot
{
	GENERATED_BODY()

public:
	// 현재 공개 상태가 어떤 슬롯을 나타내는지 식별한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;

	// 슬롯에 장착된 공개 무기 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponDataAsset> WeaponData = nullptr;

	// 현재 슬롯 무기가 손에 들린 상태인지 수납 상태인지 나타낸다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon")
	EPRWeaponCarryState CarryState = EPRWeaponCarryState::Stowed;

public:
	// 현재 공개 비주얼 상태가 비어 있는지 확인한다
	bool IsEmpty() const
	{
		return SlotType == EPRWeaponSlotType::None || !IsValid(WeaponData);
	}

	// 슬롯 타입을 유지한 채 공개 비주얼 상태를 초기화한다
	void Reset(EPRWeaponSlotType InSlotType = EPRWeaponSlotType::None)
	{
		SlotType = InSlotType;
		WeaponData = nullptr;
		CarryState = EPRWeaponCarryState::Stowed;
	}

	// 두 공개 비주얼 상태가 같은 무기와 같은 휴대 상태를 가리키는지 확인한다
	bool operator==(const FPRWeaponVisualSlot& Other) const
	{
		return SlotType == Other.SlotType
			&& WeaponData == Other.WeaponData
			&& CarryState == Other.CarryState;
	}

	// 두 공개 비주얼 상태가 다른지 확인한다
	bool operator!=(const FPRWeaponVisualSlot& Other) const
	{
		return !(*this == Other);
	}
};
