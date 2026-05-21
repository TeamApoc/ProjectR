// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRUIControllerComponent.generated.h"

class APlayerController;
class UUserWidget;
class UPRHUDWidget;
class UPRInventoryComponent;
class UPRInventoryWidget;
class UPRQuickSlotComponent;
class UPRUIManagerSubsystem;
class UPRWeaponManagerComponent;

// 플레이어 컨트롤러에 부착되어 로컬 플레이어 UI 표시 흐름을 관리한다
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRUIControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// UI 매니저 컴포넌트 기본 설정을 초기화한다
	UPRUIControllerComponent();

	// 인벤토리 위젯을 열거나 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ToggleInventory();

	// 인벤토리 위젯이 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseInventory();

	// 현재 캐시된 인벤토리 위젯을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|UI")
	UPRInventoryWidget* GetInventoryWidget() const { return InventoryWidget; }

	// 현재 HUD 위젯 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|UI")
	UPRHUDWidget* GetHUDWidget() const { return HUDWidget; }

	// 장착 무기에 맞는 스코프 위젯을 표시한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ShowWeaponScope();

	// 현재 스코프 위젯을 숨긴다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void HideWeaponScope();

	// 새 폰 possession 시점에 폰 의존 위젯을 재초기화. 초기 possession과 리스폰 양쪽에서 호출 가능
	void RefreshForPawn(APawn* InPawn);
	
	UFUNCTION(BlueprintCallable)
	void RemoveAllWidget();

protected:
	/*~ UActorComponent Interface ~*/
	// 컴포넌트 종료 시 생성한 위젯 참조를 정리한다
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* WeaponManagerComponent, EPRWeaponSlotType ChangedSlot);

	// 소유 플레이어 컨트롤러가 로컬 컨트롤러인지 확인. PlayerController 유효성과 IsLocalController 검사를 묶음
	bool IsLocalPlayer() const;

	// 소유 플레이어 컨트롤러를 조회한다
	APlayerController* GetOwningPlayerController() const;

	// 플레이어 인벤토리 컴포넌트를 조회한다
	UPRInventoryComponent* GetInventoryComponent() const;

	// 현재 폰의 무기 매니저 컴포넌트를 조회한다
	UPRWeaponManagerComponent* GetWeaponManagerComponent() const;

	// 플레이어 퀵슬롯 컴포넌트를 조회한다
	UPRQuickSlotComponent* GetQuickSlotComponent() const;

	// 로컬 플레이어 UI 매니저 서브시스템을 조회한다
	UPRUIManagerSubsystem* GetUIManager() const;

	// 인벤토리 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRInventoryWidget* GetOrCreateInventoryWidget();

	// 현재 HUD 위젯을 UIManager에서 Pop하고 참조 정리
	void TearDownHUDWidget();

	// HUDWidgetClass로 HUD 위젯을 새로 생성하여 UIManager에 Push
	void CreateHUDWidget();

	// 현재 무기에 맞는 스코프 위젯을 준비한다.
	void RefreshWeaponScopeWidget();

	// 스코프 위젯을 제거한다.
	void RemoveWeaponScopeWidget();

	// 무기 장비 변경 델리게이트를 바인딩한다.
	void BindWeaponManager(UPRWeaponManagerComponent* WeaponManagerComponent);

	// 무기 장비 변경 델리게이트 바인딩을 해제한다.
	void UnbindWeaponManager();

protected:
	// ========= Configs ==========
	// 인벤토리 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Inventory")
	TSubclassOf<UPRInventoryWidget> InventoryWidgetClass;
	
	// HUD 위젯 클래스. BP에서 지정
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD")
	TSubclassOf<UPRHUDWidget> HUDWidgetClass;

private:
	// ======= Widget Instances =======
	// 생성 후 재사용할 인벤토리 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRInventoryWidget> InventoryWidget;
	
	// 현재 활성 HUD 위젯 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UPRHUDWidget> HUDWidget;

	// 장착 무기 데이터로 생성한 스코프 위젯
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> WeaponScopeWidget;
	
	// ====== Variables =======
	// 현재 바인딩된 무기 매니저
	UPROPERTY(Transient)
	TObjectPtr<UPRWeaponManagerComponent> BoundWeaponManager;
	
	// 현재 스코프 위젯 클래스
	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> CurrentScopeWidgetClass;
	
	bool bWantsWeaponScopeVisible = false;
};
