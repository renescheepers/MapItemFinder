#if !UE_SERVER

#include "SMapItemFinderOverlay.h"

#include "MapItemFinderSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

void SMapItemFinderOverlay::Construct(const FArguments& InArgs)
{
	SetVisibility(EVisibility::SelfHitTestInvisible);
	WorldPtr = InArgs._World;
	SubsystemPtr = InArgs._Subsystem;

	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		ResultsDelegateHandle = Subsystem->OnResultsUpdated.AddSP(
			this, &SMapItemFinderOverlay::HandleResultsUpdated);
		ProducedItemsDelegateHandle = Subsystem->OnProducedItemsUpdated.AddSP(
			this, &SMapItemFinderOverlay::HandleProducedItemsUpdated);
		RebuildProducedItemEntries(Subsystem->GetCachedProducedItems());
	}

	const bool bInitialProduced = SubsystemPtr.IsValid() ? SubsystemPtr->GetShowProduced() : true;
	const bool bInitialStored = SubsystemPtr.IsValid() ? SubsystemPtr->GetShowStored() : true;

	ChildSlot
	[
		SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 16.0f, 0.0f, 0.0f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor(FLinearColor(0.04f, 0.05f, 0.07f, 0.92f))
				.Padding(FMargin(6.0f, 4.0f))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
					[
						SNew(SButton)
						.ButtonColorAndOpacity(this, &SMapItemFinderOverlay::GetNormalButtonColor)
						.OnClicked(this, &SMapItemFinderOverlay::OnNormalModeClicked)
						.ContentPadding(FMargin(14.0f, 4.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Normal")))
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonColorAndOpacity(this, &SMapItemFinderOverlay::GetFinderButtonColor)
						.OnClicked(this, &SMapItemFinderOverlay::OnFinderModeClicked)
						.ContentPadding(FMargin(14.0f, 4.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Finder")))
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
			[
				SNew(SBox)
				.WidthOverride(440.0f)
				.Visibility(this, &SMapItemFinderOverlay::GetFinderPanelVisibility)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.BorderBackgroundColor(FLinearColor(0.04f, 0.05f, 0.07f, 0.92f))
					.Padding(FMargin(10.0f, 8.0f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SAssignNew(SearchBox, SEditableTextBox)
								.HintText(FText::FromString(TEXT("Search produced items...")))
								.OnTextChanged(this, &SMapItemFinderOverlay::OnSearchTextChanged)
								.OnTextCommitted(this, &SMapItemFinderOverlay::OnSearchTextCommitted)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
							.VAlign(VAlign_Center)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("v")))
								.ToolTipText(FText::FromString(TEXT("Show all produced items")))
								.OnClicked(this, &SMapItemFinderOverlay::OnToggleDropdownClicked)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
							.VAlign(VAlign_Center)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Clear")))
								.OnClicked(this, &SMapItemFinderOverlay::OnClearClicked)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(FMargin(0.0f, 0.0f, 14.0f, 0.0f))
							[
								SNew(SCheckBox)
								.IsChecked(bInitialProduced ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
								.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnProducedCheckChanged)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Produced")))
									.ColorAndOpacity(FLinearColor(0.35f, 0.95f, 0.50f, 1.0f))
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SCheckBox)
								.IsChecked(bInitialStored ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
								.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnStoredCheckChanged)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Stored / Depot")))
									.ColorAndOpacity(FLinearColor(1.00f, 0.72f, 0.25f, 1.0f))
								]
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
						[
							SAssignNew(StatusText, STextBlock)
							.ColorAndOpacity(FLinearColor(0.80f, 0.84f, 0.90f, 1.0f))
							.AutoWrapText(true)
							.Text(FText::GetEmpty())
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.MaxHeight(240.0f)
						.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
						[
							SNew(SBox)
							.Visibility(this, &SMapItemFinderOverlay::GetDropdownVisibility)
							[
								SAssignNew(ResultsListView, SListView<TSharedPtr<FMapFinderItemEntry>>)
								.ListItemsSource(&FilteredItems)
								.OnGenerateRow(this, &SMapItemFinderOverlay::OnGenerateItemRow)
								.OnSelectionChanged(this, &SMapItemFinderOverlay::OnItemSelectionChanged)
								.SelectionMode(ESelectionMode::Single)
							]
						]
					]
				]
			]
		]
	];

	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		if (Subsystem->IsFinderModeActive() && Subsystem->GetActiveItemClass())
		{
			HandleResultsUpdated(Subsystem->GetActiveItemClass(), Subsystem->GetLastResults(), true);
		}
	}
}

SMapItemFinderOverlay::~SMapItemFinderOverlay()
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		if (ResultsDelegateHandle.IsValid())
		{
			Subsystem->OnResultsUpdated.Remove(ResultsDelegateHandle);
		}
		if (ProducedItemsDelegateHandle.IsValid())
		{
			Subsystem->OnProducedItemsUpdated.Remove(ProducedItemsDelegateHandle);
		}
	}
}

void SMapItemFinderOverlay::RebuildProducedItemEntries(
	const TArray<TSubclassOf<UFGItemDescriptor>>& ProducedItems)
{
	ProducedItemEntries.Empty();

	TSet<TSubclassOf<UFGItemDescriptor>> Seen;
	for (const TSubclassOf<UFGItemDescriptor>& Descriptor : ProducedItems)
	{
		if (!Descriptor || Seen.Contains(Descriptor))
		{
			continue;
		}
		Seen.Add(Descriptor);

		const FString Name = UFGItemDescriptor::GetItemName(Descriptor).ToString();
		if (Name.IsEmpty())
		{
			continue;
		}

		TSharedPtr<FMapFinderItemEntry> Entry = MakeShared<FMapFinderItemEntry>();
		Entry->ItemClass = Descriptor;
		Entry->DisplayName = Name;
		ProducedItemEntries.Add(Entry);
	}

	ProducedItemEntries.Sort([](const TSharedPtr<FMapFinderItemEntry>& A, const TSharedPtr<FMapFinderItemEntry>& B)
	{
		return A->DisplayName < B->DisplayName;
	});

	const FString CurrentQuery = SearchBox.IsValid() ? SearchBox->GetText().ToString() : FString();
	UpdateFilteredItems(CurrentQuery, bDropdownOpen);
}

void SMapItemFinderOverlay::UpdateFilteredItems(const FString& Query, bool bOpenIfNonEmpty)
{
	FilteredItems.Empty();
	const FString Trimmed = Query.TrimStartAndEnd();

	for (const TSharedPtr<FMapFinderItemEntry>& Entry : ProducedItemEntries)
	{
		if (Trimmed.IsEmpty() || Entry->DisplayName.Contains(Trimmed, ESearchCase::IgnoreCase))
		{
			FilteredItems.Add(Entry);
		}
	}

	bDropdownOpen = bOpenIfNonEmpty && FilteredItems.Num() > 0;
	if (ResultsListView.IsValid())
	{
		ResultsListView->RequestListRefresh();
	}
}

void SMapItemFinderOverlay::SelectItem(const TSharedPtr<FMapFinderItemEntry>& Entry)
{
	if (!Entry.IsValid())
	{
		return;
	}

	bDropdownOpen = false;
	if (SearchBox.IsValid())
	{
		SearchBox->SetText(FText::FromString(Entry->DisplayName));
	}
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("Searching for %s..."), *Entry->DisplayName)));
	}

	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->RequestSearch(WorldPtr.Get(), Entry->ItemClass);
	}
}

void SMapItemFinderOverlay::HandleProducedItemsUpdated(
	const TArray<TSubclassOf<UFGItemDescriptor>>& ProducedItems,
	bool bFromServer)
{
	RebuildProducedItemEntries(ProducedItems);

	if (StatusText.IsValid() && (!SubsystemPtr.IsValid() || !SubsystemPtr->GetActiveItemClass()))
	{
		const FString ModeSuffix = bFromServer ? TEXT("") : TEXT(" (local client only)");
		StatusText->SetText(FText::FromString(
			FString::Printf(TEXT("%d produced item(s) available%s"), ProducedItemEntries.Num(), *ModeSuffix)));
	}
}

void SMapItemFinderOverlay::HandleResultsUpdated(
	TSubclassOf<UFGItemDescriptor> ItemClass,
	const TArray<FMapFinderCluster>& Results,
	bool bFromServer)
{
	if (!StatusText.IsValid())
	{
		return;
	}

	int32 ProdSites = 0;
	int32 ProdMachines = 0;
	int32 StorageSites = 0;
	int32 StoredItems = 0;

	for (const FMapFinderCluster& Cluster : Results)
	{
		if (Cluster.Kind == EMapFinderResultKind::Produced || Cluster.Kind == EMapFinderResultKind::Extracted)
		{
			ProdSites += 1;
			ProdMachines += Cluster.BuildingCount;
		}
		else
		{
			StorageSites += 1;
			StoredItems += Cluster.ItemCount;
		}
	}

	const FString ItemName = ItemClass ? UFGItemDescriptor::GetItemName(ItemClass).ToString() : TEXT("Item");
	const FString ModeSuffix = bFromServer ? TEXT("") : TEXT(" (local client only)");

	if (Results.Num() == 0)
	{
		StatusText->SetText(FText::FromString(
			FString::Printf(TEXT("%s: no production or storage found%s"), *ItemName, *ModeSuffix)));
		return;
	}

	StatusText->SetText(FText::FromString(FString::Printf(
		TEXT("%s: %d prod site(s) (%d machines), %d storage site(s) (%d items)%s"),
		*ItemName, ProdSites, ProdMachines, StorageSites, StoredItems, *ModeSuffix)));
}

FReply SMapItemFinderOverlay::OnNormalModeClicked()
{
	bDropdownOpen = false;
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->SetFinderMode(WorldPtr.Get(), false);
	}
	return FReply::Handled();
}

FReply SMapItemFinderOverlay::OnFinderModeClicked()
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		if (StatusText.IsValid() && !Subsystem->GetActiveItemClass())
		{
			StatusText->SetText(FText::FromString(TEXT("Loading produced items...")));
		}
		Subsystem->SetFinderMode(WorldPtr.Get(), true);
	}
	return FReply::Handled();
}

FReply SMapItemFinderOverlay::OnToggleDropdownClicked()
{
	if (bDropdownOpen)
	{
		bDropdownOpen = false;
		return FReply::Handled();
	}

	const FString CurrentQuery = SearchBox.IsValid() ? SearchBox->GetText().ToString() : FString();
	UpdateFilteredItems(CurrentQuery, true);
	return FReply::Handled();
}

void SMapItemFinderOverlay::OnSearchTextChanged(const FText& NewText)
{
	const FString Query = NewText.ToString();
	UpdateFilteredItems(Query, !Query.TrimStartAndEnd().IsEmpty());
}

void SMapItemFinderOverlay::OnSearchTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter && FilteredItems.Num() > 0)
	{
		SelectItem(FilteredItems[0]);
	}
}

TSharedRef<ITableRow> SMapItemFinderOverlay::OnGenerateItemRow(
	TSharedPtr<FMapFinderItemEntry> Entry,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FMapFinderItemEntry>>, OwnerTable)
		.Padding(FMargin(6.0f, 3.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Entry.IsValid() ? Entry->DisplayName : TEXT("")))
		];
}

void SMapItemFinderOverlay::OnItemSelectionChanged(
	TSharedPtr<FMapFinderItemEntry> Entry,
	ESelectionInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct && Entry.IsValid())
	{
		SelectItem(Entry);
	}
}

FReply SMapItemFinderOverlay::OnClearClicked()
{
	bDropdownOpen = false;
	if (SearchBox.IsValid())
	{
		SearchBox->SetText(FText::GetEmpty());
	}
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->ClearPins(WorldPtr.Get());
	}
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(
			FString::Printf(TEXT("%d produced item(s) available"), ProducedItemEntries.Num())));
	}
	return FReply::Handled();
}

void SMapItemFinderOverlay::OnProducedCheckChanged(ECheckBoxState NewState)
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->SetFilters(WorldPtr.Get(), NewState == ECheckBoxState::Checked, Subsystem->GetShowStored());
	}
}

void SMapItemFinderOverlay::OnStoredCheckChanged(ECheckBoxState NewState)
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->SetFilters(WorldPtr.Get(), Subsystem->GetShowProduced(), NewState == ECheckBoxState::Checked);
	}
}

EVisibility SMapItemFinderOverlay::GetFinderPanelVisibility() const
{
	return (SubsystemPtr.IsValid() && SubsystemPtr->IsFinderModeActive())
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

EVisibility SMapItemFinderOverlay::GetDropdownVisibility() const
{
	return bDropdownOpen ? EVisibility::Visible : EVisibility::Collapsed;
}

FSlateColor SMapItemFinderOverlay::GetNormalButtonColor() const
{
	const bool bIsNormal = !SubsystemPtr.IsValid() || !SubsystemPtr->IsFinderModeActive();
	return bIsNormal
		? FSlateColor(FLinearColor(0.95f, 0.55f, 0.12f, 1.0f))
		: FSlateColor(FLinearColor(0.22f, 0.25f, 0.30f, 1.0f));
}

FSlateColor SMapItemFinderOverlay::GetFinderButtonColor() const
{
	const bool bIsFinder = SubsystemPtr.IsValid() && SubsystemPtr->IsFinderModeActive();
	return bIsFinder
		? FSlateColor(FLinearColor(0.95f, 0.55f, 0.12f, 1.0f))
		: FSlateColor(FLinearColor(0.22f, 0.25f, 0.30f, 1.0f));
}

#endif
