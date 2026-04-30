// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRFloatingTextManager.h"
#include "PRFloatingTextWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/System/PRDeveloperSettings.h"

// ===== 생성자 =====

UPRFloatingTextManager::UPRFloatingTextManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// ===== UActorComponent Interface =====

void UPRFloatingTextManager::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 클라이언트에서만 풀을 생성한다 (위젯은 로컬 전용)
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!IsValid(PC) || !PC->IsLocalController())
	{
		return;
	}

	// WidgetComponent를 소유할 로컬 전용 액터 스폰. PC의 렌더링 속성 영향을 피한다
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;
	PoolActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (IsValid(PoolActor))
	{
		PoolActor->SetReplicates(false);
	}

	// 초기 풀 사이즈만큼 WidgetComponent를 미리 생성
	for (int32 i = 0; i < InitialPoolSize; ++i)
	{
		if (UWidgetComponent* NewComp = AcquireWidgetComponent())
		{
			ReleaseWidgetComponent(NewComp);
		}
	}
}

void UPRFloatingTextManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AvailablePool.Empty();
	ActiveWidgets.Empty();
	WidgetToComponentMap.Empty();

	if (IsValid(PoolActor))
	{
		PoolActor->Destroy();
		PoolActor = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// ===== 플로팅 텍스트 표시 =====

void UPRFloatingTextManager::ClientShowFloatingText_Reliable_Implementation(const FPRFloatingTextRequest& Request)
{
	ShowFloatingText(Request);
}

void UPRFloatingTextManager::ClientShowFloatingText_Unreliable_Implementation(const FPRFloatingTextRequest& Request)
{
	ShowFloatingText(Request);
}

void UPRFloatingTextManager::ShowFloatingText(const FPRFloatingTextRequest& Request)
{
	// 활성 텍스트가 최대 수에 도달했으면 가장 오래된 것을 제거
	ForceRetireOldest();

	UWidgetComponent* WidgetComp = AcquireWidgetComponent();
	if (!IsValid(WidgetComp))
	{
		return;
	}

	// 타입에 맞는 스타일 조회
	FPRFloatingTextStyle Style = ResolveStyle(Request.TextType);
	if (!IsValid(Style.WidgetClass))
	{
		ReleaseWidgetComponent(WidgetComp);
		return;
	}

	// 기존 위젯의 클래스가 동일하면 재사용, 다르면 재생성
	UPRFloatingTextWidget* FloatingWidget = Cast<UPRFloatingTextWidget>(WidgetComp->GetWidget());
	if (!IsValid(FloatingWidget) || FloatingWidget->GetClass() != Style.WidgetClass)
	{
		WidgetComp->SetWidgetClass(Style.WidgetClass);
	}
	
	WidgetComp->InitWidget();
	FloatingWidget = Cast<UPRFloatingTextWidget>(WidgetComp->GetWidget());

	if (!IsValid(FloatingWidget))
	{
		ReleaseWidgetComponent(WidgetComp);
		return;
	}

	// WidgetComponent 위치/가시성 설정 (랜덤 오프셋 적용)
	const FVector RandomOffset(
		FMath::FRandRange(-RandomOffsetRange.X, RandomOffsetRange.X),
		FMath::FRandRange(-RandomOffsetRange.Y, RandomOffsetRange.Y),
		FMath::FRandRange(-RandomOffsetRange.Z, RandomOffsetRange.Z)
	);
	WidgetComp->SetWorldLocation(Request.WorldLocation + RandomOffset);
	WidgetComp->SetVisibility(true);

	// 위젯-컴포넌트 매핑 등록
	WidgetToComponentMap.Add(FloatingWidget, WidgetComp);

	// 연출 완료 콜백 바인딩
	FloatingWidget->OnFloatingTextFinished.BindUObject(this, &UPRFloatingTextManager::OnFloatingTextFinished);

	// 요청 + 스타일로 위젯 초기화 파라미터 조립
	FPRFloatingTextParams Params;
	Params.Text = Request.Text;
	Params.TextType = Request.TextType;
	Params.Color = Style.Color;

	// 위젯 초기화 및 연출 시작
	FloatingWidget->InitFloatingText(Params);
	FloatingWidget->OnPlay();
}

// ===== 풀 관리 =====

UWidgetComponent* UPRFloatingTextManager::AcquireWidgetComponent()
{
	// 풀에 사용 가능한 컴포넌트가 있으면 꺼낸다
	if (AvailablePool.Num() > 0)
	{
		UWidgetComponent* Comp = AvailablePool.Pop();
		ActiveWidgets.Add(Comp);
		return Comp;
	}

	// 새 WidgetComponent 생성 (PoolActor에 부착)
	if (!IsValid(PoolActor))
	{
		return nullptr;
	}

	UWidgetComponent* NewComp = NewObject<UWidgetComponent>(PoolActor);
	if (!IsValid(NewComp))
	{
		return nullptr;
	}

	NewComp->RegisterComponent();
	NewComp->SetVisibility(false);
	NewComp->SetWidgetSpace(EWidgetSpace::Screen);
	NewComp->SetDrawAtDesiredSize(true);

	ActiveWidgets.Add(NewComp);

	return NewComp;
}

void UPRFloatingTextManager::ReleaseWidgetComponent(UWidgetComponent* InComponent)
{
	if (!IsValid(InComponent))
	{
		return;
	}

	// 위젯 인스턴스는 유지한 채 숨기기만 한다 (풀링 재사용)
	InComponent->SetVisibility(false);
	
	ActiveWidgets.Remove(InComponent);
	AvailablePool.Add(InComponent);
}

void UPRFloatingTextManager::OnFloatingTextFinished(UPRFloatingTextWidget* InWidget)
{
	if (!IsValid(InWidget))
	{
		return;
	}
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,1.f,FColor::Green,FString::Printf(TEXT("Fuck")));
	}
	
	InWidget->SetVisibility(ESlateVisibility::Collapsed);
	InWidget->RemoveFromParent();
	InWidget->OnFloatingTextFinished.Unbind();
	InWidget->Destruct();

	TObjectPtr<UWidgetComponent>* FoundComp = WidgetToComponentMap.Find(InWidget);
	if (FoundComp && IsValid(*FoundComp))
	{
		ReleaseWidgetComponent(*FoundComp);
	}

	WidgetToComponentMap.Remove(InWidget);
}

void UPRFloatingTextManager::ForceRetireOldest()
{
	while (ActiveWidgets.Num() >= MaxActiveCount)
	{
		// ActiveWidgets 배열의 첫 번째가 가장 오래된 항목
		UWidgetComponent* OldestComp = ActiveWidgets[0];
		if (!IsValid(OldestComp))
		{
			ActiveWidgets.RemoveAt(0);
			continue;
		}

		// 위젯 콜백 해제 및 매핑 정리
		UPRFloatingTextWidget* OldestWidget = Cast<UPRFloatingTextWidget>(OldestComp->GetWidget());
		if (IsValid(OldestWidget))
		{
			OldestWidget->FinishFloatingText();
		}
	}
}

// ===== 위젯 클래스 조회 =====

FPRFloatingTextStyle UPRFloatingTextManager::ResolveStyle(EPRFloatingTextType TextType) const
{
	// PRDeveloperSettings에서 타입별 스타일 조회
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (IsValid(Settings))
	{
		FPRFloatingTextStyle Style = Settings->GetFloatingTextStyleSync(TextType);
		if (IsValid(Style.WidgetClass))
		{
			return Style;
		}
	}

	// 폴백: DefaultWidgetClass + 흰색
	FPRFloatingTextStyle Fallback;
	Fallback.WidgetClass = DefaultWidgetClass;
	Fallback.Color = FLinearColor::White;
	return Fallback;
}
