#pragma once

#include "CoreMinimal.h"
#include "FGActorRepresentation.h"
#include "MapItemFinderTypes.h"
#include "Resources/FGItemDescriptor.h"
#include "MapItemFinderRepresentation.generated.h"

UCLASS()
class MAPITEMFINDER_API UMapItemFinderRepresentation : public UFGActorRepresentation
{
	GENERATED_BODY()

public:
	void InitializePin(TSubclassOf<UFGItemDescriptor> InItemClass, const FMapFinderCluster& InCluster);
	void InitializeResourceNodePin(const FMapFinderResourceNode& InNode);

	virtual FVector GetActorLocation() const override;
	virtual UTexture2D* GetRepresentationTexture() const override;
	virtual FText GetRepresentationText() const override;
	virtual FLinearColor GetRepresentationColor() const override;
	virtual ERepresentationType GetRepresentationType() const override;
	virtual bool GetShouldShowInCompass() const override;
	virtual bool GetShouldShowOnMap() const override;
	virtual float GetScaleOnMap() const override;

private:
	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> ItemClass;

	EMapFinderResultKind ResultKind = EMapFinderResultKind::Produced;
	FText PinText;
	FLinearColor PinColor = FLinearColor::White;
	float PinScale = 1.35f;
};
