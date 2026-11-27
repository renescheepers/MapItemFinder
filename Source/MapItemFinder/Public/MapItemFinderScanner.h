#pragma once

#include "CoreMinimal.h"
#include "MapItemFinderTypes.h"
#include "Resources/FGItemDescriptor.h"

class MAPITEMFINDER_API FMapItemFinderScanner
{
public:
	static TArray<FMapFinderProducedItem> ScanProducedItems(UWorld* World);

	static TArray<FMapFinderResourceNode> ScanResourceNodes(UWorld* World);

	static TArray<FMapFinderMercerSphere> ScanMercerSpheres(UWorld* World);

	static TArray<FMapFinderCluster> ScanWorldForItem(
		UWorld* World,
		TSubclassOf<UFGItemDescriptor> ItemClass,
		float ClusterRadiusCm = 2500.0f,
		int32 MaxClusters = 120);
};
