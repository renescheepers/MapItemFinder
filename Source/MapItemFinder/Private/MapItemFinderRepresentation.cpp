#include "MapItemFinderRepresentation.h"

namespace
{
	FLinearColor ColorForKind(EMapFinderResultKind Kind)
	{
		switch (Kind)
		{
		case EMapFinderResultKind::Produced:
			return FLinearColor(0.20f, 0.90f, 0.35f, 1.0f);
		case EMapFinderResultKind::Extracted:
			return FLinearColor(0.20f, 0.80f, 0.95f, 1.0f);
		case EMapFinderResultKind::DimensionalDepot:
			return FLinearColor(0.75f, 0.35f, 1.00f, 1.0f);
		case EMapFinderResultKind::Stored:
		default:
			return FLinearColor(1.00f, 0.65f, 0.10f, 1.0f);
		}
	}
}

void UMapItemFinderRepresentation::InitializePin(
	TSubclassOf<UFGItemDescriptor> InItemClass,
	const FMapFinderCluster& InCluster)
{
	ItemClass = InItemClass;
	ResultKind = InCluster.Kind;
	PinColor = ColorForKind(InCluster.Kind);

	const FText ItemName = ItemClass ? UFGItemDescriptor::GetItemName(ItemClass) : FText::GetEmpty();
	PinText = FText::FromString(FString::Printf(TEXT("%s — %s"), *ItemName.ToString(), *InCluster.Label));

	mIsLocal = true;
	mIsStatic = true;
	mLocalActorLocation = InCluster.Location;
	mActorLocation = InCluster.Location;
	mRepresentationText = PinText;
	mRepresentationColor = PinColor;
	mRepresentationType = ERepresentationType::RT_MapMarker;
	mShouldShowOnMap = true;
	mShouldShowInCompass = false;
	mFogOfWarRevealType = EFogOfWarRevealType::FOWRT_None;
	SetIsOnClient(true);
	SetRepresentationID(FGuid::NewGuid());
}

FVector UMapItemFinderRepresentation::GetActorLocation() const
{
	return mLocalActorLocation;
}

UTexture2D* UMapItemFinderRepresentation::GetRepresentationTexture() const
{
	if (!ItemClass)
	{
		return nullptr;
	}
	if (UTexture2D* BigIcon = UFGItemDescriptor::GetBigIcon(ItemClass))
	{
		return BigIcon;
	}
	return UFGItemDescriptor::GetSmallIcon(ItemClass);
}

FText UMapItemFinderRepresentation::GetRepresentationText() const
{
	return PinText;
}

FLinearColor UMapItemFinderRepresentation::GetRepresentationColor() const
{
	return PinColor;
}

ERepresentationType UMapItemFinderRepresentation::GetRepresentationType() const
{
	return ERepresentationType::RT_MapMarker;
}

bool UMapItemFinderRepresentation::GetShouldShowInCompass() const
{
	return false;
}

bool UMapItemFinderRepresentation::GetShouldShowOnMap() const
{
	return true;
}

float UMapItemFinderRepresentation::GetScaleOnMap() const
{
	return 1.25f;
}
