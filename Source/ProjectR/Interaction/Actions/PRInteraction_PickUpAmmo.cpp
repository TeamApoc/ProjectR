// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteraction_PickUpAmmo.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/World/Pickable/PRPickableAmmo.h"

void UPRInteraction_PickUpAmmo::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	APRPickableAmmo* PickableAmmo = Cast<APRPickableAmmo>(GetOwner());
	if (!IsValid(PickableAmmo) || !PickableAmmo->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor);
	if (!IsValid(TargetASC))
	{
		return;
	}

	const float Remaining = PickableAmmo->GrantAmmoAndGetRemaining(TargetASC);

	// 전부 적립된 경우만 픽업 액터 제거. 일부만 흡수되면 잔여량을 갱신해 월드에 남긴다
	if (Remaining <= 0.f)
	{
		PickableAmmo->Destroy();
	}
	else
	{
		PickableAmmo->SetAmmo(PickableAmmo->GetAmmoType(), Remaining);
	}
}
