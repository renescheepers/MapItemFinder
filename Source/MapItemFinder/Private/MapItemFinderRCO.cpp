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

void UMapItemFinderRCO::Server_RequestResourceNodes_Implementation(int32 RequestId)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TArray<FMapFinderResourceNode> AllNodes = FMapItemFinderScanner::ScanResourceNodes(World);
	constexpr int32 ChunkSize = 120;

	if (AllNodes.Num() == 0)
	{
		Client_ReceiveResourceNodesChunk(RequestId, AllNodes, true, true);
		return;
	}

	for (int32 Start = 0; Start < AllNodes.Num(); Start += ChunkSize)
	{
		const int32 Count = FMath::Min(ChunkSize, AllNodes.Num() - Start);
		TArray<FMapFinderResourceNode> Chunk;
		Chunk.Append(AllNodes.GetData() + Start, Count);
		const bool bReset = (Start == 0);
		const bool bIsFinal = (Start + Count >= AllNodes.Num());
		Client_ReceiveResourceNodesChunk(RequestId, Chunk, bReset, bIsFinal);
	}
}

void UMapItemFinderRCO::Client_ReceiveResourceNodesChunk_Implementation(
	int32 RequestId,
	const TArray<FMapFinderResourceNode>& NodesChunk,
	bool bReset,
	bool bIsFinal)
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
			Subsystem->HandleResourceNodesChunk(World, RequestId, NodesChunk, bReset, bIsFinal, true);
		}
	}
}

void UMapItemFinderRCO::Server_RequestMercerSpheres_Implementation(int32 RequestId)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TArray<FMapFinderMercerSphere> AllSpheres = FMapItemFinderScanner::ScanMercerSpheres(World);
	constexpr int32 ChunkSize = 120;

	if (AllSpheres.Num() == 0)
	{
		Client_ReceiveMercerSpheresChunk(RequestId, AllSpheres, true, true);
		return;
	}

	for (int32 Start = 0; Start < AllSpheres.Num(); Start += ChunkSize)
	{
		const int32 Count = FMath::Min(ChunkSize, AllSpheres.Num() - Start);
		TArray<FMapFinderMercerSphere> Chunk;
		Chunk.Append(AllSpheres.GetData() + Start, Count);
		const bool bReset = (Start == 0);
		const bool bIsFinal = (Start + Count >= AllSpheres.Num());
		Client_ReceiveMercerSpheresChunk(RequestId, Chunk, bReset, bIsFinal);
	}
}

void UMapItemFinderRCO::Client_ReceiveMercerSpheresChunk_Implementation(
	int32 RequestId,
	const TArray<FMapFinderMercerSphere>& SpheresChunk,
	bool bReset,
	bool bIsFinal)
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
			Subsystem->HandleMercerSpheresChunk(World, RequestId, SpheresChunk, bReset, bIsFinal, true);
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
