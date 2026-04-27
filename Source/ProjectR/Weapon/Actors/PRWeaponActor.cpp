// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponActor.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

APRWeaponActor::APRWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetupAttachment(SceneRoot);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);
}

void APRWeaponActor::BeginPlay()
{
	Super::BeginPlay();
}

void APRWeaponActor::InitializeFromVisualSlot(const FPRWeaponVisualSlot& InVisualSlot)
{
	VisualSlot = InVisualSlot;
	RefreshVisualMesh();
}

void APRWeaponActor::AttachToOwnerMesh(ACharacter* OwnerCharacter, FName SocketName)
{
	// 캐릭터, 메시, 소켓 이름 중 하나라도 유효하지 않으면 부착을 진행하지 않는다.
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->GetMesh()) || SocketName.IsNone())
	{
		return;
	}

	AttachToComponent(
		OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName);
}

FTransform APRWeaponActor::GetMuzzleTransform_Implementation() const
{
	return GetTransform();
}

void APRWeaponActor::RefreshVisualMesh()
{
	// 무기 데이터가 비어 있으면 표시 메시도 함께 비운다.
	if (!IsValid(VisualSlot.WeaponData))
	{
		WeaponMeshComponent->SetSkeletalMesh(nullptr);
		return;
	}

	WeaponMeshComponent->SetSkeletalMesh(VisualSlot.WeaponData->DisplayMesh);
}
