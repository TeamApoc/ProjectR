// Copyright ProjectR. All Rights Reserved.

#include "PRAbilitySystemRegistry.h"

#include "PRStatRows.h"
#include "GameplayEffect.h"


#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

// =====  조회 =====

FGameplayAttribute UPRAbilitySystemRegistry::FindAttribute(FName PropertyName) const
{
	if (const FGameplayAttribute* Found = PropertyToAttribute.Find(PropertyName))
	{
		return *Found;
	}
	return FGameplayAttribute();
}

UDataTable* UPRAbilitySystemRegistry::GetStatTableSynchronous(EPRCharacterRole Role) const
{
	if (const TSoftObjectPtr<UDataTable>* SoftPtr = StatTables.Find(Role))
	{
		if (!SoftPtr->IsNull())
		{
			return SoftPtr->LoadSynchronous();
		}
	}
	return nullptr;
}

TSubclassOf<UGameplayEffect> UPRAbilitySystemRegistry::GetInitializeGESynchronous() const
{
	if (!InitializeGE.IsNull())
	{
		return InitializeGE.LoadSynchronous();
	}
	return nullptr;
}

// =====  Data Validation =====

#if WITH_EDITOR
EDataValidationResult UPRAbilitySystemRegistry::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// 1) StatTables 엔트리 존재 여부
	const UEnum* RoleEnum = StaticEnum<EPRCharacterRole>();
	if (IsValid(RoleEnum))
	{
		for (int32 i = 0; i < RoleEnum->NumEnums() - 1; ++i)
		{
			const EPRCharacterRole Role = static_cast<EPRCharacterRole>(RoleEnum->GetValueByIndex(i));
			if (!StatTables.Contains(Role))
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("StatTables missing entry for Role %s"),
						*RoleEnum->GetNameStringByIndex(i))));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	// 2) 각 DT 로드 + 3) RowStruct 일치 + 4) 매핑 커버리지 + 5) Attribute 유효성
	for (const TPair<EPRCharacterRole, TSoftObjectPtr<UDataTable>>& Pair : StatTables)
	{
		UDataTable* DT = Pair.Value.LoadSynchronous();
		if (!IsValid(DT))
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("StatTable for Role %d failed to load"),
					static_cast<int32>(Pair.Key))));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		const UScriptStruct* ExpectedRowStruct = (Pair.Key == EPRCharacterRole::Player)
			? FPRPlayerStatRow::StaticStruct()
			: FPREnemyStatRow::StaticStruct();
		if (DT->GetRowStruct() != ExpectedRowStruct)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("StatTable %s RowStruct mismatch (expected %s)"),
					*DT->GetName(), *ExpectedRowStruct->GetName())));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		// 매핑 커버리지·Attribute 유효성
		for (TFieldIterator<FFloatProperty> It(ExpectedRowStruct); It; ++It)
		{
			const FName PropName = It->GetFName();
			const FGameplayAttribute* Found = PropertyToAttribute.Find(PropName);
			if (Found == nullptr)
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("PropertyToAttribute missing key '%s' for %s"),
						*PropName.ToString(), *DT->GetName())));
				Result = EDataValidationResult::Invalid;
			}
			else if (!Found->IsValid())
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("PropertyToAttribute['%s'] is invalid FGameplayAttribute"),
						*PropName.ToString())));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}
#endif
