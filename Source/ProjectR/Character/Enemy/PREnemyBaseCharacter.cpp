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
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/AI/Data/PRPerceptionConfig.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"

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

	// 배치된 위치를 복귀 기준점으로 저장한다.
	HomeLocation = GetActorLocation();

	if (IsValid(CommonSet))
	{
		CommonSet->OnDeath.AddDynamic(this, &APREnemyBaseCharacter::HandleDeath);
	}

	if (IsValid(EnemySet))
	{
		EnemySet->OnGroggyStateChanged.AddDynamic(this, &APREnemyBaseCharacter::HandleGroggyStateChanged);
	}
}

void APREnemyBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitializeEnemyAbilitySystem();
	BindGameplayTagEvents();
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
	}

	// 재 Possess나 재초기화 상황에서 이전 AbilitySet이 중복 부여되지 않도록 먼저 회수한다.
	AbilitySystemComponent->ClearAbilitySetByHandles(GrantedAbilitySetHandles);

	if (IsValid(AbilitySet))
	{
		AbilitySystemComponent->GiveAbilitySet(AbilitySet, GrantedAbilitySetHandles);
	}
}

void APREnemyBaseCharacter::BindGameplayTagEvents()
{
	if (bGameplayTagEventsBound || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	AbilitySystemComponent->RegisterGameplayTagEvent(PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APREnemyBaseCharacter::HandleDeadTagChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(PRGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APREnemyBaseCharacter::HandleGroggyTagChanged);

	bGameplayTagEventsBound = true;
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

void APREnemyBaseCharacter::HandleGroggyStateChanged(bool bEntered)
{
	if (bEntered)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void APREnemyBaseCharacter::HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	GetCharacterMovement()->DisableMovement();
}

void APREnemyBaseCharacter::HandleGroggyTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}
