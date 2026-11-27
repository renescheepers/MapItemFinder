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
		ClearResourceNodePins(World);
		ClearMercerSpherePins(World);
	}
	Super::Deinitialize();
}

void UMapItemFinderSubsystem::SetFinderMode(UWorld* World, bool bEnableFinder)
{
	bFinderModeActive = bEnableFinder;

	if (!bFinderModeActive)
	{
		RemoveActivePinsFromMap(World);
		ClearResourceNodePins(World);
		ClearMercerSpherePins(World);
		return;
	}

	if (World)
	{
		if (SelectedItemClasses.Num() > 0)
		{
			RebuildPins(World);
			OnResultsUpdated.Broadcast(true);
		}
		RequestProducedItems(World);
		if (bShowResourceNodes)
		{
			if (CachedResourceNodes.Num() > 0)
			{
				RebuildResourceNodePins(World);
			}
			RequestResourceNodes(World);
		}
		if (bShowMercerSpheres)
		{
			if (CachedMercerSpheres.Num() > 0)
			{
				RebuildMercerSpherePins(World);
			}
			RequestMercerSpheres(World);
		}
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

	const TArray<FMapFinderProducedItem> LocalItems = FMapItemFinderScanner::ScanProducedItems(World);
	HandleProducedItems(RequestId, LocalItems, false);
}

void UMapItemFinderSubsystem::HandleProducedItems(
	int32 RequestId,
	const TArray<FMapFinderProducedItem>& ProducedItems,
	bool bFromServer)
{
	if (RequestId != LastProducedItemsRequestId)
	{
		return;
	}

	CachedProducedItems = ProducedItems;
	OnProducedItemsUpdated.Broadcast(CachedProducedItems, bFromServer);
}

void UMapItemFinderSubsystem::ToggleItemSelection(UWorld* World, TSubclassOf<UFGItemDescriptor> ItemClass)
{
	if (!World || !ItemClass)
	{
		return;
	}

	if (SelectedItemClasses.Contains(ItemClass))
	{
		SelectedItemClasses.Remove(ItemClass);
		ResultsByItem.Remove(ItemClass);
		ItemRequestIds.Remove(ItemClass);
		RebuildPins(World);
		OnResultsUpdated.Broadcast(true);
		return;
	}

	SelectedItemClasses.Add(ItemClass);
	RequestSearch(World, ItemClass);
}

bool UMapItemFinderSubsystem::IsItemSelected(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	return ItemClass && SelectedItemClasses.Contains(ItemClass);
}

void UMapItemFinderSubsystem::RequestSearch(UWorld* World, TSubclassOf<UFGItemDescriptor> ItemClass)
{
	if (!World || !ItemClass)
	{
		return;
	}

	const int32 RequestId = ++NextSearchRequestId;
	ItemRequestIds.Add(ItemClass, RequestId);

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
	if (!World || !ItemClass || !bFinderModeActive || !SelectedItemClasses.Contains(ItemClass))
	{
		return;
	}

	const int32* ExpectedId = ItemRequestIds.Find(ItemClass);
	if (!ExpectedId || *ExpectedId != RequestId)
	{
		return;
	}

	ResultsByItem.Add(ItemClass, Results);
	RebuildPins(World);
	OnResultsUpdated.Broadcast(bFromServer);
}

void UMapItemFinderSubsystem::RequestResourceNodes(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const int32 RequestId = ++LastResourceNodesRequestId;

	if (AFGPlayerController* PC = Cast<AFGPlayerController>(World->GetFirstPlayerController()))
	{
		if (UMapItemFinderRCO* RCO = PC->GetRemoteCallObjectOfClass<UMapItemFinderRCO>())
		{
			RCO->Server_RequestResourceNodes(RequestId);
			return;
		}
	}

	const TArray<FMapFinderResourceNode> LocalNodes = FMapItemFinderScanner::ScanResourceNodes(World);
	HandleResourceNodesChunk(World, RequestId, LocalNodes, true, true, false);
}

namespace
{
	bool AreResourceNodeListsEqual(
		const TArray<FMapFinderResourceNode>& A,
		const TArray<FMapFinderResourceNode>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 i = 0; i < A.Num(); ++i)
		{
			if (A[i].ResourceClass != B[i].ResourceClass
				|| A[i].Purity != B[i].Purity
				|| A[i].bOccupied != B[i].bOccupied
				|| !A[i].Location.Equals(B[i].Location, 1.0f))
			{
				return false;
			}
		}
		return true;
	}

	bool AreMercerSphereListsEqual(
		const TArray<FMapFinderMercerSphere>& A,
		const TArray<FMapFinderMercerSphere>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 i = 0; i < A.Num(); ++i)
		{
			if (A[i].ItemClass != B[i].ItemClass
				|| A[i].Count != B[i].Count
				|| A[i].bInStorage != B[i].bInStorage
				|| A[i].bInDepot != B[i].bInDepot
				|| !A[i].Location.Equals(B[i].Location, 1.0f))
			{
				return false;
			}
		}
		return true;
	}
}

void UMapItemFinderSubsystem::HandleResourceNodesChunk(
	UWorld* World,
	int32 RequestId,
	const TArray<FMapFinderResourceNode>& NodesChunk,
	bool bReset,
	bool bIsFinal,
	bool bFromServer)
{
	if (!World || RequestId != LastResourceNodesRequestId)
	{
		return;
	}

	if (bReset)
	{
		PendingResourceNodes.Empty();
	}
	PendingResourceNodes.Append(NodesChunk);

	if (bIsFinal)
	{
		const bool bChanged = !AreResourceNodeListsEqual(CachedResourceNodes, PendingResourceNodes);
		if (bChanged)
		{
			CachedResourceNodes = MoveTemp(PendingResourceNodes);
		}
		else
		{
			PendingResourceNodes.Empty();
		}

		if (bFinderModeActive && bShowResourceNodes)
		{
			const bool bWantsPins = bShowAllResourceNodes || SelectedNodeResourceFilters.Num() > 0;
			if (bChanged || (bWantsPins && ResourceNodePins.Num() == 0))
			{
				RebuildResourceNodePins(World);
			}
		}
		OnResourceNodesUpdated.Broadcast(CachedResourceNodes, bFromServer);
	}
}

void UMapItemFinderSubsystem::SetShowResourceNodes(UWorld* World, bool bEnable)
{
	bShowResourceNodes = bEnable;
	if (!bShowResourceNodes)
	{
		ClearResourceNodePins(World);
		return;
	}

	if (World && bFinderModeActive)
	{
		if (CachedResourceNodes.Num() > 0)
		{
			RebuildResourceNodePins(World);
		}
		RequestResourceNodes(World);
	}
}

void UMapItemFinderSubsystem::ToggleAllResourceNodes(UWorld* World)
{
	bShowAllResourceNodes = !bShowAllResourceNodes;
	if (bShowAllResourceNodes)
	{
		SelectedNodeResourceFilters.Empty();
	}

	if (World && bFinderModeActive && bShowResourceNodes)
	{
		RebuildResourceNodePins(World);
	}
}

void UMapItemFinderSubsystem::ClearResourceNodeSelection(UWorld* World)
{
	bShowAllResourceNodes = false;
	SelectedNodeResourceFilters.Empty();
	if (World)
	{
		ClearResourceNodePins(World);
	}
}

void UMapItemFinderSubsystem::ToggleResourceNodeFilter(UWorld* World, TSubclassOf<UFGItemDescriptor> ResourceClass)
{
	if (!ResourceClass)
	{
		ToggleAllResourceNodes(World);
		return;
	}

	bShowAllResourceNodes = false;
	if (SelectedNodeResourceFilters.Contains(ResourceClass))
	{
		SelectedNodeResourceFilters.Remove(ResourceClass);
	}
	else
	{
		SelectedNodeResourceFilters.Add(ResourceClass);
	}

	if (World && bFinderModeActive && bShowResourceNodes)
	{
		RebuildResourceNodePins(World);
	}
}

bool UMapItemFinderSubsystem::IsResourceNodeFilterSelected(TSubclassOf<UFGItemDescriptor> ResourceClass) const
{
	if (!ResourceClass)
	{
		return bShowAllResourceNodes;
	}
	return bShowAllResourceNodes || SelectedNodeResourceFilters.Contains(ResourceClass);
}

void UMapItemFinderSubsystem::ClearResourceNodePins(UWorld* World)
{
	if (World)
	{
		if (AFGActorRepresentationManager* RepManager = AFGActorRepresentationManager::Get(World))
		{
			for (UMapItemFinderRepresentation* Pin : ResourceNodePins)
			{
				if (IsValid(Pin))
				{
					RepManager->RemoveRepresentation(Pin);
				}
			}
		}
	}
	ResourceNodePins.Empty();
}

void UMapItemFinderSubsystem::RebuildResourceNodePins(UWorld* World)
{
	AFGActorRepresentationManager* RepManager = AFGActorRepresentationManager::Get(World);
	if (!RepManager)
	{
		return;
	}

	ClearResourceNodePins(World);

	if (!bShowAllResourceNodes && SelectedNodeResourceFilters.Num() == 0)
	{
		return;
	}

	for (const FMapFinderResourceNode& Node : CachedResourceNodes)
	{
		if (!Node.ResourceClass)
		{
			continue;
		}
		if (!bShowAllResourceNodes && !SelectedNodeResourceFilters.Contains(Node.ResourceClass))
		{
			continue;
		}

		UMapItemFinderRepresentation* Pin = NewObject<UMapItemFinderRepresentation>(RepManager);
		if (!Pin)
		{
			continue;
		}

		Pin->InitializeResourceNodePin(World, Node);
		RepManager->AddRepresentation(Pin);
		ResourceNodePins.Add(Pin);
	}
}

void UMapItemFinderSubsystem::RequestMercerSpheres(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const int32 RequestId = ++LastMercerSpheresRequestId;

	if (AFGPlayerController* PC = Cast<AFGPlayerController>(World->GetFirstPlayerController()))
	{
		if (UMapItemFinderRCO* RCO = PC->GetRemoteCallObjectOfClass<UMapItemFinderRCO>())
		{
			RCO->Server_RequestMercerSpheres(RequestId);
			return;
		}
	}

	const TArray<FMapFinderMercerSphere> LocalSpheres = FMapItemFinderScanner::ScanMercerSpheres(World);
	HandleMercerSpheresChunk(World, RequestId, LocalSpheres, true, true, false);
}

void UMapItemFinderSubsystem::HandleMercerSpheresChunk(
	UWorld* World,
	int32 RequestId,
	const TArray<FMapFinderMercerSphere>& SpheresChunk,
	bool bReset,
	bool bIsFinal,
	bool bFromServer)
{
	if (!World || RequestId != LastMercerSpheresRequestId)
	{
		return;
	}

	if (bReset)
	{
		PendingMercerSpheres.Empty();
	}
	PendingMercerSpheres.Append(SpheresChunk);

	if (bIsFinal)
	{
		const bool bChanged = !AreMercerSphereListsEqual(CachedMercerSpheres, PendingMercerSpheres);
		if (bChanged)
		{
			CachedMercerSpheres = MoveTemp(PendingMercerSpheres);
		}
		else
		{
			PendingMercerSpheres.Empty();
		}

		if (bFinderModeActive && bShowMercerSpheres)
		{
			if (bChanged || (CachedMercerSpheres.Num() > 0 && MercerSpherePins.Num() == 0))
			{
				RebuildMercerSpherePins(World);
			}
		}
	}
}

void UMapItemFinderSubsystem::SetShowMercerSpheres(UWorld* World, bool bEnable)
{
	bShowMercerSpheres = bEnable;
	if (!bShowMercerSpheres)
	{
		ClearMercerSpherePins(World);
		return;
	}

	if (World && bFinderModeActive)
	{
		if (CachedMercerSpheres.Num() > 0)
		{
			RebuildMercerSpherePins(World);
		}
		RequestMercerSpheres(World);
	}
}

void UMapItemFinderSubsystem::ClearMercerSpherePins(UWorld* World)
{
	if (World)
	{
		if (AFGActorRepresentationManager* RepManager = AFGActorRepresentationManager::Get(World))
		{
			for (UMapItemFinderRepresentation* Pin : MercerSpherePins)
			{
				if (IsValid(Pin))
				{
					RepManager->RemoveRepresentation(Pin);
				}
			}
		}
	}
	MercerSpherePins.Empty();
}

void UMapItemFinderSubsystem::RebuildMercerSpherePins(UWorld* World)
{
	AFGActorRepresentationManager* RepManager = AFGActorRepresentationManager::Get(World);
	if (!RepManager)
	{
		return;
	}

	ClearMercerSpherePins(World);

	for (const FMapFinderMercerSphere& Sphere : CachedMercerSpheres)
	{
		UMapItemFinderRepresentation* Pin = NewObject<UMapItemFinderRepresentation>(RepManager);
		if (!Pin)
		{
			continue;
		}

		Pin->InitializeMercerSpherePin(World, Sphere);
		RepManager->AddRepresentation(Pin);
		MercerSpherePins.Add(Pin);
	}
}

void UMapItemFinderSubsystem::SetFilters(UWorld* World, bool bInShowProduced, bool bInShowStored)
{
	bShowProduced = bInShowProduced;
	bShowStored = bInShowStored;
	if (World && bFinderModeActive && SelectedItemClasses.Num() > 0)
	{
		RebuildPins(World);
	}
}

void UMapItemFinderSubsystem::RemoveActivePinsFromMap(UWorld* World)
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
}

void UMapItemFinderSubsystem::ClearPins(UWorld* World)
{
	RemoveActivePinsFromMap(World);
	SelectedItemClasses.Empty();
	ResultsByItem.Empty();
	ItemRequestIds.Empty();
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

	for (const TSubclassOf<UFGItemDescriptor>& ItemClass : SelectedItemClasses)
	{
		const TArray<FMapFinderCluster>* Clusters = ResultsByItem.Find(ItemClass);
		if (!Clusters)
		{
			continue;
		}

		for (const FMapFinderCluster& Cluster : *Clusters)
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

			Pin->InitializePin(World, ItemClass, Cluster);
			RepManager->AddRepresentation(Pin);
			ActivePins.Add(Pin);
		}
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
		if (bShowResourceNodes)
		{
			RequestResourceNodes(World);
		}
		if (bShowMercerSpheres)
		{
			RequestMercerSpheres(World);
		}
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
