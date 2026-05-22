// Fill out your copyright notice in the Description page of Project Settings.


#include "PRPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Player/PRCameraModifier.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Player/Components/PRActionInputRouterComponent.h"
#include "ProjectR/Player/Components/PRFlashlightComponent.h"
#include "ProjectR/Player/Components/PRSpringArmComponent.h"
#include "ProjectR/Projectile/PRProjectileTrajectoryPreviewComponent.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"


// Sets default values
APRPlayerCharacter::APRPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
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

	ActionInputRouterComponent = CreateDefaultSubobject<UPRActionInputRouterComponent>(TEXT("ActionInputRouterComponent"));
	ProjectileTrajectoryPreviewComponent = CreateDefaultSubobject<UPRProjectileTrajectoryPreviewComponent>(TEXT("ProjectileTrajectoryPreviewComponent"));

	// 캡슐 기준 플래시라이트 부착
	FlashlightComponent = CreateDefaultSubobject<UPRFlashlightComponent>(TEXT("FlashlightComponent"));
	FlashlightComponent->SetupAttachment(GetCapsuleComponent());
	FlashlightComponent->SetRelativeLocation(FVector(50.0f, 0.0f, 55.0f));
	
	// 캡슐 설정
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	
	// 캐릭터 회전 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->bOrientRotationToMovement = true; // 이동 방향으로 캐릭터 회전 on off
	CMC->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	CMC->NavAgentProps.bCanCrouch = true; // 앉기 기능 활성화
	CMC->MaxWalkSpeed = JogSpeed; // 초기 속도 설정
	CMC->MaxFlySpeed = 1500.f;
	CMC->bAllowPhysicsRotationDuringAnimRootMotion = true; // 루트모션을 통한 회전 켜기
	// TODO: 아래 설정은 클라이언트의 movement를 서버가 신뢰하는 모델, 안정성 테스트 필요
	CMC->bIgnoreClientMovementErrorChecksAndCorrection = true;
	CMC->bServerAcceptClientAuthoritativePosition = true;
	
	InteractableComponent = CreateDefaultSubobject<UPRInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->bOnlyApplyDepthStencilOnAvailable = true;
	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(30.f);
	InteractionSphere->SetCollisionProfileName(TEXT("Interactable"));
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::Type::QueryOnly);
	InteractionSphere->SetupAttachment(RootComponent);
}

UPRAbilitySystemComponent* APRPlayerCharacter::GetPRAbilitySystemComponent() const
{
	// 플레이어 ASC는 PlayerState에 있음
	if (const APRPlayerState* PS = GetPlayerState<APRPlayerState>())
	{
		return PS->GetPRAbilitySystemComponent();
	}
	return nullptr;
}

UPRWeaponManagerComponent* APRPlayerCharacter::GetWeaponManager() const
{
	if (APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>())
	{
		return PRPlayerState->GetWeaponManagerComponent();
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
			PS->GrantCharacterAbilitySet(AbilitySet, this);
			
			// 이 시점에서 플레이어의 ASC 유효하므로 BindTagChangeEvent 호출
			BindTagChangeEvent();
		}
		
		if (PS->HasPendingSaveDataApply())
		{
			// 저장 데이터 1회 복원
			PS->ApplySaveData(PS->GetCurrentSaveData());
		}

		if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
		{
			WeaponManager->InitializeRuntimeLinks();
			WeaponManager->InitializeWithPawn(this);
		}
		
		PS->OnMouseSensitivityChanged.AddDynamic(this, &ThisClass::HandleMouseSensitivityChanged);
		CachedCameraSensitivity = PS->GetCameraSensitivity();
	}
	
	if (UPREventManagerSubsystem* EventManager = GetWorld()->GetSubsystem<UPREventManagerSubsystem>())
	{
		EventManager->BroadcastEmpty(PRGameplayTags::Player_Ready);
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

		if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
		{
			WeaponManager->InitializeRuntimeLinks();
			WeaponManager->InitializeWithPawn(this);
		}
	}
	
	if (UPREventManagerSubsystem* EventManager = GetWorld()->GetSubsystem<UPREventManagerSubsystem>())
	{
		EventManager->BroadcastEmpty(PRGameplayTags::Player_Ready);
	}
}

void APRPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool APRPlayerCharacter::IsAiming() const
{
	if (const UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent())
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Aiming);
	}
	return false;
}

bool APRPlayerCharacter::IsDown() const
{
	if (const UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent())
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Down);
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

	// 무기 미장착 상태에서 사용할 기본 애니메이션 레이어 연결. WeaponManager가 무기 장착 시 교체
	if (IsValid(DefaultAnimLayerClass) && IsValid(GetMesh()))
	{
		GetMesh()->LinkAnimClassLayers(DefaultAnimLayerClass);
	}

	if (IsValid(FlashlightComponent))
	{
		// 로컬 플레이어 전용 활성화
		FlashlightComponent->SetFlashlightEnabled(IsLocallyControlled());
		FlashlightComponent->SetRelativeLocation(IsCrouching() ? FlashlightCrouchingLocation : FlashlightStandingLocation);
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
	}
	
	// NOTE: PRPlayerController의 SetupInputComponent 에서 Ability Input을 바인딩함
}

void APRPlayerCharacter::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists)
{
	Super::HandleGameplayTagUpdated(ChangedTag, bTagExists);
	
	// if (ChangedTag.MatchesTag(PRGameplayTags::State))
	// {
	// 	if (GEngine)
	// 	{
	// 		GEngine->AddOnScreenDebugMessage(-1,1.0f,FColor::Yellow,FString::Printf(
	// 			TEXT("%s StateChanged: %s, %s"), *GetName(),*ChangedTag.ToString(),bTagExists?TEXT("True"):TEXT("False")));
	// 	}	
	// }
	
	if (ChangedTag.MatchesTag(PRGameplayTags::State_Crouching))
	{
		// 카메라 모디파이어 추가 (로컬 플레이어만 적용)
		if (IsLocallyControlled())
		{
			if (APlayerController* PC = GetController<APlayerController>())
			{
				if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
				{
					if (bTagExists)
					{
						// 모디파이어를 생성하고 매니저에 부착 (AlphaInTime에 맞춰 부드럽게 시야가 변함)
						CrouchCameraModifier = Cast<UPRCameraModifier>(
							CameraManager->AddNewCameraModifier(UPRCameraModifier::StaticClass())
						);

						if (CrouchCameraModifier)
						{
							CrouchCameraModifier->SetActionCameraSettings(60.0f, FVector(0.f, 0.f, 0.f));
						}
					}
					else
					{
						if (CrouchCameraModifier)
						{
							CrouchCameraModifier->DisableModifier(true); 
							CrouchCameraModifier = nullptr;
						}
					}
				}
			}
		}
	}
	
	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_Aiming))
	{
		bIsAiming = bTagExists;
		UpdateMaxWalkSpeed();

		if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
		{
			if (APRWeaponActor* Weapon = WeaponManager->GetActiveWeaponActor())
			{
				Weapon->SetIsIKSuppressed(false);
			}
		}

		// 조준 ON/OFF에 맞춰 투사체 예측 경로 표시 토글
		if (IsLocallyControlled())
		{
			if (IsValid(ProjectileTrajectoryPreviewComponent))
			{
				if (bTagExists)
				{
					ProjectileTrajectoryPreviewComponent->Show();
				}
				else
				{
					ProjectileTrajectoryPreviewComponent->Hide();
				}
			}	
		}
	}
	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_Sprinting))
	{
		SetSprintingFromAbility(bTagExists);
	}
	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_Block_Move))
	{
		bBlockMove = bTagExists;
	}
	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_Down))
	{
		if (bTagExists)
		{
			bIsSprinting = false;
			bIsWalking = false;
			bIsAiming = false;

			UCharacterMovementComponent* MoveComp = GetCharacterMovement();
			if (IsValid(MoveComp))
			{
				MoveComp->StopMovementImmediately();
			}
			
			if (GetNetOwner() != nullptr)
			{
				if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
				{
					WeaponManager->SetWeaponArmedState(EPRArmedState::Unarmed);	
				}
			}
		}
		
		UpdateMaxWalkSpeed();
	}
	
	// 소비템 사용중에 무기 숨기기
	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_UsingConsumable))
	{
		if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
		{
			if (APRWeaponActor* ActiveWeapon = WeaponManager->GetActiveWeaponActor())
			{
				ActiveWeapon->SetIsIKSuppressed(bTagExists);
				ActiveWeapon->SetActorHiddenInGame(bTagExists);
			}
		}
	}
	if (ChangedTag.MatchesTagExact(PRGameplayTags::State_Dodging))
	{
		bIsDodging = bTagExists;
	}
}

void APRPlayerCharacter::Crouch(bool bClientSimulation)
{
	Super::Crouch(bClientSimulation);
	
	if (FlashlightComponent)
	{
		FlashlightComponent->SetRelativeLocation(FlashlightCrouchingLocation);
	}
}

void APRPlayerCharacter::UnCrouch(bool bClientSimulation)
{
	Super::UnCrouch(bClientSimulation);
	
	if (FlashlightComponent)
	{
		FlashlightComponent->SetRelativeLocation(FlashlightStandingLocation);
	}
}

void APRPlayerCharacter::Move(const FInputActionValue& Value)
{
	// 사망 상태 등 움직임 비활성화시 움직임 수신 안함
	if (IsMovementBlocked())
	{
		return;
	}
	
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
		FRotator ForwardRotation = FRotator(0.0f, Rotation.Yaw, 0.0f);
		const FRotator YawRotation(0, Rotation.Yaw, 0);

#if !UE_BUILD_SHIPPING
		const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
		if (IsValid(MoveComp) && MoveComp->MovementMode == MOVE_Flying)
		{
			// 플라이 모드 카메라 Pitch 반영
			ForwardRotation = Rotation;
		}
#endif

		const FVector ForwardDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::X);
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
		AddControllerYawInput(LookAxisVector.X * CachedCameraSensitivity);
		AddControllerPitchInput(LookAxisVector.Y * CachedCameraSensitivity);
	}
}

void APRPlayerCharacter::UpdateMaxWalkSpeed()
{
	// 클라이언트 예측과 서버 보정 오차를 줄이기 위해 양측 동시 수행
	float BaseSpeed = JogSpeed;

	if (IsDown())
	{
		BaseSpeed = DownSpeed;
	}
	else if (bIsSprinting)
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
	
	// 최종 속도를 CharacterMovementComponent에 적용 
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (IsValid(MoveComp))
	{
		MoveComp->MaxWalkSpeed = BaseSpeed * CurrentMultiplier;
	}
	
	// 회전 방식 제어
	const bool bIsStrafeMode = bIsAiming;// bIsWalking;
	// 조준/걷기일 때는 캐릭터가 항상 카메라 정면을 바라보게 합니다.
	bUseControllerRotationYaw = bIsAiming;
	
	// Strafe 모드에서도 bUseControllerRotationYaw를 꺼야 제자리 회전이 동작합니다.
	if (IsValid(MoveComp))
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

void APRPlayerCharacter::HandleMouseSensitivityChanged(float NewSensitivity)
{
	CachedCameraSensitivity = NewSensitivity;
}

/*~ 상태 변경 및 동기화 ~*/

void APRPlayerCharacter::SetSprintingFromAbility(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting;
	UpdateMaxWalkSpeed();
}

void APRPlayerCharacter::SetFlashlightEnabled(bool bEnabled) const
{
	if (IsLocallyControlled())
	{
		FlashlightComponent->SetFlashlightEnabled(bEnabled);
	}
	else
	{
		FlashlightComponent->SetFlashlightEnabled(false);
	}
}

bool APRPlayerCharacter::IsFlashlightEnabled() const
{
	return FlashlightComponent->IsFlashlightEnabled();
}

void APRPlayerCharacter::MulticastSetMovementMode_Implementation(EMovementMode NewMovementMode)
{
	GetCharacterMovement()->SetMovementMode(NewMovementMode);
	UpdateMaxWalkSpeed();
}
