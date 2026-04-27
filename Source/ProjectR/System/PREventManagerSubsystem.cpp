// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PREventManagerSubsystem.h"

DEFINE_LOG_CATEGORY(LogPREvent);

void UPREventManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPREvent, Log, TEXT("[PREvent] Subsystem Initialized."));
}

void UPREventManagerSubsystem::Deinitialize()
{
	Listeners.Empty();
	Super::Deinitialize();
}

FDelegateHandle UPREventManagerSubsystem::Listen(FGameplayTag EventTag, FPREventMulticast::FDelegate Callback)
{
	if (!EventTag.IsValid())
	{
		UE_LOG(LogPREvent, Warning, TEXT("[PREvent] Listen called with invalid tag."));
		return FDelegateHandle();
	}

	FPREventMulticast& Multicast = Listeners.FindOrAdd(EventTag);
	return Multicast.Add(Callback);
}

void UPREventManagerSubsystem::Unlisten(FGameplayTag EventTag, FDelegateHandle Handle)
{
	if (FPREventMulticast* Multicast = Listeners.Find(EventTag))
	{
		Multicast->Remove(Handle);
		if (!Multicast->IsBound())
		{
			Listeners.Remove(EventTag);
		}
	}
}

void UPREventManagerSubsystem::UnlistenAll(FDelegateHandle Handle)
{
	TArray<FGameplayTag> EmptyTags;
	for (auto& Pair : Listeners)
	{
		Pair.Value.Remove(Handle);
		if (!Pair.Value.IsBound())
		{
			EmptyTags.Add(Pair.Key);
		}
	}
	for (const FGameplayTag& Tag : EmptyTags)
	{
		Listeners.Remove(Tag);
	}
}

void UPREventManagerSubsystem::Broadcast(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (!EventTag.IsValid())
	{
		UE_LOG(LogPREvent, Warning, TEXT("[PREvent] Broadcast called with invalid tag."));
		return;
	}

	if (FPREventMulticast* Multicast = Listeners.Find(EventTag))
	{
		Multicast->Broadcast(EventTag, Payload);
	}
}

void UPREventManagerSubsystem::BroadcastEmpty(FGameplayTag EventTag)
{
	static const FInstancedStruct EmptyPayload;
	Broadcast(EventTag, EmptyPayload);
}
