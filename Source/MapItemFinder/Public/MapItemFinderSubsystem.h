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

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnMapFinderResultsUpdated,
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

	void ToggleItemSelection(UWorld* World, TSubclassOf<UFGItemDescriptor> ItemClass);
	bool IsItemSelected(TSubclassOf<UFGItemDescriptor> ItemClass) const;
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
	void ToggleAllResourceNodes(UWorld* World);
	void ClearResourceNodeSelection(UWorld* World);
	void ToggleResourceNodeFilter(UWorld* World, TSubclassOf<UFGItemDescriptor> ResourceClass);
	bool IsAllResourceNodesSelected() const { return bShowAllResourceNodes; }
	bool IsResourceNodeFilterSelected(TSubclassOf<UFGItemDescriptor> ResourceClass) const;
	void ClearResourceNodePins(UWorld* World);

	void SetFilters(UWorld* World, bool bInShowProduced, bool bInShowStored);
	void ClearPins(UWorld* World);

	bool GetShowProduced() const { return bShowProduced; }
	bool GetShowStored() const { return bShowStored; }
	bool GetShowResourceNodes() const { return bShowResourceNodes; }
	const TSet<TSubclassOf<UFGItemDescriptor>>& GetSelectedItemClasses() const { return SelectedItemClasses; }
	const TMap<TSubclassOf<UFGItemDescriptor>, TArray<FMapFinderCluster>>& GetResultsByItem() const { return ResultsByItem; }
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
	void RemoveActivePinsFromMap(UWorld* World);
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
	TSet<TSubclassOf<UFGItemDescriptor>> SelectedItemClasses;

	UPROPERTY()
	TSet<TSubclassOf<UFGItemDescriptor>> SelectedNodeResourceFilters;

	UPROPERTY()
	TArray<FMapFinderProducedItem> CachedProducedItems;

	UPROPERTY()
	TArray<FMapFinderResourceNode> CachedResourceNodes;

	TMap<TSubclassOf<UFGItemDescriptor>, TArray<FMapFinderCluster>> ResultsByItem;
	TMap<TSubclassOf<UFGItemDescriptor>, int32> ItemRequestIds;
	int32 NextSearchRequestId = 0;
	int32 LastProducedItemsRequestId = 0;
	int32 LastResourceNodesRequestId = 0;
	bool bFinderModeActive = false;
	bool bShowProduced = true;
	bool bShowStored = true;
	bool bShowResourceNodes = false;
	bool bShowAllResourceNodes = false;
};
