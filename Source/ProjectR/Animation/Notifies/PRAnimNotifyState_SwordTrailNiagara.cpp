// Copyright ProjectR. All Rights Reserved.

#include "PRAnimNotifyState_SwordTrailNiagara.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ObjectKey.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRSwordTrailNiagaraNotify, Log, All);

UPRAnimNotifyState_SwordTrailNiagara::UPRAnimNotifyState_SwordTrailNiagara()
{
}

FString UPRAnimNotifyState_SwordTrailNiagara::GetNotifyName_Implementation() const
{
	return TEXT("PR Sword Trail Niagara");
}

void UPRAnimNotifyState_SwordTrailNiagara::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	if (!ShouldRunForMesh(MeshComp) || !IsValid(TrailSystem))
	{
		return;
	}

	// 같은 메시에서 같은 Notify State가 재진입하면 이전 컴포넌트를 먼저 정리한다.
	EndTrailForMesh(MeshComp);

	FVector TrailLocation = FVector::ZeroVector;
	FRotator TrailRotation = FRotator::ZeroRotator;
	float TrailSize = 0.0f;
	if (!BuildTrailTransform(MeshComp, TrailLocation, TrailRotation, TrailSize))
	{
		return;
	}

	UNiagaraComponent* TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TrailSystem,
		MeshComp,
		NAME_None,
		TrailLocation,
		TrailRotation,
		EAttachLocation::KeepWorldPosition,
true,
false,
ENCPoolMethod::None,
false);

	if (!IsValid(TrailComponent))
	{
		UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
			TEXT("Failed to spawn sword trail Niagara system on mesh '%s'."),
			*GetNameSafe(MeshComp));
		return;
	}

	ActiveTrails.Add(FObjectKey(MeshComp), FActiveTrailData{ TrailComponent });
	UpdateTrailComponent(MeshComp, TrailComponent);
	TrailComponent->Activate(true);
}

void UPRAnimNotifyState_SwordTrailNiagara::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	if (!bUpdateEveryTick || !IsValid(MeshComp))
	{
		return;
	}

	FActiveTrailData* TrailData = ActiveTrails.Find(FObjectKey(MeshComp));
	if (!TrailData)
	{
		return;
	}

	UNiagaraComponent* TrailComponent = TrailData->Component.Get();
	if (!IsValid(TrailComponent))
	{
		ActiveTrails.Remove(FObjectKey(MeshComp));
		return;
	}

	UpdateTrailComponent(MeshComp, TrailComponent);
}

void UPRAnimNotifyState_SwordTrailNiagara::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	EndTrailForMesh(MeshComp);
}

bool UPRAnimNotifyState_SwordTrailNiagara::ShouldRunForMesh(const USkeletalMeshComponent* MeshComp) const
{
	if (!IsValid(MeshComp))
	{
		return false;
	}

	const UWorld* World = MeshComp->GetWorld();
	if (!World)
	{
		return false;
	}

	if (bSkipDedicatedServer && World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	if (!bAllowEditorPreview &&
		(World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview))
	{
		return false;
	}

	return true;
}

bool UPRAnimNotifyState_SwordTrailNiagara::BuildTrailTransform(const USkeletalMeshComponent* MeshComp,
	FVector& OutLocation,
	FRotator& OutRotation,
	float& OutTrailSize) const
{
	if (!IsValid(MeshComp))
	{
		return false;
	}

	if (!MeshComp->DoesSocketExist(BladeBottomSocketName))
	{
		UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
			TEXT("Sword trail bottom socket '%s' does not exist on mesh '%s'."),
			*BladeBottomSocketName.ToString(),
			*GetNameSafe(MeshComp));
		return false;
	}

	if (!MeshComp->DoesSocketExist(BladeTopSocketName))
	{
		UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
			TEXT("Sword trail top socket '%s' does not exist on mesh '%s'."),
			*BladeTopSocketName.ToString(),
			*GetNameSafe(MeshComp));
		return false;
	}

	const FVector BottomLocation = MeshComp->GetSocketLocation(BladeBottomSocketName);
	const FVector TopLocation = MeshComp->GetSocketLocation(BladeTopSocketName);
	const FVector BladeVector = TopLocation - BottomLocation;
	const float BladeLength = BladeVector.Size();

	if (BladeLength <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogPRSwordTrailNiagaraNotify, Warning,
			TEXT("Sword trail sockets '%s' and '%s' overlap on mesh '%s'."),
			*BladeBottomSocketName.ToString(),
			*BladeTopSocketName.ToString(),
			*GetNameSafe(MeshComp));
		return false;
	}

	OutLocation = (BottomLocation + TopLocation) * 0.5f;
	OutRotation = FRotationMatrix::MakeFromZ(BladeVector).Rotator();
	OutTrailSize = FMath::Max(0.0f, BladeLength * TrailSizeScale + TrailSizeOffset);
	return true;
}

void UPRAnimNotifyState_SwordTrailNiagara::UpdateTrailComponent(const USkeletalMeshComponent* MeshComp,
	UNiagaraComponent* TrailComponent) const
{
	if (!IsValid(MeshComp) || !IsValid(TrailComponent))
	{
		return;
	}

	FVector TrailLocation = FVector::ZeroVector;
	FRotator TrailRotation = FRotator::ZeroRotator;
	float TrailSize = 0.0f;
	if (!BuildTrailTransform(MeshComp, TrailLocation, TrailRotation, TrailSize))
	{
		return;
	}

	TrailComponent->SetWorldLocationAndRotation(TrailLocation, TrailRotation);

	if (!TrailSizeParameterName.IsNone())
	{
		TrailComponent->SetVariableFloat(TrailSizeParameterName, TrailSize);
	}

	if (bSetUserTrailSize)
	{
		TrailComponent->SetVariableFloat(FName(TEXT("User.TrailSize")), TrailSize);
	}
}

void UPRAnimNotifyState_SwordTrailNiagara::EndTrailForMesh(USkeletalMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	FActiveTrailData TrailData;
	if (!ActiveTrails.RemoveAndCopyValue(FObjectKey(MeshComp), TrailData))
	{
		return;
	}

	UNiagaraComponent* TrailComponent = TrailData.Component.Get();
	if (!IsValid(TrailComponent))
	{
		return;
	}

	if (bDestroyImmediatelyOnEnd)
	{
		TrailComponent->DestroyComponent();
		return;
	}

	if (bDeactivateOnEnd)
	{
		TrailComponent->Deactivate();
	}
}
