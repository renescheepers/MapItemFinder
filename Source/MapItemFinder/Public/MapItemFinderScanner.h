#pragma once

#include "CoreMinimal.h"
#include "MapItemFinderTypes.h"
#include "Resources/FGItemDescriptor.h"

class MAPITEMFINDER_API FMapItemFinderScanner
{
public:
	static TArray<FMapFinderCluster> ScanWorldForItem(
		UWorld* World,
		TSubclassOf<UFGItemDescriptor> ItemClass,
		float ClusterRadiusCm = 2500.0f,
		int32 MaxClusters = 120);
};
