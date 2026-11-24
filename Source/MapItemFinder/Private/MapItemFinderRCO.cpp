#include "MapItemFinderRCO.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MapItemFinderScanner.h"
#include "MapItemFinderSubsystem.h"
#include "Net/UnrealNetwork.h"

void UMapItemFinderRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMapItemFinderRCO, bForceNetField);
}

void UMapItemFinderRCO::Server_RequestProducedItems_Implementation(int32 RequestId)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TArray<FMapFinderProducedItem> ProducedItems = FMapItemFinderScanner::ScanProducedItems(World);
	Client_ReceiveProducedItems(RequestId, ProducedItems);
}

void UMapItemFinderRCO::Client_ReceiveProducedItems_Implementation(
	int32 RequestId,
	const TArray<FMapFinderProducedItem>& ProducedItems)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UMapItemFinderSubsystem* Subsystem = GameInstance->GetSubsystem<UMapItemFinderSubsystem>())
		{
			Subsystem->HandleProducedItems(RequestId, ProducedItems, true);
		}
	}
}

bool UMapItemFinderRCO::Server_SearchItem_Validate(TSubclassOf<UFGItemDescriptor> ItemClass, int32 RequestId)
{
	return ItemClass != nullptr;
}

void UMapItemFinderRCO::Server_SearchItem_Implementation(TSubclassOf<UFGItemDescriptor> ItemClass, int32 RequestId)
{
	UWorld* World = GetWorld();
	if (!World || !ItemClass)
	{
		return;
	}

	const TArray<FMapFinderCluster> Results = FMapItemFinderScanner::ScanWorldForItem(World, ItemClass);
	Client_ReceiveSearchResults(ItemClass, RequestId, Results);
}

void UMapItemFinderRCO::Client_ReceiveSearchResults_Implementation(
	TSubclassOf<UFGItemDescriptor> ItemClass,
	int32 RequestId,
	const TArray<FMapFinderCluster>& Results)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UMapItemFinderSubsystem* Subsystem = GameInstance->GetSubsystem<UMapItemFinderSubsystem>())
		{
			Subsystem->HandleSearchResults(World, ItemClass, RequestId, Results);
		}
	}
}
