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
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true; // 컨트롤러 회전에 따라 카메라 회전

	// 카메라 설정
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
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
			ASC->GiveAbilitySet(AbilitySet,AbilitySetHandles);
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
		}
	}
}

void APRPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 상태 변수들을 모든 클라이언트에게 복제하도록 등록
	DOREPLIFETIME(APRPlayerCharacter, bIsSprinting);
	DOREPLIFETIME(APRPlayerCharacter, bIsAiming);
	DOREPLIFETIME(APRPlayerCharacter, bIsWalking);
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

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APRPlayerCharacter::SprintPressed);
		
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Started, this, &APRPlayerCharacter::WalkPressed);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APRPlayerCharacter::CrouchPressed);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &APRPlayerCharacter::AimStarted);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &APRPlayerCharacter::AimEnded);
	}
	
	// NOTE: PRPlayerController의 SetupInputComponent 에서 Ability Input을 바인딩함
}

void APRPlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

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

void APRPlayerCharacter::SprintPressed()
{
	// 멈춰있을때는 질주 불가
	if (!bIsSprinting && GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero())
	{
		return;
	}
	
	bIsSprinting = !bIsSprinting;
	UpdateMaxWalkSpeed();
	
	// 호스트가 아닐경우 서버에 보고
	if (!HasAuthority())
	{
		Server_SetSprinting(bIsSprinting);
	}
}

void APRPlayerCharacter::WalkPressed()
{
	bIsWalking = !bIsWalking;
	UpdateMaxWalkSpeed();
	
	if (!HasAuthority())
	{
		Server_SetWalking(bIsWalking);
	}
}

void APRPlayerCharacter::UpdateMaxWalkSpeed()
{
	// 이동 입력 종료시 sprint 자동 종료
	if (bIsSprinting && GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero() || bIsAiming)
	{
		bIsSprinting = false;
		if (GetLocalRole() < ROLE_Authority)
		{
			Server_SetSprinting(false);
		}
	}
	
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
	const bool bIsStrafeMode = bIsAiming || bIsWalking;
	
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 조깅/질주일 때는 이동 방향으로 캐릭터를 돌립니다.
		MoveComp->bOrientRotationToMovement = !bIsStrafeMode;
	}
	
	// 조준/걷기일 때는 캐릭터가 항상 카메라 정면을 바라보게 합니다.
	bUseControllerRotationYaw = bIsStrafeMode;
}

/*~ 상태 변경 및 동기화 ~*/


void APRPlayerCharacter::Server_SetSprinting_Implementation(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting;
	UpdateMaxWalkSpeed();
}

bool APRPlayerCharacter::Server_SetSprinting_Validate(bool bNewSprinting)
{
    // 비정상적인 상태 변경 검증 (스태미너 체크 로직 추가 ?)
    return true;
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

void APRPlayerCharacter::AimStarted()
{
	bIsAiming = true;
	UpdateMaxWalkSpeed();

	if (!HasAuthority())
	{
		Server_SetAiming(true);
	}
}

void APRPlayerCharacter::AimEnded()
{
	bIsAiming = false;
	UpdateMaxWalkSpeed();

	if (!HasAuthority())
	{
		Server_SetAiming(false);
	}
}

bool APRPlayerCharacter::Server_SetAiming_Validate(bool bNewAiming)
{
    return true;
}

void APRPlayerCharacter::Server_SetAiming_Implementation(bool bNewAiming)
{
	bIsAiming = bNewAiming;
	UpdateMaxWalkSpeed();
}


void APRPlayerCharacter::CrouchPressed()
{
	if (CanCrouch())
	{
		Crouch();
	}
	else if (bIsCrouched)
	{
		UnCrouch();
	}
}

void APRPlayerCharacter::HandleMovementInputTag(FGameplayTag InputTag, bool bPressed)
{
	if (InputTag == PRGameplayTags::Input_Locomotion_Sprint && bPressed)
	{
		bIsSprinting = !bIsSprinting;
	}
	else if (InputTag == PRGameplayTags::Input_Locomotion_Walk && bPressed)
	{
		bIsWalking = !bIsWalking;
	}
	
	UpdateMaxWalkSpeed();
}

void APRPlayerCharacter::OnRep_IsSprinting()
{
    // 타 플레이어의 화면에서도 애니메이션/속도 일치
    UpdateMaxWalkSpeed();
}

void APRPlayerCharacter::OnRep_IsAiming()
{
    UpdateMaxWalkSpeed();
}


float APRPlayerCharacter::GetDesiredLookDirection() const
{
	if (IsValid(Controller))
	{
		// 카메라(Control) 방향과 캐릭터 방향의 차이를 계산하여 Lean 애니메이션 등에 활용
		FRotator ControlRot = GetControlRotation();
		FRotator ActorRot = GetActorRotation();
		FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRot);
		return Delta.Yaw;
	}
	return 0.0f;
}