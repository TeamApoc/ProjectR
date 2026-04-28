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

void APRWeaponActor::AttachToOwnerMesh(ACharacter* OwnerCharacter, FName SocketName)
{
	// 캐릭터, 메시, 소켓 이름 중 하나라도 유효하지 않으면 부착을 진행하지 않는다.
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->GetMesh()) || SocketName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponActor] Attach failed. Actor=%s Owner=%s Socket=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OwnerCharacter),
			*SocketName.ToString());
		return;
	}

	if (!OwnerCharacter->GetMesh()->DoesSocketExist(SocketName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponActor] Attach failed. Socket missing. Actor=%s Owner=%s Socket=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OwnerCharacter),
			*SocketName.ToString());
		return;
	}

    // 무기 메시 부착
	AttachToComponent(
		OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponActor] Attached. Actor=%s Owner=%s Socket=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OwnerCharacter),
		*SocketName.ToString());
}

FTransform APRWeaponActor::GetMuzzleTransform_Implementation() const
{
	return GetTransform();
}
