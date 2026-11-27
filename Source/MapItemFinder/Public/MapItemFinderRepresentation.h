#pragma once

#include "CoreMinimal.h"
#include "MapItemFinderTypes.h"
#include "Representation/FGMapMarkerRepresentation.h"
#include "Resources/FGItemDescriptor.h"
#include "MapItemFinderRepresentation.generated.h"

UCLASS()
class MAPITEMFINDER_API UMapItemFinderRepresentation : public UFGMapMarkerRepresentation
{
	GENERATED_BODY()

public:
	void InitializePin(
		UWorld* World,
		TSubclassOf<UFGItemDescriptor> InItemClass,
		const FMapFinderCluster& InCluster);

	void InitializeResourceNodePin(
		UWorld* World,
		const FMapFinderResourceNode& InNode);

	void InitializeMercerSpherePin(
		UWorld* World,
		const FMapFinderMercerSphere& InSphere);

	virtual FVector GetActorLocation() const override;
	virtual UTexture2D* GetRepresentationTexture() const override;
	virtual FText GetRepresentationText() const override;
	virtual FLinearColor GetRepresentationColor() const override;
	virtual bool GetShouldShowInCompass() const override;
	virtual bool GetShouldShowOnMap() const override;
	virtual float GetScaleOnMap() const override;

private:
	static int32 ResolveIconID(UWorld* World, TSubclassOf<UFGItemDescriptor> InItemClass, UTexture2D* IconTexture);

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> ItemClass;

	UPROPERTY()
	TObjectPtr<UTexture2D> CachedIconTexture;

	FText PinText;
	FLinearColor PinColor = FLinearColor::White;
	float PinScale = 1.25f;
};
