// Copyright ProjectR. All Rights Reserved.

#include "PRLoadingScreenWidgetBase.h"

#include "Components/Image.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

void UPRLoadingScreenWidgetBase::SetLoadingDestination(const FString& MapPackageName)
{
	DestinationMapPackageName = MapPackageName;
	DestinationMapShortName = ResolveShortMapName(MapPackageName);

	const FName PackageName(*DestinationMapPackageName);
	const FPRLoadingScreenImageEntry* ImageEntry = FindImageEntry(PackageName, DestinationMapShortName);
	ApplyImageBrushes(ImageEntry);
	OnLoadingDestinationChanged(DestinationMapPackageName, DestinationMapShortName);
}

void UPRLoadingScreenWidgetBase::SetLoadingScreenWidgetPhase(EPRLoadingScreenWidgetPhase NewPhase)
{
	LoadingScreenWidgetPhase = NewPhase;

	const FName PackageName(*DestinationMapPackageName);
	const FPRLoadingScreenImageEntry* ImageEntry = FindImageEntry(PackageName, DestinationMapShortName);
	ApplyImageBrushes(ImageEntry);
}

const FPRLoadingScreenImageEntry* UPRLoadingScreenWidgetBase::FindImageEntry(FName MapPackageName, FName MapShortName) const
{
	for (const FPRLoadingScreenImageEntry& ImageEntry : MapImageEntries)
	{
		if (ImageEntry.MapName == MapPackageName || ImageEntry.MapName == MapShortName)
		{
			return &ImageEntry;
		}
	}

	return nullptr;
}

void UPRLoadingScreenWidgetBase::ApplyImageBrushes(const FPRLoadingScreenImageEntry* ImageEntry)
{
	const FSlateBrush& PrimaryBrush = ImageEntry ? ImageEntry->PrimaryImageBrush : DefaultPrimaryImageBrush;
	const bool bMoviePlayerPhase = LoadingScreenWidgetPhase == EPRLoadingScreenWidgetPhase::MoviePlayer;
	const FSlateBrush& SecondaryBrush = ImageEntry
		? (bMoviePlayerPhase ? ImageEntry->MoviePlayerSecondaryImageBrush : ImageEntry->ViewportSecondaryImageBrush)
		: (bMoviePlayerPhase ? DefaultMoviePlayerSecondaryImageBrush : DefaultViewportSecondaryImageBrush);

	if (IsValid(PrimaryImage))
	{
		PrimaryImage->SetBrush(PrimaryBrush);
	}

	if (IsValid(SecondaryImage))
	{
		SecondaryImage->SetBrush(SecondaryBrush);
	}
}

FName UPRLoadingScreenWidgetBase::ResolveShortMapName(const FString& MapPackageName) const
{
	FString NormalizedMapName = UWorld::RemovePIEPrefix(MapPackageName);
	int32 ObjectNameDelimiterIndex = INDEX_NONE;
	if (NormalizedMapName.FindChar(TEXT('.'), ObjectNameDelimiterIndex))
	{
		NormalizedMapName.LeftInline(ObjectNameDelimiterIndex);
	}

	return FName(*FPackageName::GetShortName(NormalizedMapName));
}
