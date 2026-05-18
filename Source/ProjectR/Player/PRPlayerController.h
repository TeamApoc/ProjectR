// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRPlayerController.generated.h"

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
	
	/*~ APlayerController Interface ~*/
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	// 서버 권위에서 동작하는 치트 핸들러 조회. 본인 클라에는 복제로 전달
	UPRCheatHandler* GetCheatHandler() const { return CheatHandler; }

	// 로컬 플레이어 UI 흐름을 담당하는 컴포넌트 조회
	UPRUIControllerComponent* GetUIController() const { return UIControllerComponent; }

	
	void UpdateCompanionHighlight();
public:
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

	// 서버 -> 본인 클라. 현재 Pawn에 생존 상태 전환 Gameplay Event를 발행한다.
	UFUNCTION(Client, Reliable)
	void ClientDispatchSurvivalGameplayEvent(FGameplayTag EventTag);

	UPRProjectileManagerComponent* GetProjectileManagerComponent() const {return ProjectileManager;}

	// 플로팅 텍스트 매니저 컴포넌트를 반환한다
	UPRFloatingTextManager* GetFloatingTextManager() const { return FloatingTextManager; }

protected:
	// 클라이언트 -> 서버. 로컬 캐릭터 페이로드 제출
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitCharacter(const FPRCharacterSaveData& Payload);

	// 치트 핸들러 클래스. BP에서 지정. 비어있으면 핸들러 미생성
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Cheat")
	TSubclassOf<UPRCheatHandler> CheatHandlerClass;

	// IA에 대한 InputTag 매핑을 담고 있는 InputConfig. 각 IA에 Pressed/Released 콜백을 라우팅
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<UPRInputConfigDataAsset> InputConfig;
	
	void OnMouseSensitivityActionUp();

	void OnMouseSensitivityActionDown();


	// IA Pressed 콜백. InputTag를 ASC로 전달
	void OnAbilityInputPressed(FGameplayTag InputTag);

	// IA Released 콜백
	void OnAbilityInputReleased(FGameplayTag InputTag);

	// 폰 -> PlayerState 경로로 ASC 조회
	UPRAbilitySystemComponent* GetASC() const;

    // ====== UI =====
	// 인벤토리 입력 시작을 처리
	void OnInventoryInputStarted();

	// 인벤토리 열기 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TObjectPtr<const UInputAction> InventoryAction;

	// 퀵슬롯 입력 액션 목록. 배열 인덱스가 퀵슬롯 인덱스와 일치한다
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input")
	TArray<UInputAction*> QuickSlotActions;

	// 마우스 감도 조절 액션 목록 
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input|MouseSensitivity")
	TObjectPtr<const UInputAction> MouseSensitivityActionUp;
	
	// 마우스 감도 조절 액션 목록 
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Input|MouseSensitivity")
	TObjectPtr<const UInputAction> MouseSensitivityActionDown;
	
	// 퀵슬롯 입력 시작을 처리
	void OnQuickSlotInputStarted(int32 SlotIndex);

	// 플레이어 퀵슬롯 컴포넌트를 조회
	UPRQuickSlotComponent* GetQuickSlotComponent() const;
	
private:
	// 캐릭터 페이로드를 이미 제출했는지 여부. 중복 제출 방지
	bool bCharacterSubmitted = false;
	
	mutable TWeakObjectPtr<UPRAbilitySystemComponent> CachedASC;

	// 서버 권위에서 생성되어 본인 클라에 복제되는 치트 명령 처리 객체
	UPROPERTY(Replicated)
	TObjectPtr<UPRCheatHandler> CheatHandler;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPRProjectileManagerComponent> ProjectileManager;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPRFloatingTextManager> FloatingTextManager;

    // ====== UI =====
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
