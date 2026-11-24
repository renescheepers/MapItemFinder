#pragma once

#include "CoreMinimal.h"
#include "MapItemFinderTypes.h"
#include "Resources/FGItemDescriptor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MapItemFinderSubsystem.generated.h"

class UFGMapWidget;
class UMapItemFinderRepresentation;

#if !UE_SERVER
class SMapItemFinderOverlay;
#endif

DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnMapFinderResultsUpdated,
	TSubclassOf<UFGItemDescriptor>,
	const TArray<FMapFinderCluster>&,
	bool /* bFromServer */);

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnMapFinderProducedItemsUpdated,
	const TArray<FMapFinderProducedItem>&,
	bool /* bFromServer */);

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnMapFinderResourceNodesUpdated,
	const TArray<FMapFinderResourceNode>&,
	bool /* bFromServer */);

UCLASS()
class MAPITEMFINDER_API UMapItemFinderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void SetFinderMode(UWorld* World, bool bEnableFinder);
	bool IsFinderModeActive() const { return bFinderModeActive; }

	void RequestProducedItems(UWorld* World);
	void HandleProducedItems(
		int32 RequestId,
		const TArray<FMapFinderProducedItem>& ProducedItems,
		bool bFromServer = true);

	void RequestSearch(UWorld* World, TSubclassOf<UFGItemDescriptor> ItemClass);
	void HandleSearchResults(
		UWorld* World,
		TSubclassOf<UFGItemDescriptor> ItemClass,
		int32 RequestId,
		const TArray<FMapFinderCluster>& Results,
		bool bFromServer = true);

	void RequestResourceNodes(UWorld* World);
	void HandleResourceNodesChunk(
		UWorld* World,
		int32 RequestId,
		const TArray<FMapFinderResourceNode>& NodesChunk,
		bool bReset,
		bool bIsFinal,
		bool bFromServer = true);

	void SetShowResourceNodes(UWorld* World, bool bEnable);
	void SetResourceNodeFilter(UWorld* World, TSubclassOf<UFGItemDescriptor> ResourceClass);
	void ClearResourceNodePins(UWorld* World);

	void SetFilters(UWorld* World, bool bInShowProduced, bool bInShowStored);
	void ClearPins(UWorld* World);

	bool GetShowProduced() const { return bShowProduced; }
	bool GetShowStored() const { return bShowStored; }
	bool GetShowResourceNodes() const { return bShowResourceNodes; }
	TSubclassOf<UFGItemDescriptor> GetSelectedNodeResourceFilter() const { return SelectedNodeResourceFilter; }
	TSubclassOf<UFGItemDescriptor> GetActiveItemClass() const { return ActiveItemClass; }
	const TArray<FMapFinderCluster>& GetLastResults() const { return LastResults; }
	const TArray<FMapFinderProducedItem>& GetCachedProducedItems() const { return CachedProducedItems; }
	const TArray<FMapFinderResourceNode>& GetCachedResourceNodes() const { return CachedResourceNodes; }

	FOnMapFinderResultsUpdated OnResultsUpdated;
	FOnMapFinderProducedItemsUpdated OnProducedItemsUpdated;
	FOnMapFinderResourceNodesUpdated OnResourceNodesUpdated;

#if !UE_SERVER
	void HandleMapOpened(UFGMapWidget* MapWidget);
	void HandleMapClosed(UFGMapWidget* MapWidget);
#endif

private:
	void RebuildPins(UWorld* World);
	void RebuildResourceNodePins(UWorld* World);

#if !UE_SERVER
	void InstallMapHooks();
	void RemoveOverlayFromViewport();

	TSharedPtr<SMapItemFinderOverlay> ActiveOverlay;
	TWeakObjectPtr<UWorld> OverlayWorld;
	bool bHooksInstalled = false;
#endif

	UPROPERTY()
	TArray<TObjectPtr<UMapItemFinderRepresentation>> ActivePins;

	UPROPERTY()
	TArray<TObjectPtr<UMapItemFinderRepresentation>> ResourceNodePins;

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> ActiveItemClass;

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> SelectedNodeResourceFilter;

	UPROPERTY()
	TArray<FMapFinderProducedItem> CachedProducedItems;

	UPROPERTY()
	TArray<FMapFinderResourceNode> CachedResourceNodes;

	TArray<FMapFinderCluster> LastResults;
	int32 LastSearchRequestId = 0;
	int32 LastProducedItemsRequestId = 0;
	int32 LastResourceNodesRequestId = 0;
	bool bFinderModeActive = false;
	bool bShowProduced = true;
	bool bShowStored = true;
	bool bShowResourceNodes = false;
};
