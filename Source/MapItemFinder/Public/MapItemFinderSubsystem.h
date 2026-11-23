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

UCLASS()
class MAPITEMFINDER_API UMapItemFinderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

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

	FOnMapFinderResultsUpdated OnResultsUpdated;

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

	TArray<FMapFinderCluster> LastResults;
	int32 LastRequestId = 0;
	bool bShowProduced = true;
	bool bShowStored = true;
};
