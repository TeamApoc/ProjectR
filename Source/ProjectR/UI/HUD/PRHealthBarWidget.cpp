// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "ProjectR/UI/HUD/PRHealthBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "TimerManager.h"

UPRHealthBarWidget::UPRHealthBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ UUserWidget Interface ~*/

void UPRHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PresentationMode == EPRHealthBarPresentationMode::LocalPlayer)
	{
		CurrentBindRetryCount = 0;
		TryBindToOwnerAbilitySystem();
	}
}

void UPRHealthBarWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	UnbindFromAbilitySystem();

	Super::NativeDestruct();
}

void UPRHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PresentationMode == EPRHealthBarPresentationMode::PartyMember || !bIsDelayedLayerActive)
	{
		return;
	}

	DelayedElapsed += InDeltaTime;
	const float Alpha = FMath::Clamp(DelayedElapsed / DelayedLayerDuration, 0.0f, 1.0f);
	DelayedPercent = FMath::Lerp(DelayedStartPercent, RecoverableEndPercent, Alpha);
	if (Alpha >= 1.0f)
	{
		DelayedPercent = RecoverableEndPercent;
		bIsDelayedLayerActive = false;
	}

	ApplyDisplayedPercentsToFill();
	BP_OnHealthLayersChanged(CurrentPercent, RecoverableEndPercent, DelayedPercent, CurrentHealth, CurrentRecoverableHealth, CurrentMaxHealth);
}

/*~ Public API ~*/

void UPRHealthBarWidget::InitializeFromAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	BindToAbilitySystem(InAbilitySystemComponent);
}

void UPRHealthBarWidget::InitializeFromPlayerState(APRPlayerState* InPlayerState)
{
	if (!IsValid(InPlayerState))
	{
		ClearHealthSource();
		return;
	}

	BindToAbilitySystem(InPlayerState->GetAbilitySystemComponent());
}

void UPRHealthBarWidget::RefreshHealthFromOwner()
{
	TryBindToOwnerAbilitySystem();
	RefreshHealthFromAbilitySystem();
}

void UPRHealthBarWidget::SetPresentationMode(EPRHealthBarPresentationMode InMode)
{
	if (PresentationMode == InMode)
	{
		return;
	}

	PresentationMode = InMode;
	if (PresentationMode == EPRHealthBarPresentationMode::PartyMember)
	{
		CurrentRecoverableHealth = 0.0f;
		RecoverableEndPercent = CurrentPercent;
		DelayedPercent = CurrentPercent;
		bIsDelayedLayerActive = false;
		RefreshLayerPercents(true);
	}
}

void UPRHealthBarWidget::ClearHealthSource()
{
	UnbindFromAbilitySystem();
	CurrentHealth = 0.0f;
	CurrentMaxHealth = 0.0f;
	CurrentRecoverableHealth = 0.0f;
	CurrentPercent = 0.0f;
	RecoverableEndPercent = 0.0f;
	DelayedPercent = 0.0f;
	bIsDelayedLayerActive = false;
	ApplyDisplayedPercentsToFill();
	BP_OnHealthLayersChanged(CurrentPercent, RecoverableEndPercent, DelayedPercent, CurrentHealth, CurrentRecoverableHealth, CurrentMaxHealth);
}

/*~ Binding ~*/

void UPRHealthBarWidget::TryBindToOwnerAbilitySystem()
{
	if (CachedAbilitySystemComponent.IsValid())
	{
		RefreshHealthFromAbilitySystem();
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	APRPlayerState* PlayerState = IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<APRPlayerState>() : nullptr;
	if (!IsValid(PlayerState))
	{
		HandleBindRetryTimer();
		return;
	}

	BindToAbilitySystem(PlayerState->GetAbilitySystemComponent());
}

void UPRHealthBarWidget::BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (!IsValid(InAbilitySystemComponent))
	{
		return;
	}

	if (CachedAbilitySystemComponent.Get() == InAbilitySystemComponent
		&& HealthChangedDelegateHandle.IsValid()
		&& MaxHealthChangedDelegateHandle.IsValid())
	{
		RefreshHealthFromAbilitySystem();
		return;
	}

	UnbindFromAbilitySystem();

	CachedAbilitySystemComponent = InAbilitySystemComponent;
	HealthChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Common::GetHealthAttribute()).AddUObject(this, &UPRHealthBarWidget::HandleHealthChanged);
	MaxHealthChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UPRAttributeSet_Common::GetMaxHealthAttribute()).AddUObject(this, &UPRHealthBarWidget::HandleMaxHealthChanged);

	if (PresentationMode == EPRHealthBarPresentationMode::LocalPlayer)
	{
		RecoverableHealthChangedDelegateHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UPRAttributeSet_Player::GetRecoverableHealthAttribute()).AddUObject(this, &UPRHealthBarWidget::HandleRecoverableHealthChanged);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	RefreshHealthFromAbilitySystem();
}

void UPRHealthBarWidget::UnbindFromAbilitySystem()
{
	UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	if (IsValid(AbilitySystemComponent))
	{
		if (HealthChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UPRAttributeSet_Common::GetHealthAttribute()).Remove(HealthChangedDelegateHandle);
		}

		if (MaxHealthChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UPRAttributeSet_Common::GetMaxHealthAttribute()).Remove(MaxHealthChangedDelegateHandle);
		}

		if (RecoverableHealthChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UPRAttributeSet_Player::GetRecoverableHealthAttribute()).Remove(RecoverableHealthChangedDelegateHandle);
		}
	}

	HealthChangedDelegateHandle.Reset();
	MaxHealthChangedDelegateHandle.Reset();
	RecoverableHealthChangedDelegateHandle.Reset();
	CachedAbilitySystemComponent.Reset();
}

void UPRHealthBarWidget::RefreshHealthFromAbilitySystem()
{
	UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetHealthAttribute());
	CurrentMaxHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetMaxHealthAttribute());
	CurrentRecoverableHealth = PresentationMode == EPRHealthBarPresentationMode::LocalPlayer
		? AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Player::GetRecoverableHealthAttribute())
		: 0.0f;

	RefreshLayerPercents(true);
}

void UPRHealthBarWidget::RefreshLayerPercents(bool bForceDelayedPercent)
{
	CurrentRecoverableHealth = FMath::Clamp(CurrentRecoverableHealth, 0.0f, FMath::Max(CurrentMaxHealth - CurrentHealth, 0.0f));
	CurrentPercent = CurrentMaxHealth > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentHealth / CurrentMaxHealth, 0.0f, 1.0f)
		: 0.0f;
	RecoverableEndPercent = CurrentMaxHealth > KINDA_SMALL_NUMBER
		? FMath::Clamp((CurrentHealth + CurrentRecoverableHealth) / CurrentMaxHealth, 0.0f, 1.0f)
		: 0.0f;

	if (PresentationMode == EPRHealthBarPresentationMode::PartyMember)
	{
		CurrentRecoverableHealth = 0.0f;
		RecoverableEndPercent = CurrentPercent;
		DelayedPercent = RecoverableEndPercent;
		bIsDelayedLayerActive = false;
	}
	else if (bForceDelayedPercent)
	{
		DelayedPercent = RecoverableEndPercent;
		bIsDelayedLayerActive = false;
	}
	else if (!bIsDelayedLayerActive)
	{
		DelayedPercent = RecoverableEndPercent;
	}

	const bool bNewCriticalHealth = CurrentPercent <= CriticalHealthThreshold;
	if (bIsCriticalHealth != bNewCriticalHealth)
	{
		bIsCriticalHealth = bNewCriticalHealth;
		BP_OnHealthCriticalChanged(bIsCriticalHealth);
	}

	ApplyDisplayedPercentsToFill();
	BP_OnHealthLayersChanged(CurrentPercent, RecoverableEndPercent, DelayedPercent, CurrentHealth, CurrentRecoverableHealth, CurrentMaxHealth);
}

void UPRHealthBarWidget::ApplyDisplayedPercentsToFill()
{
	ApplyBarLayoutWidths();

	const float MaxFillWidth = GetMaxFillWidth();
	const float RecoverableFillPercent = PresentationMode == EPRHealthBarPresentationMode::PartyMember
		? 0.0f
		: RecoverableEndPercent;
	const float DelayedFillPercent = PresentationMode == EPRHealthBarPresentationMode::PartyMember
		? 0.0f
		: DelayedPercent;

	if (USizeBox* FillSizeBox = GetCurrentFillSizeBox())
	{
		FillSizeBox->SetWidthOverride(MaxFillWidth * CurrentPercent);
	}

	if (USizeBox* FillSizeBox = GetRecoverableFillSizeBox())
	{
		FillSizeBox->SetWidthOverride(MaxFillWidth * RecoverableFillPercent);
	}

	if (USizeBox* FillSizeBox = GetDelayedFillSizeBox())
	{
		FillSizeBox->SetWidthOverride(MaxFillWidth * DelayedFillPercent);
	}
}

void UPRHealthBarWidget::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	const float OldHealth = CurrentHealth;
	CurrentHealth = ChangeData.NewValue;
	if (PresentationMode == EPRHealthBarPresentationMode::LocalPlayer
		&& ChangeData.NewValue < ChangeData.OldValue)
	{
		const float DamageStartPercent = CurrentMaxHealth > KINDA_SMALL_NUMBER
			? FMath::Clamp(ChangeData.OldValue / CurrentMaxHealth, 0.0f, 1.0f)
			: CurrentPercent;
		StartDelayedLayer(FMath::Max(DelayedPercent, DamageStartPercent));
	}
	else if (ChangeData.NewValue > OldHealth)
	{
		if (PresentationMode == EPRHealthBarPresentationMode::LocalPlayer)
		{
			const float HealAmount = FMath::Max(ChangeData.NewValue - OldHealth, 0.0f);
			CurrentRecoverableHealth = FMath::Max(CurrentRecoverableHealth - HealAmount, 0.0f);
		}

		RefreshLayerPercents(true);
		bIsDelayedLayerActive = false;
		return;
	}

	RefreshLayerPercents(false);
}

void UPRHealthBarWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentMaxHealth = ChangeData.NewValue;
	RefreshLayerPercents(true);
}

void UPRHealthBarWidget::HandleRecoverableHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentRecoverableHealth = ChangeData.NewValue;
	RefreshLayerPercents(false);
}

void UPRHealthBarWidget::HandleBindRetryTimer()
{
	if (CurrentBindRetryCount >= MaxBindRetryCount)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(BindRetryTimerHandle))
	{
		return;
	}

	++CurrentBindRetryCount;
	World->GetTimerManager().SetTimer(
		BindRetryTimerHandle,
		this,
		&UPRHealthBarWidget::TryBindToOwnerAbilitySystem,
		BindRetryInterval,
		false);
}

void UPRHealthBarWidget::StartDelayedLayer(float InStartPercent)
{
	DelayedStartPercent = FMath::Clamp(InStartPercent, 0.0f, 1.0f);
	DelayedPercent = DelayedStartPercent;
	DelayedElapsed = 0.0f;
	bIsDelayedLayerActive = true;
}

void UPRHealthBarWidget::ApplyBarLayoutWidths()
{
	if (IsValid(BackBorderSizeBox))
	{
		BackBorderSizeBox->SetWidthOverride(GetBackBorderWidth());
	}

	if (IsValid(FillAreaSizeBox))
	{
		FillAreaSizeBox->SetWidthOverride(GetFillAreaWidth());
	}
}

float UPRHealthBarWidget::GetBackBorderWidth() const
{
	return PresentationMode == EPRHealthBarPresentationMode::PartyMember ? PartyMemberMaxBarWidth : MaxBarWidth;
}

float UPRHealthBarWidget::GetFillAreaWidth() const
{
	const float BackBorderWidth = GetBackBorderWidth();
	if (!IsValid(FillAreaSizeBox))
	{
		return BackBorderWidth;
	}

	float HorizontalPadding = 0.0f;
	if (const UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(FillAreaSizeBox->Slot))
	{
		const FMargin SlotPadding = OverlaySlot->GetPadding();
		HorizontalPadding = SlotPadding.Left + SlotPadding.Right;
	}

	const float FillAreaWidth = BackBorderWidth - HorizontalPadding;
	return FillAreaWidth > 0.0f ? FillAreaWidth : BackBorderWidth;
}

float UPRHealthBarWidget::GetMaxFillWidth() const
{
	if (IsValid(FillAreaSizeBox) && FillAreaSizeBox->IsWidthOverride())
	{
		const float FillWidth = FillAreaSizeBox->GetWidthOverride();
		if (FillWidth > 0.0f)
		{
			return FillWidth;
		}
	}

	return GetFillAreaWidth();
}

USizeBox* UPRHealthBarWidget::GetCurrentFillSizeBox() const
{
	if (IsValid(HealthFillSizeBox))
	{
		return HealthFillSizeBox;
	}

	if (IsValid(SizeBox_FillClip))
	{
		return SizeBox_FillClip;
	}

	if (IsValid(FillClipBox))
	{
		return FillClipBox;
	}

	return nullptr;
}

USizeBox* UPRHealthBarWidget::GetRecoverableFillSizeBox() const
{
	return IsValid(RecoverableFillSizeBox) ? RecoverableFillSizeBox.Get() : nullptr;
}

USizeBox* UPRHealthBarWidget::GetDelayedFillSizeBox() const
{
	return IsValid(DelayedFillSizeBox) ? DelayedFillSizeBox.Get() : nullptr;
}
