#pragma once

#if !UE_SERVER

#include "CoreMinimal.h"
#include "MapItemFinderTypes.h"
#include "Resources/FGItemDescriptor.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UMapItemFinderSubsystem;
class SEditableTextBox;
class STextBlock;

struct FMapFinderItemEntry
{
	TSubclassOf<UFGItemDescriptor> ItemClass;
	FString DisplayName;
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
	void PopulateItems();
	void UpdateFilteredItems(const FString& Query);
	void SelectItem(const TSharedPtr<FMapFinderItemEntry>& Entry);
	void HandleResultsUpdated(
		TSubclassOf<UFGItemDescriptor> ItemClass,
		const TArray<FMapFinderCluster>& Results,
		bool bFromServer);

	void OnSearchTextChanged(const FText& NewText);
	void OnSearchTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	TSharedRef<ITableRow> OnGenerateItemRow(
		TSharedPtr<FMapFinderItemEntry> Entry,
		const TSharedRef<STableViewBase>& OwnerTable);
	void OnItemSelectionChanged(TSharedPtr<FMapFinderItemEntry> Entry, ESelectInfo::Type SelectInfo);
	FReply OnClearClicked();
	void OnProducedCheckChanged(ECheckBoxState NewState);
	void OnStoredCheckChanged(ECheckBoxState NewState);
	EVisibility GetDropdownVisibility() const;

	TWeakObjectPtr<UWorld> WorldPtr;
	TWeakObjectPtr<UMapItemFinderSubsystem> SubsystemPtr;
	FDelegateHandle ResultsDelegateHandle;

	TArray<TSharedPtr<FMapFinderItemEntry>> AllItems;
	TArray<TSharedPtr<FMapFinderItemEntry>> FilteredItems;

	TSharedPtr<SEditableTextBox> SearchBox;
	TSharedPtr<SListView<TSharedPtr<FMapFinderItemEntry>>> ResultsListView;
	TSharedPtr<STextBlock> StatusText;

	bool bDropdownOpen = false;
};

#endif
