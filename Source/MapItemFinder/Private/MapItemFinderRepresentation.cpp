#include "MapItemFinderRepresentation.h"

namespace
{
	FLinearColor ColorForKind(EMapFinderResultKind Kind)
	{
		switch (Kind)
		{
		case EMapFinderResultKind::Produced:
		case EMapFinderResultKind::Extracted:
			return FLinearColor::White;
		case EMapFinderResultKind::DimensionalDepot:
			return FLinearColor(0.88f, 0.72f, 1.00f, 1.0f);
		case EMapFinderResultKind::Stored:
		default:
			return FLinearColor(1.00f, 0.85f, 0.55f, 1.0f);
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

	UTexture2D* IconTexture = nullptr;
	if (ItemClass)
	{
		IconTexture = UFGItemDescriptor::GetBigIcon(ItemClass);
		if (!IconTexture)
		{
			IconTexture = UFGItemDescriptor::GetSmallIcon(ItemClass);
		}
	}

	PinScale = 1.35f;

	mIsLocal = true;
	mIsStatic = true;
	mLocalActorLocation = InCluster.Location;
	mActorLocation = InCluster.Location;
	mRepresentationTexture = IconTexture;
	mRepresentationText = PinText;
	mRepresentationColor = PinColor;
	mRepresentationType = ERepresentationType::RT_Default;
	mShouldShowOnMap = true;
	mShouldShowInCompass = false;
	mFogOfWarRevealType = EFogOfWarRevealType::FOWRT_None;
	SetIsOnClient(true);
	SetRepresentationID(FGuid::NewGuid());
}

void UMapItemFinderRepresentation::InitializeResourceNodePin(const FMapFinderResourceNode& InNode)
{
	ItemClass = InNode.ResourceClass;
	PinScale = 0.95f;
	PinColor = InNode.bOccupied
		? FLinearColor(0.55f, 0.55f, 0.55f, 1.0f)
		: FLinearColor::White;

	const FString ResourceName = ItemClass
		? UFGItemDescriptor::GetItemName(ItemClass).ToString()
		: TEXT("Resource");

	const TCHAR* PurityText = TEXT("Well");
	switch (InNode.Purity)
	{
	case 0: PurityText = TEXT("Impure"); break;
	case 1: PurityText = TEXT("Normal"); break;
	case 2: PurityText = TEXT("Pure"); break;
	default: break;
	}

	const TCHAR* StateText = InNode.bOccupied ? TEXT("Occupied") : TEXT("Free");
	PinText = FText::FromString(FString::Printf(TEXT("%s — %s (%s)"), *ResourceName, PurityText, StateText));

	UTexture2D* IconTexture = nullptr;
	if (ItemClass)
	{
		IconTexture = UFGItemDescriptor::GetBigIcon(ItemClass);
		if (!IconTexture)
		{
			IconTexture = UFGItemDescriptor::GetSmallIcon(ItemClass);
		}
	}

	mIsLocal = true;
	mIsStatic = true;
	mLocalActorLocation = InNode.Location;
	mActorLocation = InNode.Location;
	mRepresentationTexture = IconTexture;
	mRepresentationText = PinText;
	mRepresentationColor = PinColor;
	mRepresentationType = ERepresentationType::RT_Default;
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
	return ERepresentationType::RT_Default;
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
	return PinScale;
}
