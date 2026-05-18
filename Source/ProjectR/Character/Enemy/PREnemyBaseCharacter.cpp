// Copyright ProjectR. All Rights Reserved.

#include "PREnemyBaseCharacter.h"

#include "ProjectR/AI/Controllers/PREnemyAIController.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/AI/Components/PREnemyCombatEventRelayComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/AI/Data/PRPerceptionConfig.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "Engine/DataTable.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"
#include "ProjectR/UI/HUD/PREnemyWorldHealthBarComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
	AActor* ResolveEnemyThreatSourceActor(const FPRDamageAppliedContext& Context)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Context.InstigatorController.Get()))
		{
			return PlayerController->GetPawn();
		}

		AActor* InstigatorActor = Context.Instigator.Get();
		if (AController* InstigatorController = Cast<AController>(InstigatorActor))
		{
			return InstigatorController->GetPawn();
		}

		if (APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
		{
			return InstigatorPawn;
		}

		return InstigatorActor;
	}
}

APREnemyBaseCharacter::APREnemyBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(60.0f);

	AIControllerClass = APREnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

	AbilitySystemComponent = CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// 적 ASC는 서버 판단이 중심이므로 Minimal 복제로 태그/필수 상태만 공유한다.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	CommonSet = CreateDefaultSubobject<UPRAttributeSet_Common>(TEXT("CommonSet"));
	EnemySet = CreateDefaultSubobject<UPRAttributeSet_Enemy>(TEXT("EnemySet"));
	ThreatComponent = CreateDefaultSubobject<UPREnemyThreatComponent>(TEXT("ThreatComponent"));
	CombatEventRelayComponent = CreateDefaultSubobject<UPREnemyCombatEventRelayComponent>(TEXT("CombatEventRelayComponent"));
	EnemyWorldHealthBarComponent = CreateDefaultSubobject<UPREnemyWorldHealthBarComponent>(TEXT("EnemyWorldHealthBarComponent"));
	EnemyWorldHealthBarComponent->SetupAttachment(GetRootComponent());

}

void APREnemyBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	BindTagChangeEvent();
	
	InitializeHomeLocation();
	InitializeEnemyWorldHealthBar();
}

void APREnemyBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APREnemyBaseCharacter, bMaintainCombatMoveFocus);
	DOREPLIFETIME(APREnemyBaseCharacter, bUseCombatMovePose);
	DOREPLIFETIME(APREnemyBaseCharacter, bUseCombatAimOffset);
	DOREPLIFETIME(APREnemyBaseCharacter, bUseCombatStrafeState);
}

void APREnemyBaseCharacter::PossessedBy(AController* NewController)
{
	InitializeHomeLocation();
	Super::PossessedBy(NewController);

	InitializeEnemyAbilitySystem();
}

UPRAbilitySystemComponent* APREnemyBaseCharacter::GetPRAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UPRAbilitySystemComponent* APREnemyBaseCharacter::GetEnemyAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UPREnemyThreatComponent* APREnemyBaseCharacter::GetEnemyThreatComponent() const
{
	return ThreatComponent;
}

UPRPatternDataAsset* APREnemyBaseCharacter::GetPatternDataAsset() const
{
	return PatternDataAsset;
}

UPRCombatMoveDataAsset* APREnemyBaseCharacter::GetCombatDataAsset() const
{
	return CombatDataAsset;
}

UPRPerceptionConfig* APREnemyBaseCharacter::GetPerceptionConfig() const
{
	return PerceptionConfig;
}

UBehaviorTree* APREnemyBaseCharacter::GetBehaviorTreeAsset() const
{
	return BehaviorTreeAsset;
}

FVector APREnemyBaseCharacter::GetHomeLocation() const
{
	return HomeLocation;
}

void APREnemyBaseCharacter::ApplyCombatMovePresentationContext(const FPREnemyMovePresentationConfig& PresentationConfig)
{
	CacheMovementPresentationDefaults();
	ApplyMovementPresentationConfig(PresentationConfig);

	bMaintainCombatMoveFocus = PresentationConfig.bMaintainTargetFocus;
	bUseCombatMovePose = PresentationConfig.bUseCombatMovePose;
	bUseCombatAimOffset = PresentationConfig.bUseCombatAimOffset;
	bUseCombatStrafeState = PresentationConfig.bUseCombatStrafeState;
}

void APREnemyBaseCharacter::ClearCombatMovePresentationContext()
{
	RestoreMovementPresentationDefaults();
	bMaintainCombatMoveFocus = false;
	bUseCombatMovePose = false;
	bUseCombatAimOffset = false;
	bUseCombatStrafeState = false;
}

void APREnemyBaseCharacter::RequestDeathDissolveVisual(
	UAnimMontage* InDeathMontage,
	float InMontagePlayRate,
	float InDissolveDelay,
	float InDissolveDuration,
	float InDissolveStartValue,
	float InDissolveEndValue,
	FName InDissolveScalarParameterName,
	UNiagaraSystem* InDissolveNiagaraSystem,
	FName InNiagaraDissolveParameterName,
	UTexture* InDissolveTexture,
	FVector2D InDissolveTextureUV,
	float InDissolveTickInterval)
{
	if (!HasAuthority())
	{
		return;
	}

	Multicast_RequestDeathDissolveVisual(
		InDeathMontage,
		InMontagePlayRate,
		InDissolveDelay,
		InDissolveDuration,
		InDissolveStartValue,
		InDissolveEndValue,
		InDissolveScalarParameterName,
		InDissolveNiagaraSystem,
		InNiagaraDissolveParameterName,
		InDissolveTexture,
		InDissolveTextureUV,
		InDissolveTickInterval);
}

void APREnemyBaseCharacter::Multicast_RequestDeathDissolveVisual_Implementation(
	UAnimMontage* InDeathMontage,
	float InMontagePlayRate,
	float InDissolveDelay,
	float InDissolveDuration,
	float InDissolveStartValue,
	float InDissolveEndValue,
	FName InDissolveScalarParameterName,
	UNiagaraSystem* InDissolveNiagaraSystem,
	FName InNiagaraDissolveParameterName,
	UTexture* InDissolveTexture,
	FVector2D InDissolveTextureUV,
	float InDissolveTickInterval)
{
	// 서버는 Death Ability가 Actor 제거까지 소유하므로, 멀티캐스트는 클라이언트 시각 연출만 담당한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveStartTimerHandle);
		World->GetTimerManager().ClearTimer(DeathDissolveTickTimerHandle);
	}

	DeathDissolveDynamicMaterials.Reset();
	if (IsValid(ActiveDeathDissolveNiagara))
	{
		ActiveDeathDissolveNiagara->Deactivate();
	}
	ActiveDeathDissolveNiagara = nullptr;
	DeathDissolveElapsedTime = 0.0f;
	bDeathDissolveVisualStarted = false;

	PendingDeathDissolveMontage = InDeathMontage;
	PendingDeathDissolveMontagePlayRate = FMath::Max(InMontagePlayRate, UE_SMALL_NUMBER);
	PendingDeathDissolveDelay = FMath::Max(InDissolveDelay, 0.0f);
	PendingDeathDissolveDuration = FMath::Max(InDissolveDuration, 0.0f);
	PendingDeathDissolveStartValue = InDissolveStartValue;
	PendingDeathDissolveEndValue = InDissolveEndValue;
	PendingDeathDissolveScalarParameterName = InDissolveScalarParameterName != NAME_None
		? InDissolveScalarParameterName
		: TEXT("DissolveAmount");
	PendingDeathDissolveNiagaraSystem = InDissolveNiagaraSystem;
	PendingDeathDissolveNiagaraParameterName = InNiagaraDissolveParameterName != NAME_None
		? InNiagaraDissolveParameterName
		: TEXT("User.DissolveAmount");
	PendingDeathDissolveTexture = InDissolveTexture;
	PendingDeathDissolveTextureUV = InDissolveTextureUV;
	PendingDeathDissolveTickInterval = FMath::Max(InDissolveTickInterval, 0.001f);

	if (!IsValid(PendingDeathDissolveMontage))
	{
		BeginDeathDissolveVisual();
		return;
	}

	// Death는 곧바로 Dead 태그, AI 정지, Dissolve 예약이 이어지므로 클라이언트 시각 몽타주를 한 번 더 보장한다.
	if (!HasAuthority())
	{
		PlayDeathDissolveMontageIfNeeded();
	}

	const float MontageDelay = (PendingDeathDissolveMontage->GetPlayLength() / PendingDeathDissolveMontagePlayRate) + 0.1f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathDissolveStartTimerHandle,
			this,
			&APREnemyBaseCharacter::BeginDeathDissolveVisual,
			FMath::Max(MontageDelay, 0.0f),
			false);
	}
	else
	{
		BeginDeathDissolveVisual();
	}
}

void APREnemyBaseCharacter::PlayDeathDissolveMontageIfNeeded()
{
	if (!IsValid(PendingDeathDissolveMontage))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (AnimInstance->Montage_IsPlaying(PendingDeathDissolveMontage))
	{
		return;
	}

	AnimInstance->Montage_Play(PendingDeathDissolveMontage, PendingDeathDissolveMontagePlayRate);
}

void APREnemyBaseCharacter::BeginDeathDissolveVisual()
{
	if (bDeathDissolveVisualStarted)
	{
		return;
	}

	bDeathDissolveVisualStarted = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveStartTimerHandle);
	}

	FreezeDeathDissolvePose();

	if (PendingDeathDissolveDelay <= 0.0f)
	{
		StartDeathDissolveVisual();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathDissolveStartTimerHandle,
			this,
			&APREnemyBaseCharacter::StartDeathDissolveVisual,
			PendingDeathDissolveDelay,
			false);
	}
	else
	{
		StartDeathDissolveVisual();
	}
}

void APREnemyBaseCharacter::StartDeathDissolveVisual()
{
	DeathDissolveElapsedTime = 0.0f;
	DeathDissolveDynamicMaterials.Reset();

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (IsValid(MeshComponent))
	{
		const int32 MaterialCount = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInterface* CurrentMaterial = MeshComponent->GetMaterial(MaterialIndex);
			if (!IsValid(CurrentMaterial))
			{
				continue;
			}

			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
			if (!IsValid(DynamicMaterial))
			{
				DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(MaterialIndex, CurrentMaterial);
			}

			if (IsValid(DynamicMaterial))
			{
				DynamicMaterial->SetScalarParameterValue(
					PendingDeathDissolveScalarParameterName,
					PendingDeathDissolveStartValue);
				DeathDissolveDynamicMaterials.Add(DynamicMaterial);
			}
		}

		if (IsValid(PendingDeathDissolveNiagaraSystem))
		{
			ActiveDeathDissolveNiagara = UNiagaraFunctionLibrary::SpawnSystemAttached(
				PendingDeathDissolveNiagaraSystem,
				MeshComponent,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true,
				true);

			if (IsValid(ActiveDeathDissolveNiagara))
			{
				ActiveDeathDissolveNiagara->SetVariableFloat(
					PendingDeathDissolveNiagaraParameterName,
					PendingDeathDissolveStartValue);

				if (IsValid(PendingDeathDissolveTexture))
				{
					ActiveDeathDissolveNiagara->SetVariableTexture(TEXT("User.DissolveTexture"), PendingDeathDissolveTexture);
				}

				ActiveDeathDissolveNiagara->SetVariableVec2(TEXT("User.DissolveTextureUV"), PendingDeathDissolveTextureUV);
				ActiveDeathDissolveNiagara->Activate(true);
			}
		}
	}

	ApplyDeathDissolveVisualValue(PendingDeathDissolveStartValue);

	if (PendingDeathDissolveDuration <= UE_SMALL_NUMBER)
	{
		ApplyDeathDissolveVisualValue(PendingDeathDissolveEndValue);
		CompleteDeathDissolveVisual();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathDissolveTickTimerHandle,
			this,
			&APREnemyBaseCharacter::TickDeathDissolveVisual,
			PendingDeathDissolveTickInterval,
			true);
	}
	else
	{
		CompleteDeathDissolveVisual();
	}
}

void APREnemyBaseCharacter::TickDeathDissolveVisual()
{
	DeathDissolveElapsedTime += PendingDeathDissolveTickInterval;

	const float Duration = FMath::Max(PendingDeathDissolveDuration, UE_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(DeathDissolveElapsedTime / Duration, 0.0f, 1.0f);
	const float DissolveValue = FMath::Lerp(
		PendingDeathDissolveStartValue,
		PendingDeathDissolveEndValue,
		Alpha);

	ApplyDeathDissolveVisualValue(DissolveValue);

	if (Alpha >= 1.0f)
	{
		CompleteDeathDissolveVisual();
	}
}

void APREnemyBaseCharacter::CompleteDeathDissolveVisual()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveTickTimerHandle);
	}

	ApplyDeathDissolveVisualValue(PendingDeathDissolveEndValue);

	if (IsValid(ActiveDeathDissolveNiagara))
	{
		ActiveDeathDissolveNiagara->Deactivate();
		ActiveDeathDissolveNiagara = nullptr;
	}
}

void APREnemyBaseCharacter::ApplyDeathDissolveVisualValue(float DissolveValue)
{
	for (UMaterialInstanceDynamic* DynamicMaterial : DeathDissolveDynamicMaterials)
	{
		if (IsValid(DynamicMaterial))
		{
			DynamicMaterial->SetScalarParameterValue(PendingDeathDissolveScalarParameterName, DissolveValue);
		}
	}

	if (IsValid(ActiveDeathDissolveNiagara))
	{
		ActiveDeathDissolveNiagara->SetVariableFloat(PendingDeathDissolveNiagaraParameterName, DissolveValue);
	}
}

void APREnemyBaseCharacter::FreezeDeathDissolvePose()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	MeshComponent->bPauseAnims = true;
	MeshComponent->GlobalAnimRateScale = 0.0f;
}

void APREnemyBaseCharacter::CacheMovementPresentationDefaults()
{
	if (bHasCachedMovementPresentation)
	{
		return;
	}

	if (!IsValid(GetCharacterMovement()))
	{
		return;
	}

	bHasCachedMovementPresentation = true;
	CachedMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	CachedRotationRate = GetCharacterMovement()->RotationRate;
	bCachedOrientRotationToMovement = GetCharacterMovement()->bOrientRotationToMovement;
	bCachedUseControllerDesiredRotation = GetCharacterMovement()->bUseControllerDesiredRotation;
	bCachedUseControllerRotationYaw = bUseControllerRotationYaw;
}

void APREnemyBaseCharacter::ApplyMovementPresentationConfig(const FPREnemyMovePresentationConfig& PresentationConfig)
{
	if (!IsValid(GetCharacterMovement()))
	{
		return;
	}

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = PresentationConfig.bOrientRotationToMovement;
	GetCharacterMovement()->bUseControllerDesiredRotation = PresentationConfig.bUseControllerDesiredRotation;

	if (PresentationConfig.RotationYawRate > 0.0f)
	{
		GetCharacterMovement()->RotationRate = FRotator(0.0f, PresentationConfig.RotationYawRate, 0.0f);
	}
	else if (bHasCachedMovementPresentation)
	{
		GetCharacterMovement()->RotationRate = CachedRotationRate;
	}

	if (PresentationConfig.MoveSpeedOverride > 0.0f)
	{
		GetCharacterMovement()->MaxWalkSpeed = PresentationConfig.MoveSpeedOverride;
	}
	else if (bHasCachedMovementPresentation)
	{
		GetCharacterMovement()->MaxWalkSpeed = CachedMaxWalkSpeed;
	}
}

void APREnemyBaseCharacter::RestoreMovementPresentationDefaults()
{
	if (!bHasCachedMovementPresentation || !IsValid(GetCharacterMovement()))
	{
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = CachedMaxWalkSpeed;
	GetCharacterMovement()->RotationRate = CachedRotationRate;
	GetCharacterMovement()->bOrientRotationToMovement = bCachedOrientRotationToMovement;
	GetCharacterMovement()->bUseControllerDesiredRotation = bCachedUseControllerDesiredRotation;
	bUseControllerRotationYaw = bCachedUseControllerRotationYaw;
	bHasCachedMovementPresentation = false;
}
FPRDamageRegionInfo APREnemyBaseCharacter::GetDamageRegionInfo(FName BoneName) const
{
	if (bHasCachedStatRow)
	{
		if (const FPRDamageRegionInfo* Found = CachedStatRow.RegionMap.Find(BoneName))
		{
			return *Found;
		}
	}
	return FPRDamageRegionInfo();
}

void APREnemyBaseCharacter::OnPostDamageApplied(const FPRDamageAppliedContext& Context)
{
	Super::OnPostDamageApplied(Context);

	// 서버에서만 실행. InstigatorController에게 Unreliable RPC로 데미지 텍스트를 전송한다
	if (!HasAuthority())
	{
		return;
	}

	if (Context.FinalDamage > 0.0f && IsValid(ThreatComponent))
	{
		AActor* ThreatSourceActor = ResolveEnemyThreatSourceActor(Context);
		if (UPRCombatStatics::GetActorTeam(ThreatSourceActor) == EPRTeam::Player)
		{
			ThreatComponent->AddDamageThreat(ThreatSourceActor, Context.FinalDamage);
		}
	}

	APRPlayerController* PC = Cast<APRPlayerController>(Context.InstigatorController.Get());
	if (!IsValid(PC))
	{
		return;
	}

	UPRFloatingTextManager* FloatingTextManager = PC->GetFloatingTextManager();
	if (!IsValid(FloatingTextManager))
	{
		return;
	}

	// 텍스트 타입 결정
	EPRFloatingTextType TextType = EPRFloatingTextType::NormalDamage;
	if (Context.Region.IsWeakpoint())
	{
		TextType = Context.bIsCritical ? EPRFloatingTextType::WeakCritical : EPRFloatingTextType::WeakNormal;
	}
	else if (Context.bIsCritical)
	{
		TextType = EPRFloatingTextType::CriticalDamage;
	}

	FPRFloatingTextRequest Request;
	Request.Text = FText::AsNumber(FMath::CeilToInt(Context.FinalDamage));
	Request.TextType = TextType;
	Request.WorldLocation = Context.HitResult.ImpactPoint;

	FloatingTextManager->ClientShowFloatingText_Unreliable(Request);
}

void APREnemyBaseCharacter::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists)
{
	Super::HandleGameplayTagUpdated(ChangedTag, TagExists);
	
	if (ChangedTag.MatchesTag(PRGameplayTags::State_Dead))
	{
		HandleDeadTagChanged(TagExists);
	}
	if (ChangedTag.MatchesTag(PRGameplayTags::State_Groggy))
	{
		HandleGroggyTagChanged(TagExists);
	}
}

void APREnemyBaseCharacter::InitializeHomeLocation()
{
	if (bHasInitializedHomeLocation)
	{
		return;
	}

	HomeLocation = GetActorLocation();
	bHasInitializedHomeLocation = true;
}

void APREnemyBaseCharacter::InitializeEnemyAbilitySystem()
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry())
	{
		// CharacterID와 Role을 기준으로 DataTable 초기 스탯을 주입한다.
		AbilitySystemComponent->InitializeAttributesFromRegistry(Registry, GetCharacterRole(), CharacterID);

		// 부위 매핑 조회를 위해 동일 Row를 캐릭터에도 캐싱한다.
		if (UDataTable* StatTable = Registry->GetStatTableSynchronous(GetCharacterRole()))
		{
			static const FString ContextString(TEXT("APREnemyBaseCharacter::CacheStatRow"));
			if (const FPREnemyStatRow* FoundRow = StatTable->FindRow<FPREnemyStatRow>(CharacterID, ContextString, false))
			{
				CachedStatRow = *FoundRow;
				bHasCachedStatRow = true;
			}
		}
	}

	// 재 Possess나 재초기화 상황에서 이전 AbilitySet이 중복 부여되지 않도록 먼저 회수한다.
	AbilitySystemComponent->ClearAbilitySetByHandles(GrantedAbilitySetHandles);

	if (IsValid(AbilitySet))
	{
		AbilitySystemComponent->GiveAbilitySet(AbilitySet, GrantedAbilitySetHandles);
	}

	InitializeEnemyWorldHealthBar();
}

void APREnemyBaseCharacter::InitializeEnemyWorldHealthBar()
{
	if (!IsValid(EnemyWorldHealthBarComponent))
	{
		return;
	}

	if (!bUseWorldHealthBar)
	{
		EnemyWorldHealthBarComponent->HideHealthBar();
		EnemyWorldHealthBarComponent->SetComponentTickEnabled(false);
		return;
	}

	EnemyWorldHealthBarComponent->SetComponentTickEnabled(true);
	EnemyWorldHealthBarComponent->InitializeFromAbilitySystem(AbilitySystemComponent);
}

void APREnemyBaseCharacter::HandleDeath(AActor* InstigatorActor)
{
	// 사망은 Ability 쪽에서도 처리하지만, 캐릭터 레벨에서도 Brain/Movement를 확실히 멈춘다.
	GetCharacterMovement()->DisableMovement();

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();

		if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
		{
			BrainComponent->StopLogic(TEXT("Dead"));
		}
	}
}

void APREnemyBaseCharacter::HandleDeadTagChanged(bool bEntered)
{
	// 사망 상태 진입
	if (bEntered)
	{
		if (IsValid(ThreatComponent))
		{
			ThreatComponent->ForceClearAttackCommit();
		}

		ClearCombatMovePresentationContext();
		HandleDeath(nullptr);
	}
}

void APREnemyBaseCharacter::HandleGroggyTagChanged(bool bEntered)
{
	// 그로기 상태 진입
	if (bEntered)
	{
		if (IsValid(ThreatComponent))
		{
			ThreatComponent->ForceClearAttackCommit();
		}

		ClearCombatMovePresentationContext();
		GetCharacterMovement()->StopMovementImmediately();
	}
}
