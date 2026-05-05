// Copyright ProjectR. All Rights Reserved.

#include "PRPlayerController.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "ProjectR/Game/PRCameraManager.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Input/PRInputConfigDataAsset.h"
#include "ProjectR/Projectile/PRProjectileManagerComponent.h"
#include "ProjectR/UI/Components/PRUIManagerComponent.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"


APRPlayerController::APRPlayerController()
{
	PlayerCameraManagerClass = APRCameraManager::StaticClass();
	
	ProjectileManager = CreateDefaultSubobject<UPRProjectileManagerComponent>(TEXT("ProjectileManager"));
	FloatingTextManager = CreateDefaultSubobject<UPRFloatingTextManager>(TEXT("FloatingTextManager"));
	// 2026.05.01 이건주 | UI 매니저 컴포넌트 추가 
	UIManagerComponent = CreateDefaultSubobject<UPRUIManagerComponent>(TEXT("UIManagerComponent"));
}

// =====  APlayerController Interface =====

void APRPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 클라만 서버로 캐릭터 페이로드 제출
	// 호스트의 경우 GameMode가 직접 LocalCharacter를 주입하므로 별도 경로로 처리
	if (IsLocalController() && GetNetMode() == NM_Client)
	{
		SubmitLocalCharacterToServer();
	}
}

void APRPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);
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

// ===== UI =====
void APRPlayerController::OnInventoryInputStarted()
{
	if (!IsValid(UIManagerComponent))
	{
		return;
	}
    // 인벤토리 UI 토글 
	UIManagerComponent->ToggleInventory();
}