// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponActor.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "ProjectR/Animation/Weapon/PRWeaponAnimInstance.h"

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

UPRWeaponAnimInstance* APRWeaponActor::GetWeaponAnimInstance() const
{
	if (!IsValid(WeaponMeshComponent))
	{
		return nullptr;
	}

	return Cast<UPRWeaponAnimInstance>(WeaponMeshComponent->GetAnimInstance());
}

bool APRWeaponActor::RequestWeaponAnimation(EPRWeaponAnimationState AnimationState)
{
	UPRWeaponAnimInstance* WeaponAnimInstance = GetWeaponAnimInstance();
	if (!IsValid(WeaponAnimInstance))
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("[WeaponActor] 무기 애니메이션 요청 실패. Actor=%s State=%s"),
			*GetNameSafe(this),
			*UEnum::GetValueAsString(AnimationState));
		return false;
	}

	switch (AnimationState)
	{
	case EPRWeaponAnimationState::Idle:
		WeaponAnimInstance->SetIdleState();
		break;
	case EPRWeaponAnimationState::Shoot:
		WeaponAnimInstance->RequestShoot();
		break;
	case EPRWeaponAnimationState::Reload:
		WeaponAnimInstance->RequestReload();
		break;
	default:
		return false;
	}

	return true;
}

bool APRWeaponActor::RequestShootAnimation()
{
	return RequestWeaponAnimation(EPRWeaponAnimationState::Shoot);
}

bool APRWeaponActor::RequestReloadAnimation()
{
	return RequestWeaponAnimation(EPRWeaponAnimationState::Reload);
}

bool APRWeaponActor::SetWeaponAnimationIdle()
{
	return RequestWeaponAnimation(EPRWeaponAnimationState::Idle);
}

void APRWeaponActor::PlayNiagaraEffect_Implementation(EPRWeaponEffectType EffectType, UNiagaraSystem* InNiagaraSystem)
{
	if (!IsValid(InNiagaraSystem))
	{
		return;
	}
	
	if (EffectType == EPRWeaponEffectType::MuzzleFlash || EffectType == EPRWeaponEffectType::ProjectileLaunch)
	{
		FTransform MuzzleTransform = GetMuzzleTransform();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,InNiagaraSystem,
			MuzzleTransform.GetLocation(),
			MuzzleTransform.Rotator(),
			MuzzleTransform.GetScale3D(),
			true,
			true,
			ENCPoolMethod::AutoRelease);
	}
}

FTransform APRWeaponActor::GetMuzzleTransform_Implementation() const
{
	return GetTransform();
}
