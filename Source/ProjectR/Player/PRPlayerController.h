// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/Player/Components/PRPlayerGrowthComponent.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponUpgradeTypes.h"
#include "PRPlayerController.generated.h"

enum class EPRFadeColorPreset : uint8;

UENUM(BlueprintType)
enum class EPRMapTransitionType : uint8
{
	None,
	MapTravel,
	Respawn,
};

class UPRInteractorComponent;
class UPRProjectileManagerComponent;
class UPRFloatingTextManager;
class UAbilitySystemComponent;
class UPRInputConfigDataAsset;
class UPRAbilitySystemComponent;
class UPRQuickSlotComponent;
struct FInputActionValue;
class UInputAction;
class UPRUIControllerComponent;
class UPRInteractionSensor;
class UPRCheatHandler;
class UPRItemInstance_Weapon;
class UPRShopComponent;
class UPRWeaponUpgradeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRWeaponUpgradeResultSignature, const FPRWeaponUpgradeResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRShopBuyResultSignature, const FPRShopBuyResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRShopSellResultSignature, const FPRShopSellResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRTraitInvestmentResultSignature, const FPRTraitInvestmentResult&, Result);

// 플레이어 입력·UI 소유. Join 시 캐릭터 페이로드를 서버로 전송하고,
// 인게임 중 발생한 보상 Grant를 연결이 살아있는 동안 즉시 수령하여 GameInstance에 반영한다
UCLASS()
class PROJECTR_API APRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APRPlayerController();
	
	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	
	/*~ APlayerController Interface ~*/
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	/*~ APRPlayerController Interface ~*/
	// 서버 권위에서 동작하는 치트 핸들러 조회. 본인 클라에는 복제로 전달
	UPRCheatHandler* GetCheatHandler() const { return CheatHandler; }

	// 로컬 플레이어 UI 흐름을 담당하는 컴포넌트 조회
	UPRUIControllerComponent* GetUIController() const { return UIControllerComponent; }
	
	UPRProjectileManagerComponent* GetProjectileManagerComponent() const {return ProjectileManager;}

	// 플로팅 텍스트 매니저 컴포넌트를 반환
	UPRFloatingTextManager* GetFloatingTextManager() const { return FloatingTextManager; }
	
	// 폰 -> PlayerState 경로로 ASC 조회
	UPRAbilitySystemComponent* GetPRASC() const;
	
	// 로컬 클라에서 호출. GameInstance의 LocalCharacter를 서버로 제출
	// 자동 호출(BeginPlay)과 수동 호출(재제출) 모두 허용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void SubmitLocalCharacterToServer();

	// 서버 -> 본인 클라. 캐릭터 페이로드 검증 결과. 거부 시 Detail에 사유
	UFUNCTION(Client, Reliable)
	void ClientCharacterAccepted(bool bAccepted, const FString& Detail);

	// 서버 -> 본인 클라. 보상 발생 이벤트 시점에 즉시 호출
	// 수신 즉시 GameInstance.ApplyRewardGrant로 반영. 세션 종료 정산 경로는 사용하지 않는다
	UFUNCTION(Client, Reliable)
	void ClientGrantReward(const FPRRewardGrant& Grant);

	// 서버 -> 본인 클라. 현재 Pawn에 생존 상태 전환 Gameplay Event를 발행
	UFUNCTION(Client, Reliable)
	void ClientDispatchSurvivalGameplayEvent(FGameplayTag EventTag);

	// 맵 이동 또는 리스폰 전환 연출 시작
	UFUNCTION(Client, Reliable)
	void ClientStartMapTransition(float Delay, EPRMapTransitionType TransitionType);
	
	// 서버 -> 본인 클라. 무기 강화 결과를 UI에 전달한다
	UFUNCTION(Client, Reliable)
	void ClientNotifyWeaponUpgradeResult(const FPRWeaponUpgradeResult& Result);

	// 서버 -> 본인 클라. 강화 UI를 열고 강화 컴포넌트 Context를 전달한다
	UFUNCTION(Client, Reliable)
	void ClientOpenWeaponUpgradeUI(UPRWeaponUpgradeComponent* UpgradeComponent);

	// 서버 -> 본인 클라. 상점 UI를 열고 상점 컴포넌트 Context를 전달한다
	UFUNCTION(Client, Reliable)
	void ClientOpenShopUI(UPRShopComponent* ShopComponent);

	// 서버 -> 본인 클라. 상점 구매 결과를 UI에 전달한다
	UFUNCTION(Client, Reliable)
	void ClientNotifyShopBuyResult(const FPRShopBuyResult& Result);

	// 서버 -> 본인 클라. 상점 판매 결과를 UI에 전달한다
	UFUNCTION(Client, Reliable)
	void ClientNotifyShopSellResult(const FPRShopSellResult& Result);

	// 서버 -> 본인 클라. 특성 투자 결과를 UI에 전달한다
	UFUNCTION(Client, Reliable)
	void ClientNotifyTraitInvestmentResult(const FPRTraitInvestmentResult& Result);

	// 강화 UI에서 선택한 무기 강화를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	void RequestUpgradeWeapon(UPRWeaponUpgradeComponent* UpgradeComponent, UPRItemInstance_Weapon* WeaponItem);

	// 상점 UI에서 선택한 아이템 구매를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void RequestBuyShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity);

	// 상점 UI에서 선택한 아이템 판매를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void RequestSellShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity);

	// 성장 UI에서 특성 투자 확정을 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RequestConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment);

	// 성장 UI에서 특성 투자 초기화를 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RequestResetTraitInvestment();
	
protected:
	// 클라이언트 -> 서버. 로컬 캐릭터 페이로드 제출
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitCharacter(const FPRCharacterSaveData& Payload);
	
	// 클라이언트 -> 서버. 강화 NPC 또는 터미널 컴포넌트에 무기 강화를 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestUpgradeWeapon(UPRWeaponUpgradeComponent* UpgradeComponent, UPRItemInstance_Weapon* WeaponItem);
	
	void OnMouseSensitivityActionUp();

	// 클라이언트 -> 서버. 상점 컴포넌트에 아이템 구매를 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestBuyShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity);

	// 클라이언트 -> 서버. 상점 컴포넌트에 아이템 판매를 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestSellShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity);

	// 클라이언트 -> 서버. 성장 컴포넌트에 특성 투자 확정을 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment);

	// 클라이언트 -> 서버. 성장 컴포넌트에 특성 투자 초기화를 위임한다
	UFUNCTION(Server, Reliable)
	void ServerRequestResetTraitInvestment();

	void OnMouseSensitivityActionDown();
	
	// IA Pressed 콜백. InputTag를 ASC로 전달
	void OnAbilityInputPressed(FGameplayTag InputTag);

	// IA Released 콜백
	void OnAbilityInputReleased(FGameplayTag InputTag);

	// 플래시라이트 토글
	void ToggleFlashlight(const FInputActionValue& Value);
	
    // ====== UI =====
	// 인벤토리 입력 시작을 처리
	void OnInventoryInputStarted();

	// 특성 투자창 입력 시작을 처리
	void OnTraitWindowInputStarted();
	
	// 퀵슬롯 입력 시작을 처리
	void OnQuickSlotInputStarted(int32 SlotIndex);

	// 플레이어 퀵슬롯 컴포넌트를 조회
	UPRQuickSlotComponent* GetQuickSlotComponent() const;
	
private:
	void UpdateCompanionHighlight();

public:
	// 무기 강화 결과를 UI에 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|WeaponUpgrade")
	FPRWeaponUpgradeResultSignature OnWeaponUpgradeResult;
	
	// 상점 구매 결과를 UI에 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Shop")
	FPRShopBuyResultSignature OnShopBuyResult;

	// 상점 판매 결과를 UI에 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Shop")
	FPRShopSellResultSignature OnShopSellResult;

	// 특성 투자 결과를 UI에 알린다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Growth")
	FPRTraitInvestmentResultSignature OnTraitInvestmentResult;
protected:
	// ====== Configs ======
	// 치트 핸들러 클래스. BP에서 지정. 비어있으면 핸들러 미생성
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Cheat")
	TSubclassOf<UPRCheatHandler> CheatHandlerClass;

	// IA에 대한 InputTag 매핑을 담고 있는 InputConfig. 각 IA에 Pressed/Released 콜백을 라우팅
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<UPRInputConfigDataAsset> InputConfig;
	
	// 플래시 라이트 On/Off 액션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<UInputAction> FlashlightAction;
	
	// 인벤토리 열기 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<const UInputAction> InventoryAction;

	// 특성 투자창 열기 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<const UInputAction> TraitWindowAction;

	// 퀵슬롯 입력 액션 목록. 배열 인덱스가 퀵슬롯 인덱스와 일치한다
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TArray<UInputAction*> QuickSlotActions;
	
	// 마우스 감도 조절 액션 목록 
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input|MouseSensitivity")
	TObjectPtr<const UInputAction> MouseSensitivityActionUp;
	
	// 마우스 감도 조절 액션 목록 
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input|MouseSensitivity")
	TObjectPtr<const UInputAction> MouseSensitivityActionDown;
	
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Effect")
	float FadeInDuration = 1.0f;

private:
	// 캐릭터 페이로드를 이미 제출했는지 여부. 중복 제출 방지
	bool bCharacterSubmitted = false;
	
	mutable TWeakObjectPtr<UPRAbilitySystemComponent> CachedASC;

	// 서버 권위에서 생성되어 본인 클라에 복제되는 치트 명령 처리 객체
	UPROPERTY(Replicated)
	TObjectPtr<UPRCheatHandler> CheatHandler;
	
	// ====== Components =====
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPRProjectileManagerComponent> ProjectileManager;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPRFloatingTextManager> FloatingTextManager;

	// 로컬 플레이어 UI 표시 흐름을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|UI")
	TObjectPtr<UPRUIControllerComponent> UIControllerComponent;

	// 주변 Interactable 감지, 포커스 후보 선정을 담당하는 컴포넌트 (로컬 컨트롤러 전용 동작)
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Interaction")
	TObjectPtr<UPRInteractionSensor> InteractionSensor;
	
	// 상호작용 매니저 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Interaction")
	TObjectPtr<UPRInteractorComponent> InteractorComponent;
};
