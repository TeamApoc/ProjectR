// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRCheatHandler.h"

#include "Engine/NetDriver.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayEffect.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"


/*~ RPC 라우팅 ~*/

int32 UPRCheatHandler::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	return IsValid(PC) ? PC->GetFunctionCallspace(Function, Stack) : FunctionCallspace::Local;
}

bool UPRCheatHandler::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	if (!IsValid(PC))
	{
		return false;
	}

	UNetDriver* NetDriver = PC->GetNetDriver();
	if (!NetDriver)
	{
		return false;
	}

	NetDriver->ProcessRemoteFunction(PC, Function, Parms, OutParms, Stack, this);
	return true;
}


/*~ 리스폰 ~*/

void UPRCheatHandler::ServerCheatRespawn_Implementation()
{
#if !UE_BUILD_SHIPPING
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	if (!IsValid(PC))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!IsValid(GameMode))
	{
		return;
	}

	if (APawn* CurrentPawn = PC->GetPawn())
	{
		PC->UnPossess();
		CurrentPawn->Destroy();
	}

	GameMode->RestartPlayer(PC);
#endif
}


/*~ 자원 충전 ~*/

void UPRCheatHandler::ServerCheatFillAmmo_Implementation()
{
#if !UE_BUILD_SHIPPING
	UPRAbilitySystemComponent* ASC = GetOwningPlayerASC();
	if (!IsValid(ASC))
	{
		return;
	}

	const UPRAttributeSet_Weapon* WeaponSet = ASC->GetSet<UPRAttributeSet_Weapon>();
	if (!IsValid(WeaponSet))
	{
		return;
	}

	const EPRAmmoType AmmoTypes[] = { EPRAmmoType::Primary, EPRAmmoType::Secondary };
	for (const EPRAmmoType AmmoType : AmmoTypes)
	{
		const FGameplayAttribute MagazineAttr = UPRAttributeSet_Weapon::GetMagazineAmmoAttribute(AmmoType);
		const FGameplayAttribute ReserveAttr = UPRAttributeSet_Weapon::GetReserveAmmoAttribute(AmmoType);

		if (MagazineAttr.IsValid())
		{
			ASC->SetNumericAttributeBase(MagazineAttr, WeaponSet->GetMaxMagazineAmmoByType(AmmoType));
		}

		if (ReserveAttr.IsValid())
		{
			ASC->SetNumericAttributeBase(ReserveAttr, WeaponSet->GetMaxReserveAmmoByType(AmmoType));
		}
	}
#endif
}


/*~ 무한 모드 ~*/

void UPRCheatHandler::ServerCheatInfiniteMode_Implementation(bool bEnable)
{
#if !UE_BUILD_SHIPPING
	UPRAbilitySystemComponent* ASC = GetOwningPlayerASC();
	if (!IsValid(ASC))
	{
		return;
	}

	const bool bAlreadyApplied = InfiniteModeEffectHandle.IsValid();

	if (bEnable)
	{
		// 이미 적용 중이면 중복 적용 방지
		if (bAlreadyApplied)
		{
			return;
		}

		if (!IsValid(InfiniteModeEffectClass))
		{
			return;
		}

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(InfiniteModeEffectClass, 1.f, ContextHandle);
		if (!SpecHandle.IsValid())
		{
			return;
		}

		InfiniteModeEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	else
	{
		// 적용된 상태에서만 회수
		if (!bAlreadyApplied)
		{
			return;
		}

		ASC->RemoveActiveGameplayEffect(InfiniteModeEffectHandle);
		InfiniteModeEffectHandle.Invalidate();
	}
#endif
}


/*~ 내부 헬퍼 ~*/

UPRAbilitySystemComponent* UPRCheatHandler::GetOwningPlayerASC() const
{
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	if (!IsValid(PC))
	{
		return nullptr;
	}

	APRPlayerState* PS = PC->GetPlayerState<APRPlayerState>();
	if (!IsValid(PS))
	{
		return nullptr;
	}

	return PS->GetPRAbilitySystemComponent();
}
