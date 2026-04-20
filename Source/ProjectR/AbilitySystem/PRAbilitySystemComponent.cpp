// Copyright ProjectR. All Rights Reserved.

#include "PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/PRGameplayTags.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "Data/PRAbilitySet.h"
#include "Data/PRAbilitySystemRegistry.h"

// =====  UActorComponent Interface =====

void UPRAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	// GAS의 어빌리티 종료 멀티캐스트 델리게이트에 바인딩. OnAbilityEnded 브로드캐스트 경로
	// TODO: 어빌리티 발동 / 종료 이벤트 래퍼 델리게이트 발행
	//AbilityEndedCallbacks.AddUObject(this, &UPRAbilitySystemComponent::HandleAbilityEnded);
	//AbilityActivatedCallbacks.AddUObject(this, &UPRAbilitySystemComponent::HandleAbilityActivated);
}

// =====  UAbilitySystemComponent Interface =====

void UPRAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	// ActivationPolicy == OnGiven 이면 부여 즉시 활성화
	const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(AbilitySpec.Ability);
	if (IsValid(CDO) && CDO->GetActivationPolicy() == EPRAbilityActivationPolicy::OnGiven)
	{
		TryActivateAbility(AbilitySpec.Handle);
	}

	// 어빌리티 종료 통지 수신을 위해 Spec의 OnAbilityEnded에 바인딩은 개별 인스턴스에서 처리.
	// 여기서는 ASC의 범용 종료 콜백만 등록
	if (IsValid(AbilitySpec.Ability))
	{
		// GAS는 FAbilityEndedData 콜백을 ASC OnAbilityEnded 델리게이트로 제공
	}
}

void UPRAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (!Spec.IsActive() || Spec.Ability == nullptr)
	{
		return;
	}

	// WaitInputPressed Task에 이벤트 전달을 위해 InvokeReplicatedEvent 사용
	const EGameplayAbilityInstancingPolicy::Type InstancingPolicy = Spec.Ability->GetInstancingPolicy();
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::Type::InstancedPerActor)
	{
		if (UGameplayAbility* Inst = Spec.GetPrimaryInstance())
		{
			const FGameplayAbilityActivationInfo Info = Inst->GetCurrentActivationInfo();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Info.GetActivationPredictionKey());
		}
	}
	else
	{
		for (UGameplayAbility* Inst : Spec.GetAbilityInstances())
		{
			const FGameplayAbilityActivationInfo Info = Inst->GetCurrentActivationInfo();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Info.GetActivationPredictionKey());
		}
	}
}

void UPRAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (!Spec.IsActive() || Spec.Ability == nullptr)
	{
		return;
	}

	const EGameplayAbilityInstancingPolicy::Type InstancingPolicy = Spec.Ability->GetInstancingPolicy();
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::Type::InstancedPerActor)
	{
		if (UGameplayAbility* Inst = Spec.GetPrimaryInstance())
		{
			const FGameplayAbilityActivationInfo Info = Inst->GetCurrentActivationInfo();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Info.GetActivationPredictionKey());
		}
	}
	else
	{
		for (UGameplayAbility* Inst : Spec.GetAbilityInstances())
		{
			const FGameplayAbilityActivationInfo Info = Inst->GetCurrentActivationInfo();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Info.GetActivationPredictionKey());
		}
	}
}

// =====  AbilitySet 부여/해제 =====

void UPRAbilitySystemComponent::GiveAbilitySet(const UPRAbilitySet* AbilitySet, FPRAbilitySetHandles& OutHandles)
{
	if (!IsValid(AbilitySet) || !IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FPRAbilityEntry& Entry : AbilitySet->Abilities)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		FGameplayAbilitySpec Spec(Entry.AbilityClass, Entry.Level);
		Spec.GetDynamicSpecSourceTags().AppendTags(Entry.DynamicTags);

		// InputTag가 CDO에 설정되어 있으면 DynamicTags에 자동 주입 (AbilitySet이 누락해도 매칭되도록)
		if (const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(Entry.AbilityClass->GetDefaultObject()))
		{
			const FGameplayTag& CDOInputTag = CDO->GetInputTag();
			if (CDOInputTag.IsValid() && !Spec.GetDynamicSpecSourceTags().HasTagExact(CDOInputTag))
			{
				Spec.GetDynamicSpecSourceTags().AddTag(CDOInputTag);
			}
		}

		const FGameplayAbilitySpecHandle Handle = GiveAbility(Spec);
		OutHandles.AbilityHandles.Add(Handle);
	}

	for (const FPREffectEntry& Entry : AbilitySet->Effects)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		FGameplayEffectContextHandle Context = MakeEffectContext();
		Context.AddSourceObject(AbilitySet);
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(Entry.EffectClass, Entry.Level, Context);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->DynamicGrantedTags.AppendTags(Entry.DynamicTags);
			const FActiveGameplayEffectHandle ActiveHandle = ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
			OutHandles.EffectHandles.Add(ActiveHandle);
		}
	}
}

void UPRAbilitySystemComponent::ClearAbilitySetByHandles(FPRAbilitySetHandles& Handles)
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FActiveGameplayEffectHandle& Handle : Handles.EffectHandles)
	{
		if (Handle.IsValid())
		{
			RemoveActiveGameplayEffect(Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : Handles.AbilityHandles)
	{
		if (Handle.IsValid())
		{
			ClearAbility(Handle);
		}
	}

	Handles.Reset();
}

// =====  속성 초기화 =====

bool UPRAbilitySystemComponent::InitializeAttributesFromRegistry(const UPRAbilitySystemRegistry* Registry,
                                                                   EPRCharacterRole Role, FName RowName)
{
	if (!IsValid(Registry) || !IsOwnerActorAuthoritative())
	{
		return false;
	}

	UDataTable* DT = Registry->GetStatTableSynchronous(Role);
	if (!IsValid(DT))
	{
		return false;
	}

	const UScriptStruct* RowStruct = DT->GetRowStruct();
	if (RowStruct == nullptr)
	{
		return false;
	}

	uint8* RowData = DT->FindRowUnchecked(RowName);
	if (RowData == nullptr)
	{
		return false;
	}

	// Row의 모든 FFloatProperty를 순회. 프로퍼티명을 Registry로 조회하여 Attribute 주입
	for (TFieldIterator<FFloatProperty> It(RowStruct); It; ++It)
	{
		const FFloatProperty* Prop = *It;
		const FName PropName = Prop->GetFName();
		const float Value = Prop->GetPropertyValue_InContainer(RowData);

		const FGameplayAttribute Attr = Registry->FindAttribute(PropName);
		if (!Attr.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Registry missing mapping for property '%s'"), *PropName.ToString());
			continue;
		}

		SetNumericAttributeBase(Attr, Value);
	}
	
	// 초기화 GE 적용
	if (TSubclassOf<UGameplayEffect> InitializeGE = Registry->GetInitializeGESynchronous())
	{
		FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(InitializeGE, 1.0f, EffectContextHandle);
		ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	return true;
}

// =====  입력 라우팅 =====

void UPRAbilitySystemComponent::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.AddUnique(Spec.Handle);
		}
	}
}

void UPRAbilitySystemComponent::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.Remove(Spec.Handle);
		}
	}
}

void UPRAbilitySystemComponent::ProcessAbilityInput(float /*DeltaTime*/, bool /*bGamePaused*/)
{
	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	// WhileInputHeld: 입력 유지 중, 비활성인 Spec만 후보
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability && !Spec->IsActive())
			{
				const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(Spec->Ability);
				if (IsValid(CDO) && CDO->GetActivationPolicy() == EPRAbilityActivationPolicy::WhileInputHeld)
				{
					AbilitiesToActivate.AddUnique(SpecHandle);
				}
			}
		}
	}

	// OnInputTriggered: 이번 프레임 Pressed
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability)
			{
				Spec->InputPressed = true;

				if (Spec->IsActive())
				{
					AbilitySpecInputPressed(*Spec);
				}
				else
				{
					const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(Spec->Ability);
					if (IsValid(CDO) && CDO->GetActivationPolicy() == EPRAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(SpecHandle);
					}
				}
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(SpecHandle);
	}

	// Released
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability)
			{
				Spec->InputPressed = false;
				if (Spec->IsActive())
				{
					AbilitySpecInputReleased(*Spec);
				}
			}
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UPRAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

// =====  AI / 서버 호출 경로 =====

bool UPRAbilitySystemComponent::TryActivateAbilityOnServer(const FGameplayTag& AbilityTag,
                                                             FGameplayAbilitySpecHandle& OutHandle)
{
	if (!AbilityTag.IsValid() || !IsOwnerActorAuthoritative())
	{
		return false;
	}

	FGameplayTagContainer Query;
	Query.AddTag(AbilityTag);

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(Query, MatchingSpecs);

	for (FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec && TryActivateAbility(Spec->Handle))
		{
			OutHandle = Spec->Handle;
			return true;
		}
	}

	return false;
}

bool UPRAbilitySystemComponent::CanActivateAbilityByTag(const FGameplayTag& AbilityTag,
                                                         FGameplayTagContainer& OutFailureTags) const
{
	if (!AbilityTag.IsValid())
	{
		OutFailureTags.AddTag(PRGameplayTags::Fail_Invalid);
		return false;
	}

	FGameplayTagContainer Query;
	Query.AddTag(AbilityTag);

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	const_cast<UPRAbilitySystemComponent*>(this)->GetActivatableGameplayAbilitySpecsByAllMatchingTags(Query, MatchingSpecs);

	for (FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec == nullptr || Spec->Ability == nullptr)
		{
			continue;
		}
		FGameplayTagContainer FailureTags;
		if (Spec->Ability->CanActivateAbility(Spec->Handle, AbilityActorInfo.Get(), nullptr, nullptr, &FailureTags))
		{
			return true;
		}
		OutFailureTags.AppendTags(FailureTags);
	}

	if (OutFailureTags.IsEmpty())
	{
		OutFailureTags.AddTag(PRGameplayTags::Fail_Invalid);
	}
	return false;
}

// =====  내부 =====

void UPRAbilitySystemComponent::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (EndedData.AbilityThatEnded == nullptr)
	{
		return;
	}
	const FGameplayTagContainer& Tags = EndedData.AbilityThatEnded->AbilityTags;
	const FGameplayTag PrimaryTag = Tags.IsEmpty() ? FGameplayTag() : Tags.First();
	OnAbilityEnded.Broadcast(PrimaryTag, EndedData.bWasCancelled);
}

void UPRAbilitySystemComponent::HandleAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	
}
