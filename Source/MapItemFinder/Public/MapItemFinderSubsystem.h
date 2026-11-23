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
	const TArray<TSubclassOf<UFGItemDescriptor>>&,
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
		const TArray<TSubclassOf<UFGItemDescriptor>>& ProducedItems,
		bool bFromServer = true);

	void RequestSearch(UWorld* World, TSubclassOf<UFGItemDescriptor> ItemClass);
	void HandleSearchResults(
		UWorld* World,
		TSubclassOf<UFGItemDescriptor> ItemClass,
		int32 RequestId,
		const TArray<FMapFinderCluster>& Results,
		bool bFromServer = true);

	void SetFilters(UWorld* World, bool bInShowProduced, bool bInShowStored);
	void ClearPins(UWorld* World);

	bool GetShowProduced() const { return bShowProduced; }
	bool GetShowStored() const { return bShowStored; }
	TSubclassOf<UFGItemDescriptor> GetActiveItemClass() const { return ActiveItemClass; }
	const TArray<FMapFinderCluster>& GetLastResults() const { return LastResults; }
	const TArray<TSubclassOf<UFGItemDescriptor>>& GetCachedProducedItems() const { return CachedProducedItems; }

	FOnMapFinderResultsUpdated OnResultsUpdated;
	FOnMapFinderProducedItemsUpdated OnProducedItemsUpdated;

private:
	void RebuildPins(UWorld* World);

#if !UE_SERVER
	void InstallMapHooks();
	void HandleMapOpened(UFGMapWidget* MapWidget);
	void HandleMapClosed(UFGMapWidget* MapWidget);
	void RemoveOverlayFromViewport();

	TSharedPtr<SMapItemFinderOverlay> ActiveOverlay;
	TWeakObjectPtr<UWorld> OverlayWorld;
	bool bHooksInstalled = false;
#endif

	UPROPERTY()
	TArray<TObjectPtr<UMapItemFinderRepresentation>> ActivePins;

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> ActiveItemClass;

	UPROPERTY()
	TArray<TSubclassOf<UFGItemDescriptor>> CachedProducedItems;

	TArray<FMapFinderCluster> LastResults;
	int32 LastSearchRequestId = 0;
	int32 LastProducedItemsRequestId = 0;
	bool bFinderModeActive = false;
	bool bShowProduced = true;
	bool bShowStored = true;
};
