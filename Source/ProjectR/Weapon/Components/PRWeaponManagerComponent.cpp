// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponManagerComponent.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"

UPRWeaponManagerComponent::UPRWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPRWeaponManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Primary);
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Secondary);
	Super::EndPlay(EndPlayReason);
}

void UPRWeaponManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPRWeaponManagerComponent, ArmedState);
	DOREPLIFETIME(UPRWeaponManagerComponent, ActiveSlot);
	DOREPLIFETIME(UPRWeaponManagerComponent, PrimaryVisualSlot);
	DOREPLIFETIME(UPRWeaponManagerComponent, SecondaryVisualSlot);
}

UPRWeaponDataAsset* UPRWeaponManagerComponent::GetWeaponDataBySlot(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponData;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponData;
	}

	return nullptr;
}

const FPRWeaponVisualSlot& UPRWeaponManagerComponent::GetVisualSlotBySlotType(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryVisualSlot;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryVisualSlot;
	}

	static FPRWeaponVisualSlot EmptyVisualSlot;
	return EmptyVisualSlot;
}

void UPRWeaponManagerComponent::EquipTestWeaponToSlot(UPRWeaponDataAsset* WeaponData, EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청이면 서버 전송 전에 차단한다.
	if (!IsSupportedSlot(TargetSlot))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		EquipTestWeaponToSlotInternal(WeaponData, TargetSlot);
		return;
	}

	Server_EquipTestWeaponToSlot(WeaponData, TargetSlot);
}

void UPRWeaponManagerComponent::UnequipWeaponSlot(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청이면 서버 전송 전에 차단한다.
	if (!IsSupportedSlot(TargetSlot))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UnequipWeaponSlotInternal(TargetSlot);
		return;
	}

	Server_UnequipWeaponSlot(TargetSlot);
}

void UPRWeaponManagerComponent::SwapActiveSlot(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯 요청이면 서버 전송 전에 차단한다.
	if (!IsSupportedSlot(TargetSlot))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		SwapActiveSlotInternal(TargetSlot);
		return;
	}

	Server_SwapActiveSlot(TargetSlot);
}

void UPRWeaponManagerComponent::SetWeaponArmedState(EPRWeaponArmedState NewArmedState)
{
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		SetWeaponArmedStateInternal(NewArmedState);
		return;
	}

	Server_SetWeaponArmedState(NewArmedState);
}

APRWeaponActor* UPRWeaponManagerComponent::GetActiveWeaponActor() const
{
	return GetWeaponActorBySlot(ActiveSlot.SlotType);
}

void UPRWeaponManagerComponent::EquipTestWeaponToSlotInternal(UPRWeaponDataAsset* WeaponData,
                                                              EPRWeaponSlotType TargetSlot)
{
	// 장착 결과를 검증할 수 있는 최소 데이터가 없으면 상태를 바꾸지 않는다.
	if (!IsSupportedSlot(TargetSlot) || !IsValid(WeaponData))
	{
		return;
	}

	// 같은 슬롯에 같은 무기가 이미 연결되어 있으면 중복 장착과 로그를 생략한다.
	if (GetWeaponDataBySlot(TargetSlot) == WeaponData)
	{
		return;
	}

	const FPRActiveWeaponSlot PreviousActiveSlot = ActiveSlot;

	if (TargetSlot == EPRWeaponSlotType::Primary)
	{
		PrimaryWeaponData = WeaponData;
	}
	else if (TargetSlot == EPRWeaponSlotType::Secondary)
	{
		SecondaryWeaponData = WeaponData;
	}

	// 현재 활성 슬롯이 비어 있으면 첫 장착 무기를 활성 슬롯으로 지정한다.
	// 이미 활성 중인 슬롯에 다시 장착하는 경우에도 같은 슬롯 선택을 유지한다.
	if (ActiveSlot.IsEmpty() || ActiveSlot.SlotType == TargetSlot)
	{
		if (!PreviousActiveSlot.IsEmpty() && PreviousActiveSlot.SlotType == TargetSlot && PreviousActiveSlot.WeaponData
			!= WeaponData)
		{
			ClearWeaponAbilitiesSkeleton(PreviousActiveSlot);
		}

		ActiveSlot = BuildActiveSlot(TargetSlot, WeaponData);
		GrantWeaponAbilitiesSkeleton(ActiveSlot);
	}

	// 활성 슬롯과 원본 슬롯 데이터를 기준으로 두 슬롯의 공개 비주얼 상태를 다시 구성한다.
	RefreshVisualSlotsFromCurrentState();

	// 서버 로컬도 복제 콜백과 같은 경로로 슬롯별 Actor를 즉시 최신화한다.
	RefreshAllWeaponActors();
}

void UPRWeaponManagerComponent::UnequipWeaponSlotInternal(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯이거나 비어 있는 슬롯이면 해제를 수행하지 않는다.
	if (!IsSupportedSlot(TargetSlot) || !IsValid(GetWeaponDataBySlot(TargetSlot)))
	{
		return;
	}

	const FPRActiveWeaponSlot PreviousActiveSlot = ActiveSlot;
	const bool bWasActiveSlot = !PreviousActiveSlot.IsEmpty() && PreviousActiveSlot.SlotType == TargetSlot;
	if (bWasActiveSlot)
	{
		ClearWeaponAbilitiesSkeleton(PreviousActiveSlot);
	}

	if (TargetSlot == EPRWeaponSlotType::Primary)
	{
		PrimaryWeaponData = nullptr;
	}
	else if (TargetSlot == EPRWeaponSlotType::Secondary)
	{
		SecondaryWeaponData = nullptr;
	}

	if (bWasActiveSlot)
	{
		ActiveSlot = ResolveNextActiveSlotAfterUnequip(TargetSlot);

		if (!ActiveSlot.IsEmpty())
		{
			GrantWeaponAbilitiesSkeleton(ActiveSlot);
		}
		else
		{
			ArmedState = EPRWeaponArmedState::Unarmed;
		}
	}

	RefreshVisualSlotsFromCurrentState();
	RefreshAllWeaponActors();
}

void UPRWeaponManagerComponent::SetWeaponArmedStateInternal(EPRWeaponArmedState NewArmedState)
{
	// 무장 상태가 같으면 공개 상태를 다시 계산하지 않는다.
	if (ArmedState == NewArmedState)
	{
		return;
	}

	ArmedState = NewArmedState;
	RefreshVisualSlotsFromCurrentState();
	RefreshAllWeaponActors();
}

void UPRWeaponManagerComponent::SwapActiveSlotInternal(EPRWeaponSlotType TargetSlot)
{
	// 지원하지 않는 슬롯이거나 빈 슬롯이면 활성 슬롯 전환을 수행하지 않는다.
	if (!IsSupportedSlot(TargetSlot))
	{
		return;
	}

	UPRWeaponDataAsset* TargetWeaponData = GetWeaponDataBySlot(TargetSlot);
	if (!IsValid(TargetWeaponData))
	{
		return;
	}

	// 이미 같은 슬롯이 활성 상태면 중복 전환을 생략한다.
	if (!ActiveSlot.IsEmpty() && ActiveSlot.SlotType == TargetSlot && ActiveSlot.WeaponData == TargetWeaponData)
	{
		return;
	}

	const FPRActiveWeaponSlot PreviousActiveSlot = ActiveSlot;
	if (!PreviousActiveSlot.IsEmpty())
	{
		ClearWeaponAbilitiesSkeleton(PreviousActiveSlot);
	}

	ActiveSlot = BuildActiveSlot(TargetSlot, TargetWeaponData);
	RefreshVisualSlotsFromCurrentState();

	GrantWeaponAbilitiesSkeleton(ActiveSlot);

	// 슬롯 전환은 Actor를 재생성하지 않고 소켓 부착만 최신화하는 경로를 유지한다.
	RefreshAllWeaponActors();
}

void UPRWeaponManagerComponent::RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType)
{
	// 지원하지 않는 슬롯 요청이면 로컬 Actor 갱신을 진행하지 않는다.
	if (!IsSupportedSlot(SlotType))
	{
		return;
	}

	const FPRWeaponVisualSlot& VisualSlot = GetVisualSlotBySlotType(SlotType);
	TObjectPtr<APRWeaponActor>& WeaponActor = GetMutableWeaponActorBySlot(SlotType);

	// 슬롯이 비어 있으면 해당 슬롯 Actor도 함께 제거한다.
	if (VisualSlot.IsEmpty())
	{
		DestroyWeaponActorForSlot(SlotType);
		return;
	}

	// 월드, 소유자, Actor 클래스가 준비되지 않았으면 공개 Actor 생성 경로를 보류한다.
	if (!IsValid(GetWorld()) || !IsValid(GetOwner()) || !IsValid(VisualSlot.WeaponData) || !IsValid(
		VisualSlot.WeaponData->WeaponActorClass))
	{
		return;
	}

	const bool bNeedsRespawn = !IsValid(WeaponActor)
		|| WeaponActor->GetClass() != VisualSlot.WeaponData->WeaponActorClass
		|| WeaponActor->GetVisualSlot().WeaponData != VisualSlot.WeaponData;

	// 같은 슬롯에 다른 무기를 다시 장착한 경우에만 기존 Actor를 정리하고 새 무기로 재생성한다.
	if (bNeedsRespawn)
	{
		DestroyWeaponActorForSlot(SlotType);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		WeaponActor = GetWorld()->SpawnActor<APRWeaponActor>(
			VisualSlot.WeaponData->WeaponActorClass,
			FTransform::Identity,
			SpawnParameters);
	}

	// Actor 생성에 실패하면 이후 시각화 경로를 중단한다.
	if (!IsValid(WeaponActor))
	{
		return;
	}

	WeaponActor->InitializeFromVisualSlot(VisualSlot);
	RefreshWeaponAttachmentForSlot(SlotType);
}

void UPRWeaponManagerComponent::RefreshAllWeaponActors()
{
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary);
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary);
}

void UPRWeaponManagerComponent::RefreshWeaponAttachmentForSlot(EPRWeaponSlotType SlotType)
{
	// 부착 갱신은 장착된 슬롯 Actor와 캐릭터 소유자가 모두 존재할 때만 진행한다.
	if (!IsSupportedSlot(SlotType))
	{
		return;
	}

	APRWeaponActor* WeaponActor = GetWeaponActorBySlot(SlotType);
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!IsValid(WeaponActor) || !IsValid(OwnerCharacter))
	{
		return;
	}

	const FPRWeaponVisualSlot& VisualSlot = GetVisualSlotBySlotType(SlotType);
	const FName AttachSocketName = ResolveAttachSocketName(SlotType, VisualSlot.CarryState);
	WeaponActor->AttachToOwnerMesh(OwnerCharacter, AttachSocketName);
}

void UPRWeaponManagerComponent::OnRep_ActiveSlot(FPRActiveWeaponSlot OldActiveSlot)
{
	// 활성 슬롯이 실제로 바뀌었을 때만 현재 보이는 두 슬롯의 부착 상태를 다시 맞춘다.
	if (OldActiveSlot == ActiveSlot)
	{
		return;
	}

	RefreshAllWeaponActors();
}

void UPRWeaponManagerComponent::OnRep_PrimaryVisualSlot(FPRWeaponVisualSlot OldVisualSlot)
{
	// 주무기 공개 비주얼 상태가 달라졌을 때만 해당 슬롯 Actor를 갱신한다.
	if (OldVisualSlot == PrimaryVisualSlot)
	{
		return;
	}

	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary);
}

void UPRWeaponManagerComponent::OnRep_SecondaryVisualSlot(FPRWeaponVisualSlot OldVisualSlot)
{
	// 보조무기 공개 비주얼 상태가 달라졌을 때만 해당 슬롯 Actor를 갱신한다.
	if (OldVisualSlot == SecondaryVisualSlot)
	{
		return;
	}

	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary);
}

void UPRWeaponManagerComponent::OnRep_ArmedState(EPRWeaponArmedState OldArmedState)
{
	// 무장 상태가 실제로 바뀌었을 때만 로컬 Actor 부착 상태를 다시 맞춘다.
	if (OldArmedState == ArmedState)
	{
		return;
	}

	RefreshAllWeaponActors();
}

FPRActiveWeaponSlot UPRWeaponManagerComponent::BuildActiveSlot(EPRWeaponSlotType SlotType,
                                                               UPRWeaponDataAsset* WeaponData) const
{
	FPRActiveWeaponSlot NewActiveSlot;
	NewActiveSlot.SlotType = SlotType;
	NewActiveSlot.WeaponData = WeaponData;
	return NewActiveSlot;
}

void UPRWeaponManagerComponent::Server_EquipTestWeaponToSlot_Implementation(
	UPRWeaponDataAsset* WeaponData, EPRWeaponSlotType TargetSlot)
{
	EquipTestWeaponToSlotInternal(WeaponData, TargetSlot);
}

void UPRWeaponManagerComponent::Server_UnequipWeaponSlot_Implementation(EPRWeaponSlotType TargetSlot)
{
	UnequipWeaponSlotInternal(TargetSlot);
}

void UPRWeaponManagerComponent::Server_SwapActiveSlot_Implementation(EPRWeaponSlotType TargetSlot)
{
	SwapActiveSlotInternal(TargetSlot);
}

void UPRWeaponManagerComponent::Server_SetWeaponArmedState_Implementation(EPRWeaponArmedState NewArmedState)
{
	SetWeaponArmedStateInternal(NewArmedState);
}

FPRWeaponVisualSlot UPRWeaponManagerComponent::BuildVisualSlot(EPRWeaponSlotType SlotType,
                                                               UPRWeaponDataAsset* WeaponData) const
{
	FPRWeaponVisualSlot NewVisualSlot;
	NewVisualSlot.SlotType = SlotType;
	NewVisualSlot.WeaponData = WeaponData;
	NewVisualSlot.CarryState = ResolveCarryState(SlotType);

	// 슬롯은 유지하되 무기가 비어 있으면 비주얼 상태만 초기화한다.
	if (!IsValid(WeaponData))
	{
		NewVisualSlot.Reset(SlotType);
	}

	return NewVisualSlot;
}

EPRWeaponCarryState UPRWeaponManagerComponent::ResolveCarryState(EPRWeaponSlotType SlotType) const
{
	// 현재 무장 상태가 Armed 이고 활성 슬롯과 일치하는 슬롯만 손에 든 상태로 취급한다.
	if (ArmedState == EPRWeaponArmedState::Armed
		&& !ActiveSlot.IsEmpty()
		&& ActiveSlot.SlotType == SlotType)
	{
		return EPRWeaponCarryState::Armed;
	}

	return EPRWeaponCarryState::Stowed;
}

FName UPRWeaponManagerComponent::ResolveAttachSocketName(EPRWeaponSlotType SlotType,
                                                         EPRWeaponCarryState CarryState) const
{
	// 손에 든 무기는 항상 Gun_Attach를 사용한다.
	if (CarryState == EPRWeaponCarryState::Armed)
	{
		return TEXT("Gun_Attach");
	}

	// 수납 상태 무기는 슬롯 타입에 맞는 고정 소켓을 사용한다.
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return TEXT("LongGun_Stow");
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return TEXT("Pistol_Stow");
	}

	return NAME_None;
}

void UPRWeaponManagerComponent::RefreshVisualSlotsFromCurrentState()
{
	PrimaryVisualSlot = BuildVisualSlot(EPRWeaponSlotType::Primary, PrimaryWeaponData);
	SecondaryVisualSlot = BuildVisualSlot(EPRWeaponSlotType::Secondary, SecondaryWeaponData);
}

FPRActiveWeaponSlot UPRWeaponManagerComponent::ResolveNextActiveSlotAfterUnequip(EPRWeaponSlotType UnequippedSlot) const
{
	FPRActiveWeaponSlot NextActiveSlot;

	// 제거된 슬롯 반대편에 장착 무기가 남아 있으면 그 슬롯을 다음 활성 슬롯으로 선택한다.
	if (UnequippedSlot != EPRWeaponSlotType::Primary && IsValid(PrimaryWeaponData))
	{
		return BuildActiveSlot(EPRWeaponSlotType::Primary, PrimaryWeaponData);
	}

	if (UnequippedSlot != EPRWeaponSlotType::Secondary && IsValid(SecondaryWeaponData))
	{
		return BuildActiveSlot(EPRWeaponSlotType::Secondary, SecondaryWeaponData);
	}

	return NextActiveSlot;
}

void UPRWeaponManagerComponent::GrantWeaponAbilitiesSkeleton(const FPRActiveWeaponSlot& WeaponSlot) const
{
	// 1차 단계에서는 서버에서만 부여 시점을 로그로 남기고 실제 AbilitySet은 연결하지 않는다.
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || WeaponSlot.IsEmpty())
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon ability skeleton granted. Owner=%s Slot=%d Weapon=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(WeaponSlot.SlotType),
		*GetNameSafe(WeaponSlot.WeaponData));
}

void UPRWeaponManagerComponent::ClearWeaponAbilitiesSkeleton(const FPRActiveWeaponSlot& WeaponSlot) const
{
	// 1차 단계에서는 서버에서만 해제 시점을 로그로 남기고 실제 AbilitySet은 연결하지 않는다.
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || WeaponSlot.IsEmpty())
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon ability skeleton cleared. Owner=%s Slot=%d Weapon=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(WeaponSlot.SlotType),
		*GetNameSafe(WeaponSlot.WeaponData));
}

bool UPRWeaponManagerComponent::IsSupportedSlot(EPRWeaponSlotType SlotType) const
{
	return SlotType == EPRWeaponSlotType::Primary || SlotType == EPRWeaponSlotType::Secondary;
}

FPRWeaponVisualSlot& UPRWeaponManagerComponent::GetMutableVisualSlotBySlotType(EPRWeaponSlotType SlotType)
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryVisualSlot;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryVisualSlot;
	}

	checkNoEntry();
	return PrimaryVisualSlot;
}

APRWeaponActor* UPRWeaponManagerComponent::GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponActor;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponActor;
	}

	return nullptr;
}

TObjectPtr<APRWeaponActor>& UPRWeaponManagerComponent::GetMutableWeaponActorBySlot(EPRWeaponSlotType SlotType)
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponActor;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponActor;
	}

	checkNoEntry();
	return PrimaryWeaponActor;
}

void UPRWeaponManagerComponent::DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType)
{
	TObjectPtr<APRWeaponActor>& WeaponActor = GetMutableWeaponActorBySlot(SlotType);
	if (!IsValid(WeaponActor))
	{
		return;
	}

	WeaponActor->Destroy();
	WeaponActor = nullptr;
}
