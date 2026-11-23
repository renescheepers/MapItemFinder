#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "MapItemFinderTypes.h"
#include "Resources/FGItemDescriptor.h"
#include "MapItemFinderRCO.generated.h"

UCLASS()
class MAPITEMFINDER_API UMapItemFinderRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SearchItem(TSubclassOf<UFGItemDescriptor> ItemClass, int32 RequestId);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveSearchResults(
		TSubclassOf<UFGItemDescriptor> ItemClass,
		int32 RequestId,
		const TArray<FMapFinderCluster>& Results);

private:
	// Required so UE replicates this RCO to clients
	UPROPERTY(Replicated)
	bool bForceNetField = false;
};
