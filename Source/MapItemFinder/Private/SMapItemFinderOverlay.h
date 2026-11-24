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
	void UpdateFilteredItems(const FString& Query, bool bOpenIfNonEmpty);
	void SelectItem(const TSharedPtr<FMapFinderItemEntry>& Entry);

	void HandleProducedItemsUpdated(
		const TArray<FMapFinderProducedItem>& ProducedItems,
		bool bFromServer);
	void HandleResultsUpdated(
		TSubclassOf<UFGItemDescriptor> ItemClass,
		const TArray<FMapFinderCluster>& Results,
		bool bFromServer);

	FReply OnNormalModeClicked();
	FReply OnFinderModeClicked();
	FReply OnToggleDropdownClicked();
	FReply OnClearClicked();

	void OnSearchTextChanged(const FText& NewText);
	void OnSearchTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	TSharedRef<ITableRow> OnGenerateItemRow(
		TSharedPtr<FMapFinderItemEntry> Entry,
		const TSharedRef<STableViewBase>& OwnerTable);
	void OnItemSelectionChanged(TSharedPtr<FMapFinderItemEntry> Entry, ESelectInfo::Type SelectInfo);
	void OnProducedCheckChanged(ECheckBoxState NewState);
	void OnStoredCheckChanged(ECheckBoxState NewState);

	EVisibility GetFinderPanelVisibility() const;
	EVisibility GetDropdownVisibility() const;
	FSlateColor GetNormalButtonColor() const;
	FSlateColor GetFinderButtonColor() const;

	TWeakObjectPtr<UWorld> WorldPtr;
	TWeakObjectPtr<UMapItemFinderSubsystem> SubsystemPtr;
	FDelegateHandle ResultsDelegateHandle;
	FDelegateHandle ProducedItemsDelegateHandle;

	TArray<TSharedPtr<FMapFinderItemEntry>> ProducedItemEntries;
	TArray<TSharedPtr<FMapFinderItemEntry>> FilteredItems;

	TSharedPtr<SEditableTextBox> SearchBox;
	TSharedPtr<SListView<TSharedPtr<FMapFinderItemEntry>>> ResultsListView;
	TSharedPtr<STextBlock> StatusText;

	bool bDropdownOpen = false;
};

#endif
