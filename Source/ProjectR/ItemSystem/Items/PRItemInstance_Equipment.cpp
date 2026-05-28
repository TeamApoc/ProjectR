// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance_Equipment.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRItemInstance_Equipment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool UPRItemInstance_Equipment::ActivateItem(const FPRItemActivationContext& ActivationContext)
{
	// 인벤토리 UI의 장비 목록 선택은 InventoryComponent 요청을 거쳐 서버에서 이 함수로 도착
	// 장비 슬롯 교체, 기존 장비 해제, 복제용 외형 정보 갱신은 EquipmentManager가 한 번에 처리
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// UserActor는 로컬 UI에서는 PlayerController로 넘어오며 서버에서는 PlayerState의 EquipmentManager로 해석
	// 장비 ItemInstance는 자기 슬롯 데이터만 알고 실제 장착 상태 배열은 EquipmentManager가 보관
	UPREquipmentManagerComponent* EquipmentManager = UPRGameplayStatics::GetEquipmentManagerComponent(ActivationContext.UserActor);
	if (!IsValid(EquipmentManager))
	{
		return false;
	}

	return EquipmentManager->EquipItem(this);
}

bool UPRItemInstance_Equipment::DeactivateItem(const FPRItemActivationContext& ActivationContext)
{
	// 장비 슬롯의 해제 항목과 이미 장착 중인 장비 항목은 모두 이 ItemInstance 비활성화 요청으로 처리
	// UI가 별도 해제 함수를 직접 부르지 않아 장착과 해제가 같은 InventoryComponent 검증 경로를 사용
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// 해제 대상 슬롯은 목록이 열렸던 UI 상태가 아니라 장비 데이터의 SlotType으로 결정
	// 목록이 갱신되거나 닫힌 뒤에도 ItemInstance 데이터만으로 같은 슬롯을 다시 찾는 구조
	UPREquipmentManagerComponent* EquipmentManager = UPRGameplayStatics::GetEquipmentManagerComponent(ActivationContext.UserActor);
	if (!IsValid(EquipmentManager))
	{
		return false;
	}

	return EquipmentManager->UnequipSlot(GetSlotType());
}

UPREquipmentDataAsset* UPRItemInstance_Equipment::GetEquipmentData() const
{
	return Cast<UPREquipmentDataAsset>(ItemData);
}

EPREquipmentSlotType UPRItemInstance_Equipment::GetSlotType() const
{
	const UPREquipmentDataAsset* Data = GetEquipmentData();
	return IsValid(Data) ? Data->GetSlotType() : EPREquipmentSlotType::None;
}

void UPRItemInstance_Equipment::OnEquipped(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	// EquipmentManager 장착 확정 이후의 장비별 후처리 지점
	// 현재는 로그만 남기며 추후 캐릭터 ChildMesh 교체나 스탯 적용 위치
	UE_LOG(LogTemp, Log, TEXT("[Equipment] 장착: %s | 슬롯: %d | Owner: %s"), 
		*GetNameSafe(GetEquipmentData()), 
		(int32)GetSlotType(), 
		*GetNameSafe(OwnerActor));
}

void UPRItemInstance_Equipment::OnUnequipped(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	// EquipmentManager 해제 확정 이후의 장비별 후처리 지점
	// 현재는 로그만 남기며 추후 캐릭터 ChildMesh 복원이나 스탯 제거 위치
	UE_LOG(LogTemp, Log, TEXT("[Equipment] 해제: %s | 슬롯: %d | Owner: %s"), 
		*GetNameSafe(GetEquipmentData()), 
		(int32)GetSlotType(), 
		*GetNameSafe(OwnerActor));
}

void UPRItemInstance_Equipment::FillSaveEntry(FPREquipmentSlotSaveEntry& OutEntry) const
{
	OutEntry = FPREquipmentSlotSaveEntry();
	OutEntry.SlotType = GetSlotType();
	// ItemIndex는 EquipmentManager 혹은 InventoryComponent에서 채운다.
}

void UPRItemInstance_Equipment::ApplySaveEntry(const FPREquipmentSlotSaveEntry& InEntry)
{
	// 인스턴스의 내부 상태가 필요해지면 복원
}
