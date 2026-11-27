#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "Resources/FGItemDescriptor.h"
#include "MapItemFinderTypes.generated.h"

UENUM(BlueprintType)
enum class EMapFinderResultKind : uint8
{
	Produced,
	Extracted,
	Stored,
	DimensionalDepot
};

USTRUCT(BlueprintType)
struct MAPITEMFINDER_API FMapFinderCluster
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY()
	EMapFinderResultKind Kind = EMapFinderResultKind::Produced;

	UPROPERTY()
	int32 BuildingCount = 0;

	UPROPERTY()
	int32 ActiveCount = 0;

	UPROPERTY()
	int32 ItemCount = 0;

	UPROPERTY()
	FString Label;
};

USTRUCT(BlueprintType)
struct MAPITEMFINDER_API FMapFinderProducedItem
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> ItemClass;

	UPROPERTY()
	bool bFromManufacturer = false;
};

USTRUCT(BlueprintType)
struct MAPITEMFINDER_API FMapFinderResourceNode
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> ResourceClass;

	UPROPERTY()
	uint8 Purity = 3;

	UPROPERTY()
	bool bOccupied = false;
};

USTRUCT(BlueprintType)
struct MAPITEMFINDER_API FMapFinderMercerSphere
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> ItemClass;

	UPROPERTY()
	int32 Count = 1;

	UPROPERTY()
	bool bInStorage = false;

	UPROPERTY()
	bool bInDepot = false;
};
