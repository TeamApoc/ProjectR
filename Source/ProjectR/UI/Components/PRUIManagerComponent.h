// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRUIManagerComponent.generated.h"

class APlayerController;
class UPRInventoryComponent;
class UPRInventoryWidget;
class UPRUIManagerSubsystem;
class UPRWeaponManagerComponent;

// 플레이어 컨트롤러에 부착되어 로컬 플레이어 UI 표시 흐름을 관리한다
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// UI 매니저 컴포넌트 기본 설정을 초기화한다
	UPRUIManagerComponent();

	// 인벤토리 위젯을 열거나 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void ToggleInventory();

	// 인벤토리 위젯이 열려 있으면 닫는다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|UI")
	void CloseInventory();

	// 현재 캐시된 인벤토리 위젯을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|UI")
	UPRInventoryWidget* GetInventoryWidget() const { return InventoryWidget; }

protected:
	/*~ UActorComponent Interface ~*/
	// 컴포넌트 종료 시 생성한 위젯 참조를 정리한다
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 소유 플레이어 컨트롤러를 조회한다
	APlayerController* GetOwningPlayerController() const;

	// 플레이어 인벤토리 컴포넌트를 조회한다
	UPRInventoryComponent* GetInventoryComponent() const;

	// 현재 폰의 무기 매니저 컴포넌트를 조회한다
	UPRWeaponManagerComponent* GetWeaponManagerComponent() const;

	// 로컬 플레이어 UI 매니저 서브시스템을 조회한다
	UPRUIManagerSubsystem* GetUIManager() const;

	// 인벤토리 위젯 인스턴스를 생성하거나 캐시된 인스턴스를 반환한다
	UPRInventoryWidget* GetOrCreateInventoryWidget();

private:
	// 인벤토리 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Inventory")
	TSubclassOf<UPRInventoryWidget> InventoryWidgetClass;

	// 생성 후 재사용할 인벤토리 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRInventoryWidget> InventoryWidget;
};
