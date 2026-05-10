// Fill out your copyright notice in the Description page of Project Settings.


#include "PRPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Player/Components/PRActionInputRouterComponent.h"
#include "ProjectR/Player/Components/PRSpringArmComponent.h"


// Sets default values
APRPlayerCharacter::APRPlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// 멀티플레이어 설정
	bReplicates = true;
	
	// 네트워크 동기화 빈도
	SetNetUpdateFrequency(100.0f);

	// 카메라 붐 설정 (캐릭터 뒤에 배치)
	CameraBoom = CreateDefaultSubobject<UPRSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 80.0f);
	CameraBoom->SocketOffset = FVector(0.0f, 70.0f, 0.0f);
	CameraBoom->bUsePawnControlRotation = true; // 컨트롤러 회전에 따라 카메라 회전
	CameraBoom->bDoCollisionTest = true; // 카메라 충돌 처리 (장애물 감지 시 당김)
	CameraBoom->bEnableCameraLag = true; // 카메라 지연(Lag) 활성화
	CameraBoom->CameraLagSpeed = 15.0f; // 지연 속도 조절

	// 카메라 설정
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, UPRSpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(80.0f); // 기본 FOV 80도 설정

	WeaponManagerComponent = CreateDefaultSubobject<UPRWeaponManagerComponent>(TEXT("WeaponManagerComponent"));
	ActionInputRouterComponent = CreateDefaultSubobject<UPRActionInputRouterComponent>(TEXT("ActionInputRouterComponent"));
	
	// 캡슐 설정
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	
	
	// 캐릭터 회전 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 캐릭터 회전 on off
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// 앉기 기능 활성화
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// 초기 속도 설정
	GetCharacterMovement()->MaxWalkSpeed = JogSpeed;
	
	// 루트모션을 통한 회전 켜기
	GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = true;
}

// =====  ASC 연동 =====

UPRAbilitySystemComponent* APRPlayerCharacter::GetPRAbilitySystemComponent() const
{
	// 플레이어 ASC는 PlayerState에 있음
	if (const APRPlayerState* PS = GetPlayerState<APRPlayerState>())
	{
		return PS->GetPRAbilitySystemComponent();
	}
	return nullptr;
}

void APRPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 서버 측 ActorInfo 초기화. Owner = PlayerState, Avatar = this
	if (APRPlayerState* PS = GetPlayerState<APRPlayerState>())
	{
		if (UPRAbilitySystemComponent* ASC = PS->GetPRAbilitySystemComponent())
		{
			ASC->InitAbilityActorInfo(PS, this);
			ASC->InitializeAttributesFromRegistry(
				UPRAssetManager::Get().GetAbilitySystemRegistry(),
				EPRCharacterRole::Player,
				PRRowNames::Player::Default);
			ASC->GiveAbilitySet(AbilitySet, AbilitySetHandles);
			
			// 이 시점에서 플레이어의 ASC 유효하므로 BindTagChangeEvent 호출
			BindTagChangeEvent();
		}

		if (IsValid(WeaponManagerComponent))
		{
			WeaponManagerComponent->InitializeRuntimeLinks();
		}
	}
}

void APRPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라이언트 측 ActorInfo 초기화
	if (APRPlayerState* PS = GetPlayerState<APRPlayerState>())
	{
		if (UPRAbilitySystemComponent* ASC = PS->GetPRAbilitySystemComponent())
		{
			ASC->InitAbilityActorInfo(PS, this);
			BindTagChangeEvent();
		}

		if (IsValid(WeaponManagerComponent))
		{
			WeaponManagerComponent->InitializeRuntimeLinks();
		}
	}
}

void APRPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 상태 변수들을 모든 클라이언트에게 복제하도록 등록
	DOREPLIFETIME(APRPlayerCharacter, bIsSprinting);
	//DOREPLIFETIME(APRPlayerCharacter, bIsAiming);
	DOREPLIFETIME(APRPlayerCharacter, bIsWalking);
}

bool APRPlayerCharacter::IsAiming() const
{
	if (const UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent())
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Aiming);
	}
	return false;
}

// Called when the game starts or when spawned
void APRPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Enhanced Input 매핑 컨텍스트 추가
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	if (IsValid(DefaultAnimLayerClass) && IsValid(GetMesh()))
	{
		GetMesh()->LinkAnimClassLayers(DefaultAnimLayerClass);
	}
}

// Called to bind functionality to input
void APRPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (IsValid(EnhancedInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APRPlayerCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APRPlayerCharacter::Look);
		
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Started, this, &APRPlayerCharacter::WalkPressed);
	}
	
	// NOTE: PRPlayerController의 SetupInputComponent 에서 Ability Input을 바인딩함
}

void APRPlayerCharacter::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists)
{
	Super::HandleGameplayTagUpdated(ChangedTag, bTagExists);
	
	// 26.04.26, Yuchan, Aiming 임시 테스트 코드
	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_Aiming))
	{
		bIsAiming = bTagExists;
		UpdateMaxWalkSpeed();
	}

	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_Sprinting))
	{
		SetSprintingFromAbility(bTagExists);
	}
}

void APRPlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (!MovementVector.IsNearlyZero()
		&& IsValid(ActionInputRouterComponent)
		&& ActionInputRouterComponent->IsRoutingInput()
		&& ActionInputRouterComponent->HandleRoutedInput())
	{
		return;
	}

	if (IsMoveInputLockedByState())
	{
		return;
	}

	if (IsValid(Controller))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APRPlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (IsValid(Controller))
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APRPlayerCharacter::WalkPressed()
{
	if (IsValid(ActionInputRouterComponent)
		&& ActionInputRouterComponent->IsRoutingInput()
		&& ActionInputRouterComponent->HandleRoutedInput())
	{
		return;
	}

	if (IsMoveInputLockedByState())
	{
		return;
	}

	bIsWalking = !bIsWalking;
	UpdateMaxWalkSpeed();
	
	if (!HasAuthority())
	{
		Server_SetWalking(bIsWalking);
	}
}

void APRPlayerCharacter::UpdateMaxWalkSpeed()
{
    // 클라이언트 예측과 서버 보정 오차를 줄이기 위해 양측 동시 수행
    float BaseSpeed = JogSpeed;

    if (bIsSprinting)
    {
        BaseSpeed = SprintSpeed;
    }
    else if (bIsAiming || bIsWalking)
    {
        BaseSpeed = WalkSpeed;
    }

	float CurrentMultiplier = 1.0f;

	if (const UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent())
	{
		CurrentMultiplier = ASC->GetNumericAttribute(UPRAttributeSet_Common::GetMovementSpeedMultiplierAttribute());
	}
	
	// 3. 최종 속도를 CharacterMovementComponent에 적용 
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = BaseSpeed * CurrentMultiplier;
	}
	
	// 회전 방식 제어
	const bool bIsStrafeMode = bIsAiming;// bIsWalking;
	// 조준/걷기일 때는 캐릭터가 항상 카메라 정면을 바라보게 합니다.
	bUseControllerRotationYaw = bIsAiming;
	
	// Strafe 모드에서도 bUseControllerRotationYaw를 꺼야 제자리 회전이 동작합니다.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = !bIsStrafeMode;
		// 26.04.27, Yuchan, 아래 코드가 플레이어 Strafe 모드를 해제하지 않도록 막아 주석처리
		// // 조준 중(Strafe)일 때 이동하면 카메라 방향으로 부드럽게 회전하도록 설정
		// bool bIsMoving = GetVelocity().Size2D() > 10.0f;
		// MoveComp->bOrientRotationToMovement = !bIsStrafeMode && bIsMoving;
		// MoveComp->bUseControllerDesiredRotation = bIsStrafeMode && bIsMoving;
		//
		MoveComp->bAllowPhysicsRotationDuringAnimRootMotion = true;
	}
}

bool APRPlayerCharacter::IsMoveInputLockedByState() const
{
	const UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent();
	return IsValid(ASC) && ASC->HasMatchingGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);
}

/*~ 상태 변경 및 동기화 ~*/

void APRPlayerCharacter::SetSprintingFromAbility(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting;
	UpdateMaxWalkSpeed();
}

void APRPlayerCharacter::Server_SetWalking_Implementation(bool bNewWalking)
{
	bIsWalking = bNewWalking;
	UpdateMaxWalkSpeed();
}

bool APRPlayerCharacter::Server_SetWalking_Validate(bool bNewWalking)
{
	return true;
}

void APRPlayerCharacter::OnRep_IsSprinting()
{
    // 타 플레이어의 화면에서도 애니메이션/속도 일치
    UpdateMaxWalkSpeed();
}
