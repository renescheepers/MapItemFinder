#pragma once

#if !UE_SERVER

#include "CoreMinimal.h"
#include "MapItemFinderTypes.h"
#include "Resources/FGItemDescriptor.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UMapItemFinderSubsystem;
class SEditableTextBox;
class STextBlock;
class SWrapBox;

struct FMapFinderItemEntry
{
	bool bIsHeader = false;
	TSubclassOf<UFGItemDescriptor> ItemClass;
	FString DisplayName;
	FString CategoryName;
	float CategoryPriority = 0.0f;
	FSlateBrush IconBrush;
};

class SMapItemFinderOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMapItemFinderOverlay) {}
		SLATE_ARGUMENT(UWorld*, World)
		SLATE_ARGUMENT(UMapItemFinderSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMapItemFinderOverlay() override;

private:
	void RebuildProducedItemEntries(const TArray<FMapFinderProducedItem>& ProducedItems);
	void RebuildResourceNodeFilterBar(const TArray<FMapFinderResourceNode>& Nodes);
	void UpdateFilteredItems(const FString& Query);
	void ToggleItem(const TSharedPtr<FMapFinderItemEntry>& Entry);
	void RefreshStatusText(bool bFromServer);

	void HandleProducedItemsUpdated(
		const TArray<FMapFinderProducedItem>& ProducedItems,
		bool bFromServer);
	void HandleResourceNodesUpdated(
		const TArray<FMapFinderResourceNode>& Nodes,
		bool bFromServer);
	void HandleResultsUpdated(bool bFromServer);

	FReply OnToggleFinderModeClicked();
	FReply OnClearClicked();
	FReply OnSelectNodeResourceFilter(TSubclassOf<UFGItemDescriptor> ResourceClass);
	FReply OnClearNodeResourceFilterClicked();

	void OnSearchTextChanged(const FText& NewText);
	void OnSearchTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	TSharedRef<ITableRow> OnGenerateItemRow(
		TSharedPtr<FMapFinderItemEntry> Entry,
		const TSharedRef<STableViewBase>& OwnerTable);
	void OnItemRowClicked(TSharedPtr<FMapFinderItemEntry> Entry);
	void OnItemCheckboxChanged(ECheckBoxState NewState, TSubclassOf<UFGItemDescriptor> ItemClass);
	ECheckBoxState IsItemChecked(TSubclassOf<UFGItemDescriptor> ItemClass) const;
	void OnProducedCheckChanged(ECheckBoxState NewState);
	void OnStoredCheckChanged(ECheckBoxState NewState);
	void OnResourceNodesCheckChanged(ECheckBoxState NewState);
	void OnMercerSpheresCheckChanged(ECheckBoxState NewState);

	EVisibility GetFinderPanelVisibility() const;
	EVisibility GetResourceNodeFilterVisibility() const;
	FText GetToggleFinderButtonText() const;
	FSlateColor GetToggleFinderButtonColor() const;
	FSlateColor GetNodeFilterButtonColor(TSubclassOf<UFGItemDescriptor> ResourceClass) const;

	TWeakObjectPtr<UWorld> WorldPtr;
	TWeakObjectPtr<UMapItemFinderSubsystem> SubsystemPtr;
	FDelegateHandle ResultsDelegateHandle;
	FDelegateHandle ProducedItemsDelegateHandle;
	FDelegateHandle ResourceNodesDelegateHandle;

	TArray<TSharedPtr<FMapFinderItemEntry>> ProducedItemEntries;
	TArray<TSharedPtr<FMapFinderItemEntry>> FilteredItems;

	TSharedPtr<SEditableTextBox> SearchBox;
	TSharedPtr<SListView<TSharedPtr<FMapFinderItemEntry>>> ResultsListView;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SWrapBox> ResourceNodeFilterBox;
};

#endif
