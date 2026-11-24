#include "MapItemFinderRepresentation.h"

#include "FGIconDatabaseSubsystem.h"

namespace
{
	FLinearColor ColorForKind(EMapFinderResultKind Kind)
	{
		switch (Kind)
		{
		case EMapFinderResultKind::Produced:
			return FLinearColor(0.20f, 0.85f, 0.35f, 1.0f);
		case EMapFinderResultKind::Extracted:
			return FLinearColor(0.20f, 0.78f, 0.95f, 1.0f);
		case EMapFinderResultKind::DimensionalDepot:
			return FLinearColor(0.72f, 0.35f, 0.98f, 1.0f);
		case EMapFinderResultKind::Stored:
		default:
			return FLinearColor(0.98f, 0.62f, 0.12f, 1.0f);
		}
	}

	UTexture2D* GetItemIconTexture(TSubclassOf<UFGItemDescriptor> InItemClass)
	{
		if (!InItemClass)
		{
			return nullptr;
		}
		if (UTexture2D* BigIcon = UFGItemDescriptor::GetBigIcon(InItemClass))
		{
			return BigIcon;
		}
		return UFGItemDescriptor::GetSmallIcon(InItemClass);
	}
}

int32 UMapItemFinderRepresentation::ResolveIconID(
	UWorld* World,
	TSubclassOf<UFGItemDescriptor> InItemClass,
	UTexture2D* IconTexture)
{
	if (!World)
	{
		return INDEX_NONE;
	}

	AFGIconDatabaseSubsystem* IconDB = AFGIconDatabaseSubsystem::Get(World);
	if (!IconDB)
	{
		return INDEX_NONE;
	}

	if (IconTexture)
	{
		const int32 ByBigTex = IconDB->GetIconIDForTexture(IconTexture);
		if (ByBigTex != INDEX_NONE)
		{
			return ByBigTex;
		}
	}

	if (InItemClass)
	{
		if (UTexture2D* SmallIcon = UFGItemDescriptor::GetSmallIcon(InItemClass))
		{
			const int32 BySmallTex = IconDB->GetIconIDForTexture(SmallIcon);
			if (BySmallTex != INDEX_NONE)
			{
				return BySmallTex;
			}
		}

		const FSoftClassPath TargetPath(InItemClass.Get());
		for (const FIconData& Data : IconDB->GetAllIconData())
		{
			if (Data.ItemDescriptor.ToSoftObjectPath() == TargetPath)
			{
				return Data.ID;
			}
		}
	}

	return INDEX_NONE;
}

void UMapItemFinderRepresentation::InitializePin(
	UWorld* World,
	TSubclassOf<UFGItemDescriptor> InItemClass,
	const FMapFinderCluster& InCluster)
{
	ItemClass = InItemClass;
	CachedIconTexture = GetItemIconTexture(InItemClass);
	PinColor = ColorForKind(InCluster.Kind);
	PinScale = 1.25f;

	const FString ItemName = ItemClass ? UFGItemDescriptor::GetItemName(ItemClass).ToString() : TEXT("Item");
	const FString FullLabel = FString::Printf(TEXT("%s — %s"), *ItemName, *InCluster.Label);
	PinText = FText::FromString(FullLabel);

	const int32 ResolvedIconId = ResolveIconID(World, InItemClass, CachedIconTexture);

	FMapMarker Marker;
	Marker.MarkerGUID = FGuid::NewGuid();
	Marker.Location = InCluster.Location;
	Marker.Name = FullLabel;
	Marker.CategoryName = TEXT("Map Item Finder");
	Marker.MapMarkerType = ERepresentationType::RT_MapMarker;
	Marker.IconID = (ResolvedIconId != INDEX_NONE) ? ResolvedIconId : 0;
	Marker.Color = PinColor;
	Marker.Scale = PinScale;
	Marker.CompassViewDistance = ECompassViewDistance::CVD_Off;

	Init(Marker);

	mIsLocal = true;
	mIsStatic = true;
	mLocalActorLocation = InCluster.Location;
	mActorLocation = InCluster.Location;
	if (CachedIconTexture)
	{
		mRepresentationTexture = CachedIconTexture;
	}
	mRepresentationText = PinText;
	mRepresentationColor = PinColor;
	mShouldShowOnMap = true;
	mShouldShowInCompass = false;
	mFogOfWarRevealType = EFogOfWarRevealType::FOWRT_None;
	SetIsOnClient(true);
	SetRepresentationID(Marker.MarkerGUID);
}

void UMapItemFinderRepresentation::InitializeResourceNodePin(
	UWorld* World,
	const FMapFinderResourceNode& InNode)
{
	ItemClass = InNode.ResourceClass;
	CachedIconTexture = GetItemIconTexture(ItemClass);
	PinScale = 0.95f;
	PinColor = InNode.bOccupied
		? FLinearColor(0.45f, 0.45f, 0.45f, 1.0f)
		: FLinearColor(0.20f, 0.78f, 0.95f, 1.0f);

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
	const FString FullLabel = FString::Printf(TEXT("%s — %s (%s)"), *ResourceName, PurityText, StateText);
	PinText = FText::FromString(FullLabel);

	const int32 ResolvedIconId = ResolveIconID(World, ItemClass, CachedIconTexture);

	FMapMarker Marker;
	Marker.MarkerGUID = FGuid::NewGuid();
	Marker.Location = InNode.Location;
	Marker.Name = FullLabel;
	Marker.CategoryName = TEXT("Resource Nodes");
	Marker.MapMarkerType = ERepresentationType::RT_MapMarker;
	Marker.IconID = (ResolvedIconId != INDEX_NONE) ? ResolvedIconId : 0;
	Marker.Color = PinColor;
	Marker.Scale = PinScale;
	Marker.CompassViewDistance = ECompassViewDistance::CVD_Off;

	Init(Marker);

	mIsLocal = true;
	mIsStatic = true;
	mLocalActorLocation = InNode.Location;
	mActorLocation = InNode.Location;
	if (CachedIconTexture)
	{
		mRepresentationTexture = CachedIconTexture;
	}
	mRepresentationText = PinText;
	mRepresentationColor = PinColor;
	mShouldShowOnMap = true;
	mShouldShowInCompass = false;
	mFogOfWarRevealType = EFogOfWarRevealType::FOWRT_None;
	SetIsOnClient(true);
	SetRepresentationID(Marker.MarkerGUID);
}

FVector UMapItemFinderRepresentation::GetActorLocation() const
{
	return mLocalActorLocation;
}

UTexture2D* UMapItemFinderRepresentation::GetRepresentationTexture() const
{
	if (CachedIconTexture)
	{
		return CachedIconTexture;
	}
	return Super::GetRepresentationTexture();
}

FText UMapItemFinderRepresentation::GetRepresentationText() const
{
	return PinText;
}

FLinearColor UMapItemFinderRepresentation::GetRepresentationColor() const
{
	return PinColor;
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
