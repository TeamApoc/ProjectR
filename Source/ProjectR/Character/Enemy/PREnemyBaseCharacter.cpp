// Copyright ProjectR. All Rights Reserved.

#include "PREnemyBaseCharacter.h"

#include "ProjectR/AI/Controllers/PREnemyAIController.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AI/Components/PREnemyCombatEventRelayComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/AI/Data/PRPerceptionConfig.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "Engine/DataTable.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"
#include "Net/UnrealNetwork.h"

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

	ArmorCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ArmorCollision"));
	ArmorCollision->SetupAttachment(GetMesh());
	ArmorCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 데미지 계산은 ComponentTag 접두사로 Armor/Weakpoint를 구분한다.
	ArmorCollision->ComponentTags.Add(TEXT("Armor.Torso"));

	WeakpointCollision = CreateDefaultSubobject<USphereComponent>(TEXT("WeakpointCollision"));
	WeakpointCollision->SetupAttachment(GetMesh());
	WeakpointCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeakpointCollision->ComponentTags.Add(TEXT("Weakpoint.Head"));
}

void APREnemyBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	BindTagChangeEvent();
	
	// 배치된 위치를 복귀 기준점으로 저장한다.
	HomeLocation = GetActorLocation();

	if (IsValid(CommonSet))
	{
		// CommonSet->OnDeath.AddDynamic(this, &APREnemyBaseCharacter::HandleDeath);
		// !!!Note (26.04.22, Yuchan): 이미 Tag 이벤트로 Death 처리 함수를 호출하기 때문에 위 이벤트 바인딩 불필요하여 주석처리 
	}
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

UPREnemyCombatDataAsset* APREnemyBaseCharacter::GetCombatDataAsset() const
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
		ClearCombatMovePresentationContext();
		GetCharacterMovement()->DisableMovement();
	}
}

void APREnemyBaseCharacter::HandleGroggyTagChanged(bool bEntered)
{
	// 그로기 상태 진입
	if (bEntered)
	{
		ClearCombatMovePresentationContext();
		GetCharacterMovement()->StopMovementImmediately();
	}
}
