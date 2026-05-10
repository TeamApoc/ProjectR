// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/WeaponStatusHUD/FPRWeaponStatusViewData.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRWeaponStatusWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;

// 무기 하나의 아이콘, 탄창, 잔탄, Mod 상태를 표시하는 HUD 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWeaponStatusWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 주무기 또는 보조무기 슬롯 타입을 고정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetSlotType(EPRWeaponSlotType InSlotType);

	// 표시 데이터 전체를 한 번에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetWeaponStatus(const FPRWeaponStatusViewData& ViewData);

	// 무기 아이콘을 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetWeaponIcon(UTexture2D* Icon);

	// 탄창 용량 텍스트를 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetMagazineAmmoText(float MagazineAmmo);

	// 잔탄 용량 표시 텍스트를 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetReserveAmmoText(float ReserveAmmo);

	// Mod 아이콘을 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetModIcon(UTexture2D* Icon);

	// 원형 게이지 바 퍼센트를 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetModGaugePercent(float Percent);

	// Mod 스택 개수 텍스트를 화면에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void SetModStackText(float StackCount);

	// 무기가 없는 슬롯의 기본 표시 상태로 되돌린다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void ClearWeaponStatus();

	// Mod가 없는 슬롯의 기본 표시 상태로 되돌린다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Weapon")
	void ClearModStatus();

protected:
	// UMG에서 바인딩할 무기 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UImage> WeaponIconImage;

	// UMG에서 바인딩할 탄창 용량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTextBlock> MagazineAmmoText;

	// UMG에서 바인딩할 잔탄 용량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTextBlock> ReserveAmmoText;

	// UMG에서 바인딩할 Mod 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UImage> ModIconImage;

	// UMG에서 바인딩할 원형 게이지 바
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UProgressBar> ModGaugeBar;

	// UMG에서 바인딩할 Mod 스택 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Weapon")
	TObjectPtr<UTextBlock> ModStackText;

private:
	// 이 위젯이 담당하는 무기 슬롯 타입
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|HUD|Weapon", meta = (AllowPrivateAccess = "true"))
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
};
