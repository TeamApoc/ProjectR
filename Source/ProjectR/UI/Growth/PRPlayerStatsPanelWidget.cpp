// Copyright ProjectR. All Rights Reserved.

#include "PRPlayerStatsPanelWidget.h"

#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Growth.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Player/Components/PRPlayerGrowthComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PRDeveloperSettings.h"

/*~ UUserWidget Interface ~*/

void UPRPlayerStatsPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// BindWidget 포인터 배열화
	CacheTextBindings();
	if (!BoundPlayerState.IsValid())
	{
		APlayerController* OwningPlayerController = GetOwningPlayer();
		SetPlayerStateSource(IsValid(OwningPlayerController) ? OwningPlayerController->GetPlayerState<APRPlayerState>() : nullptr);
		return;
	}

	BindGrowthComponent();
	RefreshStatsPanelView();
}

void UPRPlayerStatsPanelWidget::NativeDestruct()
{
	// 성장 컴포넌트 이벤트 정리
	UnbindGrowthComponent();

	Super::NativeDestruct();
}

/*~ 표시 소스 ~*/

void UPRPlayerStatsPanelWidget::SetPlayerStateSource(APRPlayerState* InPlayerState)
{
	// 기존 런타임 소스 바인딩 해제
	UnbindGrowthComponent();

	BoundPlayerState = InPlayerState;
	BoundGrowthComponent = IsValid(InPlayerState) ? InPlayerState->GetGrowthComponent() : nullptr;

	BindGrowthComponent();
	RefreshStatsPanelView();
}

void UPRPlayerStatsPanelWidget::RefreshStatsPanelView()
{
	CurrentViewData = FPRPlayerStatsPanelViewData();
	BuildPlayerStateViewData(BoundPlayerState.Get(), CurrentViewData);
	ApplyViewDataToWidgets();
}

void UPRPlayerStatsPanelWidget::HandleTraitInvestmentChanged(const FPRTraitInvestmentInfo& InvestmentInfo)
{
	// 특성 변경 후 Attribute 반영값 재조회
	static_cast<void>(InvestmentInfo);
	RefreshStatsPanelView();
}

/*~ 내부 바인딩 ~*/

void UPRPlayerStatsPanelWidget::BindGrowthComponent()
{
	if (UPRPlayerGrowthComponent* GrowthComponent = BoundGrowthComponent.Get())
	{
		GrowthComponent->OnTraitInvestmentChanged.RemoveDynamic(this, &ThisClass::HandleTraitInvestmentChanged);
		GrowthComponent->OnTraitInvestmentChanged.AddDynamic(this, &ThisClass::HandleTraitInvestmentChanged);
	}
}

void UPRPlayerStatsPanelWidget::UnbindGrowthComponent()
{
	if (UPRPlayerGrowthComponent* GrowthComponent = BoundGrowthComponent.Get())
	{
		GrowthComponent->OnTraitInvestmentChanged.RemoveDynamic(this, &ThisClass::HandleTraitInvestmentChanged);
	}
}

void UPRPlayerStatsPanelWidget::CacheTextBindings()
{
	// 특성 행 위젯 포인터 단일 순회용 캐시
	TextBindings.Reset();
	TextBindings.Add({ EPRTraitStatType::MaxHealth, MaxHealthLabelText.Get(), MaxHealthValueText.Get() });
	TextBindings.Add({ EPRTraitStatType::Armor, ArmorLabelText.Get(), ArmorValueText.Get() });
	TextBindings.Add({ EPRTraitStatType::MovementSpeed, MovementSpeedLabelText.Get(), MovementSpeedValueText.Get() });
	TextBindings.Add({ EPRTraitStatType::AttackPower, AttackPowerLabelText.Get(), AttackPowerValueText.Get() });
	TextBindings.Add({ EPRTraitStatType::MaxStamina, MaxStaminaLabelText.Get(), MaxStaminaValueText.Get() });
	TextBindings.Add({ EPRTraitStatType::CriticalHitChance, CriticalHitChanceLabelText.Get(), CriticalHitChanceValueText.Get() });
	TextBindings.Add({ EPRTraitStatType::CriticalDamageMultiplier, CriticalDamageMultiplierLabelText.Get(), CriticalDamageMultiplierValueText.Get() });
}

void UPRPlayerStatsPanelWidget::ApplyViewDataToWidgets()
{
	// 상단 성장 요약 텍스트 반영
	if (IsValid(LevelText))
	{
		LevelText->SetText(FText::AsNumber(CurrentViewData.Level));
	}

	// 특성 행 텍스트 반복 반영
	for (const FPRStatsPanelTextBinding& TextBinding : TextBindings)
	{
		const FPRPlayerStatsPanelRowViewData* RowViewData = FindRowViewData(TextBinding.TraitType);
		if (RowViewData == nullptr)
		{
			continue;
		}

		if (IsValid(TextBinding.LabelText))
		{
			TextBinding.LabelText->SetText(RowViewData->DisplayName);
		}

		if (IsValid(TextBinding.ValueText))
		{
			TextBinding.ValueText->SetText(MakeTraitValueText(RowViewData->TraitType, RowViewData->StatValue));
		}
	}
}

/*~ 표시 데이터 구성 ~*/

void UPRPlayerStatsPanelWidget::BuildPlayerStateViewData(APRPlayerState* InPlayerState, FPRPlayerStatsPanelViewData& OutViewData) const
{
	// 런타임 ASC 기반 표시 데이터 구성
	const UPRAbilitySystemComponent* AbilitySystemComponent = IsValid(InPlayerState) ? InPlayerState->GetPRAbilitySystemComponent() : nullptr;
	OutViewData.Level = IsValid(AbilitySystemComponent)
		? FMath::Max(FMath::RoundToInt(AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Growth::GetLevelAttribute())), 1)
		: (IsValid(InPlayerState) ? InPlayerState->GetCharacterLevel() : 1);

	for (EPRTraitStatType TraitType : GetDisplayTraitTypes())
	{
		AddTraitRowFromPlayerState(InPlayerState, TraitType, OutViewData);
	}
}

void UPRPlayerStatsPanelWidget::AddTraitRowFromPlayerState(APRPlayerState* InPlayerState, EPRTraitStatType TraitType, FPRPlayerStatsPanelViewData& OutViewData) const
{
	// 특성 규칙의 TargetAttribute 기준 실제 ASC 값 조회
	const UPRAbilitySystemComponent* AbilitySystemComponent = IsValid(InPlayerState) ? InPlayerState->GetPRAbilitySystemComponent() : nullptr;
	FPRTraitStatRuleRow Rule;

	FPRPlayerStatsPanelRowViewData RowViewData;
	RowViewData.TraitType = TraitType;
	RowViewData.DisplayName = GetTraitDisplayName(TraitType);
	RowViewData.StatValue = IsValid(AbilitySystemComponent) && FindTraitRule(TraitType, Rule) && Rule.TargetAttribute.IsValid()
		? AbilitySystemComponent->GetNumericAttribute(Rule.TargetAttribute)
		: 0.0f;
	OutViewData.StatRows.Add(RowViewData);
}

/*~ 데이터 조회 ~*/

bool UPRPlayerStatsPanelWidget::FindTraitRule(EPRTraitStatType TraitType, FPRTraitStatRuleRow& OutRule) const
{
	UDataTable* RuleTable = ResolveTraitRuleTable();
	if (!IsValid(RuleTable))
	{
		return false;
	}

	TArray<FPRTraitStatRuleRow*> Rows;
	RuleTable->GetAllRows(TEXT("UPRPlayerStatsPanelWidget::FindTraitRule"), Rows);
	for (const FPRTraitStatRuleRow* Row : Rows)
	{
		if (Row != nullptr && Row->TraitType == TraitType)
		{
			OutRule = *Row;
			return true;
		}
	}
	return false;
}

UDataTable* UPRPlayerStatsPanelWidget::ResolveTraitRuleTable() const
{
	// 프로젝트 설정 특성 규칙 테이블
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	return Settings != nullptr ? Settings->TraitStatRuleTable.LoadSynchronous() : nullptr;
}

FText UPRPlayerStatsPanelWidget::GetTraitDisplayName(EPRTraitStatType TraitType) const
{
	switch (TraitType)
	{
	case EPRTraitStatType::MaxHealth:
		return FText::FromString(TEXT("최대 체력"));
	case EPRTraitStatType::Armor:
		return FText::FromString(TEXT("방어력"));
	case EPRTraitStatType::MovementSpeed:
		return FText::FromString(TEXT("이동 속도"));
	case EPRTraitStatType::AttackPower:
		return FText::FromString(TEXT("공격력 보너스"));
	case EPRTraitStatType::MaxStamina:
		return FText::FromString(TEXT("최대 스태미너"));
	case EPRTraitStatType::CriticalHitChance:
		return FText::FromString(TEXT("치명타 확률"));
	case EPRTraitStatType::CriticalDamageMultiplier:
		return FText::FromString(TEXT("치명타 피해"));
	default:
		return FText::FromString(TEXT("알 수 없음"));
	}
}

FText UPRPlayerStatsPanelWidget::MakeTraitValueText(EPRTraitStatType TraitType, float Value) const
{
	if (TraitType == EPRTraitStatType::MovementSpeed
		|| TraitType == EPRTraitStatType::CriticalHitChance
		|| TraitType == EPRTraitStatType::CriticalDamageMultiplier)
	{
		return FText::FromString(FString::Printf(TEXT("%.1f%%"), Value * 100.0f));
	}

	return FText::FromString(FString::Printf(TEXT("%.1f"), Value));
}

const FPRPlayerStatsPanelRowViewData* UPRPlayerStatsPanelWidget::FindRowViewData(EPRTraitStatType TraitType) const
{
	for (const FPRPlayerStatsPanelRowViewData& RowViewData : CurrentViewData.StatRows)
	{
		if (RowViewData.TraitType == TraitType)
		{
			return &RowViewData;
		}
	}
	return nullptr;
}

TArray<EPRTraitStatType> UPRPlayerStatsPanelWidget::GetDisplayTraitTypes() const
{
	return
	{
		EPRTraitStatType::MaxHealth,
		EPRTraitStatType::Armor,
		EPRTraitStatType::MovementSpeed,
		EPRTraitStatType::AttackPower,
		EPRTraitStatType::MaxStamina,
		EPRTraitStatType::CriticalHitChance,
		EPRTraitStatType::CriticalDamageMultiplier
	};
}
