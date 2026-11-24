#include "MapItemFinderSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "FGActorRepresentationManager.h"
#include "FGPlayerController.h"
#include "MapItemFinderModule.h"
#include "MapItemFinderRCO.h"
#include "MapItemFinderRepresentation.h"
#include "MapItemFinderScanner.h"
#include "Registry/RemoteCallObjectRegistry.h"

#if !UE_SERVER
#include "Patching/NativeHookManager.h"
#include "SMapItemFinderOverlay.h"
#include "UI/FGMapWidget.h"
#endif

void UMapItemFinderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<URemoteCallObjectRegistry>();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (URemoteCallObjectRegistry* RCORegistry = GI->GetSubsystem<URemoteCallObjectRegistry>())
		{
			RCORegistry->RegisterRemoteCallObject(UMapItemFinderRCO::StaticClass());
		}
	}

#if !UE_SERVER
	if (!IsRunningDedicatedServer())
	{
		InstallMapHooks();
	}
#endif
}

void UMapItemFinderSubsystem::Deinitialize()
{
#if !UE_SERVER
	RemoveOverlayFromViewport();
#endif
	if (UWorld* World = GetWorld())
	{
		ClearPins(World);
	}
	Super::Deinitialize();
}

void UMapItemFinderSubsystem::SetFinderMode(UWorld* World, bool bEnableFinder)
{
	bFinderModeActive = bEnableFinder;

	if (!bFinderModeActive)
	{
		ClearPins(World);
		return;
	}

	if (World)
	{
		RequestProducedItems(World);
	}
}

void UMapItemFinderSubsystem::RequestProducedItems(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const int32 RequestId = ++LastProducedItemsRequestId;

	if (AFGPlayerController* PC = Cast<AFGPlayerController>(World->GetFirstPlayerController()))
	{
		if (UMapItemFinderRCO* RCO = PC->GetRemoteCallObjectOfClass<UMapItemFinderRCO>())
		{
			RCO->Server_RequestProducedItems(RequestId);
			return;
		}
	}

	const TArray<TSubclassOf<UFGItemDescriptor>> LocalItems = FMapItemFinderScanner::ScanProducedItems(World);
	HandleProducedItems(RequestId, LocalItems, false);
}

void UMapItemFinderSubsystem::HandleProducedItems(
	int32 RequestId,
	const TArray<TSubclassOf<UFGItemDescriptor>>& ProducedItems,
	bool bFromServer)
{
	if (RequestId != LastProducedItemsRequestId)
	{
		return;
	}

	CachedProducedItems = ProducedItems;
	OnProducedItemsUpdated.Broadcast(CachedProducedItems, bFromServer);
}

void UMapItemFinderSubsystem::RequestSearch(UWorld* World, TSubclassOf<UFGItemDescriptor> ItemClass)
{
	if (!World || !ItemClass)
	{
		return;
	}

	ActiveItemClass = ItemClass;
	const int32 RequestId = ++LastSearchRequestId;

	if (AFGPlayerController* PC = Cast<AFGPlayerController>(World->GetFirstPlayerController()))
	{
		if (UMapItemFinderRCO* RCO = PC->GetRemoteCallObjectOfClass<UMapItemFinderRCO>())
		{
			RCO->Server_SearchItem(ItemClass, RequestId);
			return;
		}
	}

	const TArray<FMapFinderCluster> LocalResults = FMapItemFinderScanner::ScanWorldForItem(World, ItemClass);
	HandleSearchResults(World, ItemClass, RequestId, LocalResults, false);
}

void UMapItemFinderSubsystem::HandleSearchResults(
	UWorld* World,
	TSubclassOf<UFGItemDescriptor> ItemClass,
	int32 RequestId,
	const TArray<FMapFinderCluster>& Results,
	bool bFromServer)
{
	if (!World || RequestId != LastSearchRequestId || !bFinderModeActive)
	{
		return;
	}

	ActiveItemClass = ItemClass;
	LastResults = Results;
	RebuildPins(World);
	OnResultsUpdated.Broadcast(ItemClass, LastResults, bFromServer);
}

void UMapItemFinderSubsystem::SetFilters(UWorld* World, bool bInShowProduced, bool bInShowStored)
{
	bShowProduced = bInShowProduced;
	bShowStored = bInShowStored;
	if (World && bFinderModeActive && ActiveItemClass)
	{
		RebuildPins(World);
	}
}

void UMapItemFinderSubsystem::ClearPins(UWorld* World)
{
	if (World)
	{
		if (AFGActorRepresentationManager* RepManager = AFGActorRepresentationManager::Get(World))
		{
			for (UMapItemFinderRepresentation* Pin : ActivePins)
			{
				if (IsValid(Pin))
				{
					RepManager->RemoveRepresentation(Pin);
				}
			}
		}
	}
	ActivePins.Empty();
	ActiveItemClass = nullptr;
	LastResults.Empty();
}

void UMapItemFinderSubsystem::RebuildPins(UWorld* World)
{
	AFGActorRepresentationManager* RepManager = AFGActorRepresentationManager::Get(World);
	if (!RepManager)
	{
		return;
	}

	for (UMapItemFinderRepresentation* Pin : ActivePins)
	{
		if (IsValid(Pin))
		{
			RepManager->RemoveRepresentation(Pin);
		}
	}
	ActivePins.Empty();

	for (const FMapFinderCluster& Cluster : LastResults)
	{
		const bool bIsProduction = (Cluster.Kind == EMapFinderResultKind::Produced || Cluster.Kind == EMapFinderResultKind::Extracted);
		const bool bIsStorage = (Cluster.Kind == EMapFinderResultKind::Stored || Cluster.Kind == EMapFinderResultKind::DimensionalDepot);

		if ((bIsProduction && !bShowProduced) || (bIsStorage && !bShowStored))
		{
			continue;
		}

		UMapItemFinderRepresentation* Pin = NewObject<UMapItemFinderRepresentation>(RepManager);
		if (!Pin)
		{
			continue;
		}

		Pin->InitializePin(ActiveItemClass, Cluster);
		RepManager->AddRepresentation(Pin);
		ActivePins.Add(Pin);
	}
}

#if !UE_SERVER
namespace
{
	// Exposes protected UFGMapWidget virtuals for SML's RegisterHookFunction
	struct FMapWidgetProtectedAccess : public UFGMapWidget
	{
		using UFGMapWidget::NativeConstruct;
		using UFGMapWidget::NativeDestruct;
	};

	TWeakObjectPtr<UMapItemFinderSubsystem> GMapFinderHookSubsystem;
	void (*GOriginalMapNativeConstruct)(UFGMapWidget*) = nullptr;
	void (*GOriginalMapNativeDestruct)(UFGMapWidget*) = nullptr;

	void Hook_MapNativeConstruct(UFGMapWidget* MapWidget)
	{
		if (GOriginalMapNativeConstruct)
		{
			GOriginalMapNativeConstruct(MapWidget);
		}
		if (GMapFinderHookSubsystem.IsValid())
		{
			GMapFinderHookSubsystem->HandleMapOpened(MapWidget);
		}
	}

	void Hook_MapNativeDestruct(UFGMapWidget* MapWidget)
	{
		if (GOriginalMapNativeDestruct)
		{
			GOriginalMapNativeDestruct(MapWidget);
		}
		if (GMapFinderHookSubsystem.IsValid())
		{
			GMapFinderHookSubsystem->HandleMapClosed(MapWidget);
		}
	}
}

void UMapItemFinderSubsystem::InstallMapHooks()
{
	GMapFinderHookSubsystem = this;
	if (bHooksInstalled)
	{
		return;
	}
	bHooksInstalled = true;

	UFGMapWidget* MapCDO = GetMutableDefault<UFGMapWidget>();
	if (!MapCDO)
	{
		return;
	}

	using FMapWidgetVoidFn = void (UFGMapWidget::*)();

	FMapWidgetVoidFn NativeConstructPtr = &FMapWidgetProtectedAccess::NativeConstruct;
	FNativeHookManagerInternal::RegisterHookFunction(
		TEXT("UFGMapWidget::NativeConstruct"),
		ConvertFunctionPointer(NativeConstructPtr),
		MapCDO,
		reinterpret_cast<void*>(&Hook_MapNativeConstruct),
		reinterpret_cast<void**>(&GOriginalMapNativeConstruct));

	FMapWidgetVoidFn NativeDestructPtr = &FMapWidgetProtectedAccess::NativeDestruct;
	FNativeHookManagerInternal::RegisterHookFunction(
		TEXT("UFGMapWidget::NativeDestruct"),
		ConvertFunctionPointer(NativeDestructPtr),
		MapCDO,
		reinterpret_cast<void*>(&Hook_MapNativeDestruct),
		reinterpret_cast<void**>(&GOriginalMapNativeDestruct));
}

void UMapItemFinderSubsystem::HandleMapOpened(UFGMapWidget* MapWidget)
{
	if (!MapWidget)
	{
		return;
	}

	UWorld* World = MapWidget->GetWorld();
	if (!World || !World->GetGameViewport())
	{
		return;
	}

	RemoveOverlayFromViewport();

	OverlayWorld = World;
	ActiveOverlay = SNew(SMapItemFinderOverlay)
		.World(World)
		.Subsystem(this);

	World->GetGameViewport()->AddViewportWidgetContent(ActiveOverlay.ToSharedRef(), 100);

	if (bFinderModeActive)
	{
		RequestProducedItems(World);
	}
}

void UMapItemFinderSubsystem::HandleMapClosed(UFGMapWidget* MapWidget)
{
	RemoveOverlayFromViewport();
}

void UMapItemFinderSubsystem::RemoveOverlayFromViewport()
{
	if (ActiveOverlay.IsValid())
	{
		if (OverlayWorld.IsValid() && OverlayWorld->GetGameViewport())
		{
			OverlayWorld->GetGameViewport()->RemoveViewportWidgetContent(ActiveOverlay.ToSharedRef());
		}
		ActiveOverlay.Reset();
	}
	OverlayWorld.Reset();
}
#endif
