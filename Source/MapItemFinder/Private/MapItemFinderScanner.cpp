#include "MapItemFinderScanner.h"

#include "EngineUtils.h"
#include "FGBuildableSubsystem.h"
#include "FGInventoryComponent.h"
#include "FGItemPickup.h"
#include "FGItemPickup_Spawnable.h"
#include "FGRecipe.h"
#include "ItemAmount.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "Buildables/FGBuildableStorage.h"
#include "Buildables/FGCentralStorageContainer.h"
#include "Resources/FGExtractableResourceInterface.h"
#include "Resources/FGResourceDeposit.h"
#include "Resources/FGResourceNode.h"
#include "Resources/FGResourceNodeBase.h"
#include "Resources/FGResourceNodeFrackingSatellite.h"

namespace
{
	struct FClusterAccumulator
	{
		FVector SumLocation = FVector::ZeroVector;
		EMapFinderResultKind Kind = EMapFinderResultKind::Produced;
		int32 BuildingCount = 0;
		int32 ActiveCount = 0;
		int32 ItemCount = 0;
		TMap<FString, int32> BuildingNames;

		FVector GetCentroid() const
		{
			return BuildingCount > 0 ? (SumLocation / static_cast<double>(BuildingCount)) : FVector::ZeroVector;
		}
	};

	void AddToClusters(
		TArray<FClusterAccumulator>& Clusters,
		const FVector& Location,
		EMapFinderResultKind Kind,
		const FString& BuildingName,
		bool bIsActive,
		int32 ItemCount,
		float ClusterRadiusSq)
	{
		for (FClusterAccumulator& Cluster : Clusters)
		{
			if (Cluster.Kind == Kind && FVector::DistSquared(Cluster.GetCentroid(), Location) <= ClusterRadiusSq)
			{
				Cluster.SumLocation += Location;
				Cluster.BuildingCount += 1;
				Cluster.ActiveCount += bIsActive ? 1 : 0;
				Cluster.ItemCount += ItemCount;
				Cluster.BuildingNames.FindOrAdd(BuildingName) += 1;
				return;
			}
		}

		FClusterAccumulator NewCluster;
		NewCluster.SumLocation = Location;
		NewCluster.Kind = Kind;
		NewCluster.BuildingCount = 1;
		NewCluster.ActiveCount = bIsActive ? 1 : 0;
		NewCluster.ItemCount = ItemCount;
		NewCluster.BuildingNames.Add(BuildingName, 1);
		Clusters.Add(MoveTemp(NewCluster));
	}

	FString FormatClusterLabel(const FClusterAccumulator& Cluster)
	{
		FString PrimaryName = TEXT("Building");
		int32 PrimaryCount = 0;
		for (const TPair<FString, int32>& Pair : Cluster.BuildingNames)
		{
			if (Pair.Value > PrimaryCount)
			{
				PrimaryName = Pair.Key;
				PrimaryCount = Pair.Value;
			}
		}

		const FString CountPrefix = Cluster.BuildingNames.Num() > 1
			? FString::Printf(TEXT("%dx %s+"), Cluster.BuildingCount, *PrimaryName)
			: FString::Printf(TEXT("%dx %s"), Cluster.BuildingCount, *PrimaryName);

		switch (Cluster.Kind)
		{
		case EMapFinderResultKind::Produced:
			return FString::Printf(TEXT("[Produced] %s (%d/%d active)"), *CountPrefix, Cluster.ActiveCount, Cluster.BuildingCount);
		case EMapFinderResultKind::Extracted:
			return FString::Printf(TEXT("[Extracted] %s (%d/%d active)"), *CountPrefix, Cluster.ActiveCount, Cluster.BuildingCount);
		case EMapFinderResultKind::DimensionalDepot:
			return FString::Printf(TEXT("[Depot] %s (%d items)"), *CountPrefix, Cluster.ItemCount);
		case EMapFinderResultKind::Stored:
		default:
			return FString::Printf(TEXT("[Stored] %s (%d items)"), *CountPrefix, Cluster.ItemCount);
		}
	}
}

TArray<FMapFinderProducedItem> FMapItemFinderScanner::ScanProducedItems(UWorld* World)
{
	TArray<FMapFinderProducedItem> OutItems;
	if (!World)
	{
		return OutItems;
	}

	AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(World);
	if (!BuildableSubsystem)
	{
		return OutItems;
	}

	TMap<TSubclassOf<UFGItemDescriptor>, bool> ItemToIsCrafted;

	TArray<AFGBuildableManufacturer*> Manufacturers;
	BuildableSubsystem->GetTypedBuildable<AFGBuildableManufacturer>(Manufacturers);
	for (AFGBuildableManufacturer* Manufacturer : Manufacturers)
	{
		if (!IsValid(Manufacturer))
		{
			continue;
		}

		const TSubclassOf<UFGRecipe> Recipe = Manufacturer->GetCurrentRecipe();
		if (!Recipe)
		{
			continue;
		}

		const TArray<FItemAmount> Products = UFGRecipe::GetProducts(Recipe);
		for (const FItemAmount& Product : Products)
		{
			if (Product.ItemClass)
			{
				ItemToIsCrafted.Add(Product.ItemClass, true);
			}
		}
	}

	TArray<AFGBuildableResourceExtractorBase*> Extractors;
	BuildableSubsystem->GetTypedBuildable<AFGBuildableResourceExtractorBase>(Extractors);
	for (AFGBuildableResourceExtractorBase* Extractor : Extractors)
	{
		if (!IsValid(Extractor))
		{
			continue;
		}

		const TScriptInterface<IFGExtractableResourceInterface> Resource = Extractor->GetExtractableResource();
		if (Resource.GetInterface())
		{
			if (TSubclassOf<UFGResourceDescriptor> ResourceClass = Resource->GetResourceClass())
			{
				if (!ItemToIsCrafted.Contains(ResourceClass))
				{
					ItemToIsCrafted.Add(ResourceClass, false);
				}
			}
		}
	}

	OutItems.Reserve(ItemToIsCrafted.Num());
	for (const TPair<TSubclassOf<UFGItemDescriptor>, bool>& Pair : ItemToIsCrafted)
	{
		FMapFinderProducedItem Entry;
		Entry.ItemClass = Pair.Key;
		Entry.bFromManufacturer = Pair.Value;
		OutItems.Add(Entry);
	}
	return OutItems;
}

TArray<FMapFinderResourceNode> FMapItemFinderScanner::ScanResourceNodes(UWorld* World)
{
	TArray<FMapFinderResourceNode> OutNodes;
	if (!World)
	{
		return OutNodes;
	}

	for (TActorIterator<AFGResourceNodeBase> It(World); It; ++It)
	{
		AFGResourceNodeBase* Node = *It;
		if (!IsValid(Node) || Node->IsA<AFGResourceDeposit>() || Node->IsA<AFGResourceNodeFrackingSatellite>())
		{
			continue;
		}

		const TSubclassOf<UFGResourceDescriptor> ResourceClass = Node->GetResourceClass();
		if (!ResourceClass)
		{
			continue;
		}

		FMapFinderResourceNode Entry;
		Entry.Location = Node->GetActorLocation();
		Entry.ResourceClass = ResourceClass;
		Entry.bOccupied = Node->IsOccupied();

		if (AFGResourceNode* ConcreteNode = Cast<AFGResourceNode>(Node))
		{
			Entry.Purity = static_cast<uint8>(ConcreteNode->GetResourcePurity());
		}
		else
		{
			Entry.Purity = 3;
		}

		OutNodes.Add(Entry);
	}

	return OutNodes;
}

TArray<FMapFinderMercerSphere> FMapItemFinderScanner::ScanMercerSpheres(UWorld* World)
{
	TArray<FMapFinderMercerSphere> OutSpheres;
	if (!World)
	{
		return OutSpheres;
	}

	auto IsMercerSphereClass = [](TSubclassOf<UFGItemDescriptor> ItemClass) -> bool
	{
		if (!ItemClass)
		{
			return false;
		}
		const FString ClassName = ItemClass->GetName();
		return ClassName.Contains(TEXT("WAT2")) || ItemClass->GetPathName().Contains(TEXT("Desc_WAT2"));
	};

	TSubclassOf<UFGItemDescriptor> MercerClass;

	for (TActorIterator<AFGItemPickup> It(World); It; ++It)
	{
		AFGItemPickup* Pickup = *It;
		if (!IsValid(Pickup) || Pickup->IsA<AFGItemPickup_Spawnable>() || Pickup->IsPickedUp())
		{
			continue;
		}

		const TSubclassOf<UFGItemDescriptor> ItemClass = Pickup->GetPickupItemClass();
		if (!IsMercerSphereClass(ItemClass))
		{
			continue;
		}

		if (!MercerClass)
		{
			MercerClass = ItemClass;
		}

		FMapFinderMercerSphere Entry;
		Entry.Location = Pickup->GetActorLocation();
		Entry.ItemClass = ItemClass;
		Entry.Count = FMath::Max(1, Pickup->GetNumItems());
		Entry.bInStorage = false;
		Entry.bInDepot = false;
		OutSpheres.Add(Entry);
	}

	if (AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(World))
	{
		TArray<AFGBuildableStorage*> Storages;
		BuildableSubsystem->GetTypedBuildable<AFGBuildableStorage>(Storages);
		for (AFGBuildableStorage* Storage : Storages)
		{
			if (!IsValid(Storage))
			{
				continue;
			}

			UFGInventoryComponent* Inventory = Storage->GetStorageInventory();
			if (!IsValid(Inventory))
			{
				continue;
			}

			int32 Count = 0;
			TSubclassOf<UFGItemDescriptor> FoundClass = MercerClass;
			if (MercerClass)
			{
				Count = Inventory->GetNumItems(MercerClass);
			}
			else
			{
				TArray<FInventoryStack> Stacks;
				Inventory->GetInventoryStacks(Stacks);
				for (const FInventoryStack& Stack : Stacks)
				{
					const TSubclassOf<UFGItemDescriptor> StackClass = Stack.Item.GetItemClass();
					if (IsMercerSphereClass(StackClass))
					{
						FoundClass = StackClass;
						MercerClass = StackClass;
						Count += Stack.NumItems;
					}
				}
			}

			if (Count <= 0)
			{
				continue;
			}

			FMapFinderMercerSphere Entry;
			Entry.Location = Storage->GetActorLocation();
			Entry.ItemClass = FoundClass;
			Entry.Count = Count;
			Entry.bInStorage = true;
			Entry.bInDepot = Storage->IsA<AFGCentralStorageContainer>();
			OutSpheres.Add(Entry);
		}
	}

	return OutSpheres;
}

TArray<FMapFinderCluster> FMapItemFinderScanner::ScanWorldForItem(
	UWorld* World,
	TSubclassOf<UFGItemDescriptor> ItemClass,
	float ClusterRadiusCm,
	int32 MaxClusters)
{
	TArray<FMapFinderCluster> OutClusters;
	if (!World || !ItemClass)
	{
		return OutClusters;
	}

	AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(World);
	if (!BuildableSubsystem)
	{
		return OutClusters;
	}

	const float ClusterRadiusSq = FMath::Square(ClusterRadiusCm);
	TArray<FClusterAccumulator> Accumulators;

	TArray<AFGBuildableManufacturer*> Manufacturers;
	BuildableSubsystem->GetTypedBuildable<AFGBuildableManufacturer>(Manufacturers);
	for (AFGBuildableManufacturer* Manufacturer : Manufacturers)
	{
		if (!IsValid(Manufacturer))
		{
			continue;
		}

		const TSubclassOf<UFGRecipe> Recipe = Manufacturer->GetCurrentRecipe();
		if (!Recipe)
		{
			continue;
		}

		bool bProducesItem = false;
		const TArray<FItemAmount> Products = UFGRecipe::GetProducts(Recipe);
		for (const FItemAmount& Product : Products)
		{
			if (Product.ItemClass == ItemClass)
			{
				bProducesItem = true;
				break;
			}
		}

		if (bProducesItem)
		{
			const FString Name = Manufacturer->mDisplayName.IsEmpty()
				? Manufacturer->GetName()
				: Manufacturer->mDisplayName.ToString();
			AddToClusters(
				Accumulators,
				Manufacturer->GetActorLocation(),
				EMapFinderResultKind::Produced,
				Name,
				Manufacturer->IsProducing(),
				0,
				ClusterRadiusSq);
		}
	}

	TArray<AFGBuildableResourceExtractorBase*> Extractors;
	BuildableSubsystem->GetTypedBuildable<AFGBuildableResourceExtractorBase>(Extractors);
	for (AFGBuildableResourceExtractorBase* Extractor : Extractors)
	{
		if (!IsValid(Extractor))
		{
			continue;
		}

		const TScriptInterface<IFGExtractableResourceInterface> Resource = Extractor->GetExtractableResource();
		if (!Resource.GetInterface())
		{
			continue;
		}

		if (Resource->GetResourceClass() == ItemClass)
		{
			const FString Name = Extractor->mDisplayName.IsEmpty()
				? Extractor->GetName()
				: Extractor->mDisplayName.ToString();
			AddToClusters(
				Accumulators,
				Extractor->GetActorLocation(),
				EMapFinderResultKind::Extracted,
				Name,
				Extractor->IsProducing(),
				0,
				ClusterRadiusSq);
		}
	}

	TArray<AFGBuildableStorage*> Storages;
	BuildableSubsystem->GetTypedBuildable<AFGBuildableStorage>(Storages);
	for (AFGBuildableStorage* Storage : Storages)
	{
		if (!IsValid(Storage))
		{
			continue;
		}

		UFGInventoryComponent* Inventory = Storage->GetStorageInventory();
		if (!IsValid(Inventory))
		{
			continue;
		}

		const int32 Count = Inventory->GetNumItems(ItemClass);
		if (Count <= 0)
		{
			continue;
		}

		const bool bIsDepot = Storage->IsA<AFGCentralStorageContainer>();
		const FString Name = Storage->mDisplayName.IsEmpty()
			? Storage->GetName()
			: Storage->mDisplayName.ToString();
		AddToClusters(
			Accumulators,
			Storage->GetActorLocation(),
			bIsDepot ? EMapFinderResultKind::DimensionalDepot : EMapFinderResultKind::Stored,
			Name,
			true,
			Count,
			ClusterRadiusSq);
	}

	Accumulators.Sort([](const FClusterAccumulator& A, const FClusterAccumulator& B)
	{
		if (A.BuildingCount != B.BuildingCount)
		{
			return A.BuildingCount > B.BuildingCount;
		}
		return A.ItemCount > B.ItemCount;
	});

	const int32 ResultCount = FMath::Min(Accumulators.Num(), MaxClusters);
	OutClusters.Reserve(ResultCount);

	for (int32 Index = 0; Index < ResultCount; ++Index)
	{
		const FClusterAccumulator& Acc = Accumulators[Index];
		FMapFinderCluster Cluster;
		Cluster.Location = Acc.GetCentroid();
		Cluster.Kind = Acc.Kind;
		Cluster.BuildingCount = Acc.BuildingCount;
		Cluster.ActiveCount = Acc.ActiveCount;
		Cluster.ItemCount = Acc.ItemCount;
		Cluster.Label = FormatClusterLabel(Acc);
		OutClusters.Add(MoveTemp(Cluster));
	}

	return OutClusters;
}
