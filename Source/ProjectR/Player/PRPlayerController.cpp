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
#include "ProjectR/Inventory/Components/PRQuickSlotComponent.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"
#include "ProjectR/Interaction/PRInteractionSensor.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Game/PRGameStateBase.h"


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
}

void APRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EIC))
	{
		return;
	}

	if (IsValid(InventoryAction.Get()))
	{
		EIC->BindAction(InventoryAction.Get(), ETriggerEvent::Started, this, &APRPlayerController::OnInventoryInputStarted);
	}

	for (int32 SlotIndex = 0; SlotIndex < QuickSlotActions.Num(); ++SlotIndex)
	{
		if (!IsValid(QuickSlotActions[SlotIndex]))
		{
			continue;
		}

		EIC->BindAction(QuickSlotActions[SlotIndex], ETriggerEvent::Started, this, &APRPlayerController::OnQuickSlotInputStarted, SlotIndex);
	}
	
	if (IsValid(MouseSensitivityActionUp.Get()))
	{
		EIC->BindAction(MouseSensitivityActionUp.Get(), ETriggerEvent::Started, this, &APRPlayerController::OnMouseSensitivityActionUp);
	}
	
	if (IsValid(MouseSensitivityActionDown.Get()))
	{
		EIC->BindAction(MouseSensitivityActionDown.Get(), ETriggerEvent::Started, this, &APRPlayerController::OnMouseSensitivityActionDown);
	}

	if (!IsValid(InputConfig))
	{
		return;
	}

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

void APRPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UPRAbilitySystemComponent* ASC = GetASC())
	{
		ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

// =====  입력 콜백 =====

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
	if (UPRAbilitySystemComponent* ASC = GetASC())
	{
		ASC->AbilityInputPressed(InputTag);
	}
}

void APRPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (UPRAbilitySystemComponent* ASC = GetASC())
	{
		ASC->AbilityInputReleased(InputTag);
	}
}

UPRAbilitySystemComponent* APRPlayerController::GetASC() const
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

// =====  캐릭터 페이로드 제출 =====

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
