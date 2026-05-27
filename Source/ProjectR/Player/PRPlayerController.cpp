// Copyright ProjectR. All Rights Reserved.

#include "PRPlayerController.h"

#include "Net/UnrealNetwork.h"
#include "ProjectR/Test/PRCheatHandler.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "ProjectR/Game/PRCameraManager.h"
#include "ProjectR/Input/PRInputConfigDataAsset.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Projectile/PRProjectileManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"
#include "ProjectR/Interaction/PRInteractionSensor.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Shop/Components/PRShopComponent.h"
#include "ProjectR/ItemSystem/Components/PRWeaponUpgradeComponent.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"


APRPlayerController::APRPlayerController()
{
	PlayerCameraManagerClass = APRCameraManager::StaticClass();

	// 등록 기반 SubObject 복제 시스템 사용. CheatHandler를 AddReplicatedSubObject로 등록 가능
	bReplicateUsingRegisteredSubObjectList = true;

	ProjectileManager = CreateDefaultSubobject<UPRProjectileManagerComponent>(TEXT("ProjectileManager"));
	FloatingTextManager = CreateDefaultSubobject<UPRFloatingTextManager>(TEXT("FloatingTextManager"));
	// 2026.05.01 이건주 | UI 컨트롤러 컴포넌트 추가 
	UIControllerComponent = CreateDefaultSubobject<UPRUIControllerComponent>(TEXT("UIControllerComponent"));
	InteractionSensor = CreateDefaultSubobject<UPRInteractionSensor>(TEXT("InteractionSensor"));
	InteractorComponent = CreateDefaultSubobject<UPRInteractorComponent>(TEXT("InteractorComponent"));
}

// =====  APlayerController Interface =====

void APRPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APRPlayerController, CheatHandler, COND_OwnerOnly);
}

void APRPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCompanionHighlight();
}

void APRPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	EnableCheats();

	// 서버 권위에서 CheatHandler 생성 후 ReplicatedSubObject로 등록. 본인 클라에 복제
	if (HasAuthority() && IsValid(CheatHandlerClass))
	{
		CheatHandler = NewObject<UPRCheatHandler>(this, CheatHandlerClass);
		AddReplicatedSubObject(CheatHandler);
	}
#endif

	// TODO: 로컬 클라만 서버로 캐릭터 페이로드 제출
	// 호스트의 경우 GameMode가 직접 LocalCharacter를 주입하므로 별도 경로로 처리
	// if (IsLocalController() && GetNetMode() == NM_Client)
	// {
	// 	SubmitLocalCharacterToServer();
	// }
}

void APRPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);

	// 새 폰 possession 시점에 폰 의존 UI를 재초기화. 초기 possession과 리스폰 양쪽에서 동작
	if (IsValid(UIControllerComponent))
	{
		UIControllerComponent->RefreshForPawn(InPawn);
	}
	
	// 게임 시작 or 맵 진입 후 FadeIn
	if (IsLocalController())
	{
		if (APRCameraManager* CM = Cast<APRCameraManager>(PlayerCameraManager))
		{
			CM->FadeIn(EPRFadeColorPreset::Black, FadeInDuration, false);
		}
	}
}

void APRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EIC))
	{
		return;
	}

	if (IsValid(InventoryAction))
	{
		EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &APRPlayerController::OnInventoryInputStarted);
	}
	
	if (IsValid(MouseSensitivityActionUp))
	{
		EIC->BindAction(MouseSensitivityActionUp, ETriggerEvent::Started, this, &APRPlayerController::OnMouseSensitivityActionUp);
	}
	
	if (IsValid(MouseSensitivityActionDown))
	{
		EIC->BindAction(MouseSensitivityActionDown, ETriggerEvent::Started, this, &APRPlayerController::OnMouseSensitivityActionDown);
	}
	
	if (IsValid(FlashlightAction))
	{
		EIC->BindAction(FlashlightAction, ETriggerEvent::Started, this, &APRPlayerController::ToggleFlashlight);
	}

	if (IsValid(TraitWindowAction.Get()))
	{
		EIC->BindAction(TraitWindowAction.Get(), ETriggerEvent::Started, this, &APRPlayerController::OnTraitWindowInputStarted);
	}

	
	for (int32 SlotIndex = 0; SlotIndex < QuickSlotActions.Num(); ++SlotIndex)
	{
		if (!IsValid(QuickSlotActions[SlotIndex]))
		{
			continue;
		}

		EIC->BindAction(QuickSlotActions[SlotIndex], ETriggerEvent::Started, this, &APRPlayerController::OnQuickSlotInputStarted, SlotIndex);
	}

	if (IsValid(InputConfig))
	{
		// IA별로 Started/Completed에 InputTag 포함 콜백을 바인딩
		for (const FPRInputActionBinding& Binding : InputConfig->AbilityInputBindings)
		{
			if (!IsValid(Binding.InputAction.Get()) || !Binding.InputTag.IsValid())
			{
				continue;
			}

			EIC->BindAction(Binding.InputAction.Get(), ETriggerEvent::Started, this,
				&APRPlayerController::OnAbilityInputPressed, Binding.InputTag);
			EIC->BindAction(Binding.InputAction.Get(), ETriggerEvent::Completed, this,
				&APRPlayerController::OnAbilityInputReleased, Binding.InputTag);
		}
	}
}

void APRPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UPRAbilitySystemComponent* ASC = GetPRASC())
	{
		ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void APRPlayerController::ClientStartMapTransition_Implementation(float Delay, EPRMapTransitionType TransitionType)
{
	if (IsValid(UIControllerComponent))
	{
		// UI 정리
		UIControllerComponent->RemoveAllWidget();
	}

	if (TransitionType == EPRMapTransitionType::None)
	{
		return;
	}

	APRCameraManager* CM = Cast<APRCameraManager>(PlayerCameraManager);
	if (!IsValid(CM))
	{
		return;
	}

	switch (TransitionType)
	{
	case EPRMapTransitionType::MapTravel:
		// 맵 이동 페이드
		CM->FadeOut(EPRFadeColorPreset::White, Delay, false);
		break;
	case EPRMapTransitionType::Respawn:
		// 리스폰 페이드
		CM->FadeOut(EPRFadeColorPreset::Black, Delay, false);
		break;
	default:
		break;
	}
}

void APRPlayerController::OnMouseSensitivityActionUp()
{
	APRPlayerState* PS = GetPlayerState<APRPlayerState>();
	
	if (IsValid(PS))
	{
		float NewSensitivity = PS->GetCameraSensitivity() + 0.05;
		PS->SetCameraSensitivity(NewSensitivity);
	}
}

void APRPlayerController::OnMouseSensitivityActionDown()
{
	APRPlayerState* PS = GetPlayerState<APRPlayerState>();
	
	if (IsValid(PS))
	{
		float NewSensitivity = PS->GetCameraSensitivity() - 0.05;
		PS->SetCameraSensitivity(NewSensitivity);
	}
}

void APRPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (UPRAbilitySystemComponent* ASC = GetPRASC())
	{
		ASC->AbilityInputPressed(InputTag);
	}
}

void APRPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (UPRAbilitySystemComponent* ASC = GetPRASC())
	{
		ASC->AbilityInputReleased(InputTag);
	}
}

void APRPlayerController::ToggleFlashlight(const FInputActionValue& Value)
{
	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}
	
	PlayerCharacter->SetFlashlightEnabled(!PlayerCharacter->IsFlashlightEnabled());
}

UPRAbilitySystemComponent* APRPlayerController::GetPRASC() const
{
	if (CachedASC.IsValid())
	{
		return CachedASC.Get();
	}
	
	if (APawn* LocalPawn = GetPawn())
	{
		if (UPRAbilitySystemComponent* ASC =  Cast<UPRAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(LocalPawn)))
		{
			CachedASC = ASC;
			return ASC;
		}
	}
	return nullptr;
}

void APRPlayerController::UpdateCompanionHighlight()
{
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	APRGameStateBase* GS = World->GetGameState<APRGameStateBase>();
	if (!IsValid(GS))
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	APawn* MyPawn = GetPawn();

	// 본인 제외 모든 플레이어 캐릭터 순회. 카메라 뷰포인트에서 캐릭터 위치까지 라인 트레이스로 차폐 여부 판정
	for (APRPlayerCharacter* OtherCharacter : GS->GetPlayerCharacters())
	{
		if (OtherCharacter == MyPawn)
		{
			continue;
		}
		
		UPRInteractableComponent* Interactable = OtherCharacter->GetInteractableComponent();
		if (InteractorComponent->GetFocusedComponent() == Interactable)
		{
			continue;
		}
		
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PRPlayerVisibility), false, this);
		Params.AddIgnoredActor(OtherCharacter);
		if (IsValid(MyPawn))
		{
			Params.AddIgnoredActor(MyPawn);
		}

		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(
			Hit, ViewLocation, OtherCharacter->GetActorLocation(), ECC_Visibility, Params);
		const bool bVisible = !bBlocked;
		
		if (Interactable->IsDepthStencilApplied())
		{
			// 보이는 경우 하이라이트 해제
			if (bVisible)
			{
				Interactable->ResetDepthStencilValues();
			}
		}
		// 벽에 가려진 경우 하이라이트 적용
		else if (!bVisible)
		{
			Interactable->ApplyDepthStencilValues(false);
		}
	}
}

void APRPlayerController::SubmitLocalCharacterToServer()
{
	if (bCharacterSubmitted)
	{
		return;
	}

	UPRGameInstance* GI = GetGameInstance<UPRGameInstance>();
	if (!IsValid(GI))
	{
		return;
	}

	bCharacterSubmitted = true;
	ServerSubmitCharacter(GI->GetLocalCharacter());
}

bool APRPlayerController::ServerSubmitCharacter_Validate(const FPRCharacterSaveData& Payload)
{
	// RPC 단계에서는 포맷만 확인. 상세 검증은 GameMode에서 수행
	return true;
}

void APRPlayerController::ServerSubmitCharacter_Implementation(const FPRCharacterSaveData& Payload)
{
	APRPlayGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<APRPlayGameMode>() : nullptr;
	if (!IsValid(GM))
	{
		ClientCharacterAccepted(false, TEXT("Server GameMode unavailable"));
		return;
	}

	const bool bAccepted = GM->AcceptGuestCharacter(this, Payload);
	if (!bAccepted)
	{
		ClientCharacterAccepted(false, TEXT("Payload rejected"));
	}
	else
	{
		ClientCharacterAccepted(true, FString());
	}
}

// =====  서버 -> 클라 통지 =====

void APRPlayerController::ClientCharacterAccepted_Implementation(bool bAccepted, const FString& Detail)
{
	if (!bAccepted)
	{
		// 거부 시 세션 퇴장
		if (UPRGameInstance* GI = GetGameInstance<UPRGameInstance>())
		{
			GI->LeaveSession();
		}
	}
}

void APRPlayerController::ClientGrantReward_Implementation(const FPRRewardGrant& Grant)
{
	UPRGameInstance* GI = GetGameInstance<UPRGameInstance>();
	if (!IsValid(GI))
	{
		return;
	}

	GI->ApplyRewardGrant(Grant);
}

void APRPlayerController::ClientDispatchSurvivalGameplayEvent_Implementation(FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Target = ControlledPawn;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		ControlledPawn,
		EventTag,
		Payload);
}

void APRPlayerController::ClientNotifyWeaponUpgradeResult_Implementation(const FPRWeaponUpgradeResult& Result)
{
	OnWeaponUpgradeResult.Broadcast(Result);
}

void APRPlayerController::ClientOpenWeaponUpgradeUI_Implementation(UPRWeaponUpgradeComponent* UpgradeComponent)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->OpenWeaponUpgrade(UpgradeComponent);
}

void APRPlayerController::ClientOpenShopUI_Implementation(UPRShopComponent* ShopComponent)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->OpenShop(ShopComponent);
}

void APRPlayerController::ClientNotifyShopBuyResult_Implementation(const FPRShopBuyResult& Result)
{
	OnShopBuyResult.Broadcast(Result);
}

void APRPlayerController::ClientNotifyShopSellResult_Implementation(const FPRShopSellResult& Result)
{
	OnShopSellResult.Broadcast(Result);
}

void APRPlayerController::ClientNotifyTraitInvestmentResult_Implementation(const FPRTraitInvestmentResult& Result)
{
	OnTraitInvestmentResult.Broadcast(Result);
}

void APRPlayerController::ClientNotifyPlayerLevelUp_Implementation(int32 PreviousLevel, int32 CurrentLevel)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->ShowLevelUpPopup(PreviousLevel, CurrentLevel);
}

void APRPlayerController::ClientNotifyPickupReward_Implementation(const FPRPickupNotificationPayload& Payload)
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->ShowPickupRewardNotification(Payload);
}

void APRPlayerController::RequestUpgradeWeapon(UPRWeaponUpgradeComponent* UpgradeComponent, UPRItemInstance_Weapon* WeaponItem)
{
	if (!IsValid(UpgradeComponent) || !IsValid(WeaponItem))
	{
		return;
	}

	ServerRequestUpgradeWeapon(UpgradeComponent, WeaponItem);
}

void APRPlayerController::RequestBuyShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity)
{
	if (!IsValid(ShopComponent) || EntryId.IsNone() || Quantity <= 0)
	{
		return;
	}

	ServerRequestBuyShopItem(ShopComponent, EntryId, Quantity);
}

void APRPlayerController::RequestSellShopItem(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity)
{
	if (!IsValid(ShopComponent) || EntryId.IsNone() || Quantity <= 0)
	{
		return;
	}

	ServerRequestSellShopItem(ShopComponent, EntryId, Quantity);
}

void APRPlayerController::RequestConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment)
{
	ServerRequestConfirmTraitInvestment(DesiredInvestment);
}

void APRPlayerController::RequestResetTraitInvestment()
{
	ServerRequestResetTraitInvestment();
}

void APRPlayerController::ServerRequestUpgradeWeapon_Implementation(UPRWeaponUpgradeComponent* UpgradeComponent, UPRItemInstance_Weapon* WeaponItem)
{
	if (!IsValid(UpgradeComponent) || !IsValid(UpgradeComponent->GetOwner()))
	{
		FPRWeaponUpgradeResult Result;
		Result.bSuccess = false;
		Result.FailReason = EPRWeaponUpgradeFailReason::InvalidUpgradeStation;
		Result.UpgradeComponent = UpgradeComponent;
		Result.WeaponItem = WeaponItem;
		Result.UpgradeLevel = IsValid(WeaponItem) ? WeaponItem->GetUpgradeLevel() : 0;
		ClientNotifyWeaponUpgradeResult(Result);
		return;
	}

	UpgradeComponent->RequestUpgradeWeapon(this, WeaponItem);
}

void APRPlayerController::ServerRequestBuyShopItem_Implementation(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity)
{
	if (!IsValid(ShopComponent) || !IsValid(ShopComponent->GetOwner()))
	{
		FPRShopBuyResult Result;
		Result.bSuccess = false;
		Result.FailReason = EPRShopBuyFailReason::InvalidShopData;
		Result.ShopComponent = ShopComponent;
		Result.EntryId = EntryId;
		Result.Quantity = Quantity;
		ClientNotifyShopBuyResult(Result);
		return;
	}

	ShopComponent->RequestBuyItem(this, EntryId, Quantity);
}

void APRPlayerController::ServerRequestSellShopItem_Implementation(UPRShopComponent* ShopComponent, FName EntryId, int32 Quantity)
{
	if (!IsValid(ShopComponent) || !IsValid(ShopComponent->GetOwner()))
	{
		FPRShopSellResult Result;
		Result.bSuccess = false;
		Result.FailReason = EPRShopSellFailReason::InvalidShopData;
		Result.ShopComponent = ShopComponent;
		Result.EntryId = EntryId;
		Result.Quantity = Quantity;
		ClientNotifyShopSellResult(Result);
		return;
	}

	ShopComponent->RequestSellItem(this, EntryId, Quantity);
}

void APRPlayerController::ServerRequestConfirmTraitInvestment_Implementation(const FPRTraitInvestmentInfo& DesiredInvestment)
{
	APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>();
	UPRPlayerGrowthComponent* GrowthComponent = IsValid(PRPlayerState) ? PRPlayerState->GetGrowthComponent() : nullptr;
	FPRTraitInvestmentResult Result;
	if (!IsValid(GrowthComponent))
	{
		Result.FailReason = EPRTraitInvestmentFailReason::InvalidGrowthComponent;
		ClientNotifyTraitInvestmentResult(Result);
		return;
	}

	Result = GrowthComponent->ConfirmTraitInvestment(DesiredInvestment);
	ClientNotifyTraitInvestmentResult(Result);
}

void APRPlayerController::ServerRequestResetTraitInvestment_Implementation()
{
	APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>();
	UPRPlayerGrowthComponent* GrowthComponent = IsValid(PRPlayerState) ? PRPlayerState->GetGrowthComponent() : nullptr;
	FPRTraitInvestmentResult Result;
	if (!IsValid(GrowthComponent))
	{
		Result.FailReason = EPRTraitInvestmentFailReason::InvalidGrowthComponent;
		ClientNotifyTraitInvestmentResult(Result);
		return;
	}

	Result = GrowthComponent->ResetTraitInvestment();
	ClientNotifyTraitInvestmentResult(Result);
}

// ===== UI =====
void APRPlayerController::OnInventoryInputStarted()
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}
    // 인벤토리 UI 토글 
	UIControllerComponent->ToggleInventory();
}

void APRPlayerController::OnTraitWindowInputStarted()
{
	if (!IsValid(UIControllerComponent))
	{
		return;
	}

	UIControllerComponent->ToggleTraitWindow();
}

void APRPlayerController::OnQuickSlotInputStarted(int32 SlotIndex)
{
	if (UPRQuickSlotComponent* QuickSlotComponent = GetQuickSlotComponent())
	{
		QuickSlotComponent->RequestUseQuickSlot(SlotIndex);
	}
}

UPRQuickSlotComponent* APRPlayerController::GetQuickSlotComponent() const
{
	const APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return nullptr;
	}

	return PRPlayerState->GetQuickSlotComponent();
}
