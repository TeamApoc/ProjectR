// Fill out your copyright notice in the Description page of Project Settings.


#include "PRPlayerCharacter.h"

#include "AbilitySystemComponent.h"
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
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "ProjectR/Player/PRCameraModifier.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Player/Components/PRActionInputRouterComponent.h"
#include "ProjectR/Player/Components/PRFlashlightComponent.h"
#include "ProjectR/Player/Components/PRSpringArmComponent.h"
#include "ProjectR/Projectile/PRProjectileTrajectoryPreviewComponent.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "TimerManager.h"


// Sets default values
APRPlayerCharacter::APRPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 멀티플레이어 설정
	bReplicates = true;

	// 네트워크 동기화 빈도
	SetNetUpdateFrequency(100.0f);


	// 캐릭터 파츠 설정
	USkeletalMeshComponent* LeaderMesh = GetMesh();
	LeaderMesh->SetVisibility(false);
	LeaderMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	Mesh_Head = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Head"));
	Mesh_Head->SetupAttachment(LeaderMesh);
	Mesh_Head->SetLeaderPoseComponent(LeaderMesh);
	Mesh_Head->bUseAttachParentBound = true;
	Mesh_Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Body"));
	Mesh_Body->SetupAttachment(LeaderMesh);
	Mesh_Body->SetLeaderPoseComponent(LeaderMesh);
	Mesh_Body->bUseAttachParentBound = true;
	Mesh_Hands = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Hands"));
	Mesh_Hands->SetupAttachment(LeaderMesh);
	Mesh_Hands->SetLeaderPoseComponent(LeaderMesh);
	Mesh_Hands->bUseAttachParentBound = true;
	Mesh_Legs = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Legs"));
	Mesh_Legs->SetupAttachment(LeaderMesh);
	Mesh_Legs->SetLeaderPoseComponent(LeaderMesh);
	Mesh_Legs->bUseAttachParentBound = true;

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

UPREquipmentManagerComponent* APRPlayerCharacter::GetEquipmentManager() const
{
	if (APRPlayerState* PRPlayerState = GetPlayerState<APRPlayerState>())
	{
		return PRPlayerState->GetEquipmentManagerComponent();
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
			BindMovementSpeedAttributeChange();
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

		// PlayerState 장비 복원 이후 현재 장착 외형을 캐릭터 파츠에 반영
		CacheDefaultEquipmentMeshes();
		BindEquipmentManager();
		ApplyEquipmentVisualsFromManager();
		
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
			BindMovementSpeedAttributeChange();
		}

		if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
		{
			WeaponManager->InitializeRuntimeLinks();
			WeaponManager->InitializeWithPawn(this);
		}

		// 클라이언트는 PlayerState 복제 이후 장비 외형 복제 알림을 받아 파츠 메시를 갱신
		CacheDefaultEquipmentMeshes();
		BindEquipmentManager();
		ApplyEquipmentVisualsFromManager();
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

void APRPlayerCharacter::ClientStartExternalForcedMove_Implementation(
	FVector_NetQuantize Destination,
	FRotator Rotation,
	float Duration,
	float TickInterval,
	float EaseExponent,
	bool bSweep,
	bool bStopMovement)
{
	StartExternalForcedMoveLocal(Destination, Rotation, Duration, TickInterval, EaseExponent, bSweep, bStopMovement);
}

float APRPlayerCharacter::GetMovementSpeedMultiplier() const
{
	if (const UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(UPRAttributeSet_Common::GetMovementSpeedMultiplierAttribute());
	}
	return 1.0f;
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

	// BP에서 파츠 컴포넌트에 직접 넣은 메시를 기본 장비 메시로 사용
	CacheDefaultEquipmentMeshes();
}

void APRPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CompleteExternalForcedMove(true);
	UnbindEquipmentManager();
	UnbindMovementSpeedAttributeChange();

	Super::EndPlay(EndPlayReason);
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

	const float CurrentMultiplier = GetMovementSpeedMultiplier();
	
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

void APRPlayerCharacter::StartExternalForcedMoveLocal(
	const FVector& Destination,
	const FRotator& Rotation,
	float Duration,
	float TickInterval,
	float EaseExponent,
	bool bSweep,
	bool bStopMovement)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(ExternalForcedMoveTimerHandle);

	ExternalForcedMoveStartLocation = GetActorLocation();
	ExternalForcedMoveEndLocation = Destination;
	ExternalForcedMoveStartRotation = GetActorRotation();
	ExternalForcedMoveEndRotation = Rotation;
	ExternalForcedMoveDuration = FMath::Max(Duration, 0.0f);
	ExternalForcedMoveElapsedSeconds = 0.0f;
	ExternalForcedMoveLastUpdateTime = World->GetTimeSeconds();
	ExternalForcedMoveTickInterval = FMath::Max(TickInterval, 0.005f);
	ExternalForcedMoveEaseExponent = FMath::Max(EaseExponent, 0.1f);
	bExternalForcedMoveSweep = bSweep;
	bExternalForcedMoveStopMovement = bStopMovement;
	bExternalForcedMoveActive = true;

	if (bExternalForcedMoveStopMovement)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
	}

	if (ExternalForcedMoveDuration <= UE_SMALL_NUMBER)
	{
		CompleteExternalForcedMove(false);
		return;
	}

	World->GetTimerManager().SetTimer(
		ExternalForcedMoveTimerHandle,
		this,
		&APRPlayerCharacter::TickExternalForcedMove,
		ExternalForcedMoveTickInterval,
		true);
	TickExternalForcedMove();
}

void APRPlayerCharacter::TickExternalForcedMove()
{
	UWorld* World = GetWorld();
	if (!bExternalForcedMoveActive || !IsValid(World))
	{
		CompleteExternalForcedMove(true);
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float DeltaTime = ExternalForcedMoveLastUpdateTime > 0.0f
		? CurrentTime - ExternalForcedMoveLastUpdateTime
		: ExternalForcedMoveTickInterval;
	ExternalForcedMoveLastUpdateTime = CurrentTime;
	ExternalForcedMoveElapsedSeconds += FMath::Max(DeltaTime, 0.0f);

	const float Alpha = ExternalForcedMoveDuration > 0.0f
		? FMath::Clamp(ExternalForcedMoveElapsedSeconds / ExternalForcedMoveDuration, 0.0f, 1.0f)
		: 1.0f;
	const float EasedAlpha = FMath::InterpEaseInOut(
		0.0f,
		1.0f,
		Alpha,
		ExternalForcedMoveEaseExponent);
	const FVector NewLocation = FMath::Lerp(ExternalForcedMoveStartLocation, ExternalForcedMoveEndLocation, EasedAlpha);
	const FQuat NewRotation = FQuat::Slerp(
		ExternalForcedMoveStartRotation.Quaternion(),
		ExternalForcedMoveEndRotation.Quaternion(),
		EasedAlpha);

	FHitResult SweepHit;
	SetActorLocationAndRotation(
		NewLocation,
		NewRotation.Rotator(),
		bExternalForcedMoveSweep,
		bExternalForcedMoveSweep ? &SweepHit : nullptr,
		ETeleportType::TeleportPhysics);

	if (bExternalForcedMoveStopMovement)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
	}

	if (Alpha >= 1.0f)
	{
		CompleteExternalForcedMove(false);
	}
}

void APRPlayerCharacter::CompleteExternalForcedMove(bool bWasCancelled)
{
	if (!bExternalForcedMoveActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExternalForcedMoveTimerHandle);
	}

	if (!bWasCancelled)
	{
		FHitResult SweepHit;
		SetActorLocationAndRotation(
			ExternalForcedMoveEndLocation,
			ExternalForcedMoveEndRotation,
			bExternalForcedMoveSweep,
			bExternalForcedMoveSweep ? &SweepHit : nullptr,
			ETeleportType::TeleportPhysics);
	}

	if (bExternalForcedMoveStopMovement)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
	}

	bExternalForcedMoveActive = false;
	ExternalForcedMoveElapsedSeconds = 0.0f;
	ExternalForcedMoveLastUpdateTime = 0.0f;
}

void APRPlayerCharacter::BindMovementSpeedAttributeChange()
{
	UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Common::GetMovementSpeedMultiplierAttribute()).RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Common::GetMovementSpeedMultiplierAttribute()).AddUObject(
		this,
		&ThisClass::HandleMovementSpeedMultiplierChanged);
	UpdateMaxWalkSpeed();
}

void APRPlayerCharacter::UnbindMovementSpeedAttributeChange()
{
	UPRAbilitySystemComponent* ASC = GetPRAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Common::GetMovementSpeedMultiplierAttribute()).RemoveAll(this);
}

void APRPlayerCharacter::HandleMovementSpeedMultiplierChanged(const FOnAttributeChangeData& ChangeData)
{
	UpdateMaxWalkSpeed();
}

void APRPlayerCharacter::BindEquipmentManager()
{
	UnbindEquipmentManager();

	UPREquipmentManagerComponent* EquipmentManager = GetEquipmentManager();
	if (!IsValid(EquipmentManager))
	{
		return;
	}

	// EquipmentManager는 Owner 전용 ItemInstance와 전체 공개 외형 정보를 분리해 복제
	// 캐릭터 파츠 교체는 모든 클라이언트가 볼 수 있는 외형 정보 변경 이벤트만 사용
	BoundEquipmentManager = EquipmentManager;
	BoundEquipmentManager->OnEquipmentVisualInfosChanged.RemoveDynamic(this, &ThisClass::HandleEquipmentVisualInfosChanged);
	BoundEquipmentManager->OnEquipmentVisualInfosChanged.AddDynamic(this, &ThisClass::HandleEquipmentVisualInfosChanged);
}

void APRPlayerCharacter::UnbindEquipmentManager()
{
	if (IsValid(BoundEquipmentManager))
	{
		BoundEquipmentManager->OnEquipmentVisualInfosChanged.RemoveDynamic(this, &ThisClass::HandleEquipmentVisualInfosChanged);
	}

	BoundEquipmentManager = nullptr;
}

void APRPlayerCharacter::ApplyEquipmentVisualsFromManager()
{
	// 장착 정보가 아직 없거나 PlayerState 복제 전이면 모든 파츠를 기본 메시로 복원
	const UPREquipmentManagerComponent* EquipmentManager = GetEquipmentManager();
	if (!IsValid(EquipmentManager))
	{
		ApplyEquipmentVisual(EPREquipmentSlotType::Head, nullptr);
		ApplyEquipmentVisual(EPREquipmentSlotType::Body, nullptr);
		ApplyEquipmentVisual(EPREquipmentSlotType::Hands, nullptr);
		ApplyEquipmentVisual(EPREquipmentSlotType::Legs, nullptr);
		return;
	}

	TSet<EPREquipmentSlotType> AppliedSlots;
	for (const FPRReplicatedEquipmentInfo& EquipmentInfo : EquipmentManager->GetEquippedVisualInfos())
	{
		if (EquipmentInfo.SlotType == EPREquipmentSlotType::None)
		{
			continue;
		}

		const UPREquipmentDataAsset* EquipmentData = EquipmentInfo.EquipmentData.LoadSynchronous();
		ApplyEquipmentVisual(EquipmentInfo.SlotType, EquipmentData);
		AppliedSlots.Add(EquipmentInfo.SlotType);
	}

	// 복제 목록에 없는 방어구 파츠는 해제된 상태로 간주하고 기본 메시 복원
	const EPREquipmentSlotType ArmorSlots[] =
	{
		EPREquipmentSlotType::Head,
		EPREquipmentSlotType::Body,
		EPREquipmentSlotType::Hands,
		EPREquipmentSlotType::Legs
	};

	for (EPREquipmentSlotType SlotType : ArmorSlots)
	{
		if (!AppliedSlots.Contains(SlotType))
		{
			ApplyEquipmentVisual(SlotType, nullptr);
		}
	}
}

void APRPlayerCharacter::CacheDefaultEquipmentMeshes()
{
	// 기본 메시 프로퍼티가 비어 있으면 현재 BP 파츠 메시를 해제 복원값으로 사용
	if (!IsValid(DefaultHeadMesh.Get()) && IsValid(Mesh_Head))
	{
		DefaultHeadMesh = Mesh_Head->GetSkeletalMeshAsset();
	}

	if (!IsValid(DefaultBodyMesh.Get()) && IsValid(Mesh_Body))
	{
		DefaultBodyMesh = Mesh_Body->GetSkeletalMeshAsset();
	}

	if (!IsValid(DefaultHandsMesh.Get()) && IsValid(Mesh_Hands))
	{
		DefaultHandsMesh = Mesh_Hands->GetSkeletalMeshAsset();
	}

	if (!IsValid(DefaultLegsMesh.Get()) && IsValid(Mesh_Legs))
	{
		DefaultLegsMesh = Mesh_Legs->GetSkeletalMeshAsset();
	}
}

void APRPlayerCharacter::ApplyEquipmentVisual(EPREquipmentSlotType SlotType, const UPREquipmentDataAsset* EquipmentData)
{
	USkeletalMeshComponent* PartMeshComponent = GetEquipmentMeshComponent(SlotType);
	if (!IsValid(PartMeshComponent))
	{
		return;
	}

	// 장비 데이터가 없거나 메시가 비어 있으면 해당 파츠의 기본 메시를 사용
	USkeletalMesh* MeshToApply = GetDefaultEquipmentMesh(SlotType);
	if (IsValid(EquipmentData))
	{
		if (USkeletalMesh* EquipmentMesh = EquipmentData->GetEquipmentMesh().LoadSynchronous())
		{
			MeshToApply = EquipmentMesh;
		}
	}

	if (!IsValid(MeshToApply))
	{
		return;
	}

	PartMeshComponent->SetSkeletalMesh(MeshToApply);
}

USkeletalMeshComponent* APRPlayerCharacter::GetEquipmentMeshComponent(EPREquipmentSlotType SlotType) const
{
	switch (SlotType)
	{
	case EPREquipmentSlotType::Head:
		return Mesh_Head;

	case EPREquipmentSlotType::Body:
		return Mesh_Body;

	case EPREquipmentSlotType::Hands:
		return Mesh_Hands;

	case EPREquipmentSlotType::Legs:
		return Mesh_Legs;

	case EPREquipmentSlotType::Amulet:
	case EPREquipmentSlotType::Ring1:
	case EPREquipmentSlotType::Ring2:
	case EPREquipmentSlotType::None:
	default:
		return nullptr;
	}
}

USkeletalMesh* APRPlayerCharacter::GetDefaultEquipmentMesh(EPREquipmentSlotType SlotType) const
{
	switch (SlotType)
	{
	case EPREquipmentSlotType::Head:
		return DefaultHeadMesh.Get();

	case EPREquipmentSlotType::Body:
		return DefaultBodyMesh.Get();

	case EPREquipmentSlotType::Hands:
		return DefaultHandsMesh.Get();

	case EPREquipmentSlotType::Legs:
		return DefaultLegsMesh.Get();

	case EPREquipmentSlotType::Amulet:
	case EPREquipmentSlotType::Ring1:
	case EPREquipmentSlotType::Ring2:
	case EPREquipmentSlotType::None:
	default:
		return nullptr;
	}
}

void APRPlayerCharacter::HandleEquipmentVisualInfosChanged(UPREquipmentManagerComponent* ChangedEquipmentManagerComponent)
{
	if (ChangedEquipmentManagerComponent != GetEquipmentManager())
	{
		return;
	}

	ApplyEquipmentVisualsFromManager();
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
