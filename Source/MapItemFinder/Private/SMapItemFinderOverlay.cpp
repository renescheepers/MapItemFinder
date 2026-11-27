#if !UE_SERVER

#include "SMapItemFinderOverlay.h"

#include "FGCategory.h"
#include "MapItemFinderSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
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
		ResourceNodesDelegateHandle = Subsystem->OnResourceNodesUpdated.AddSP(
			this, &SMapItemFinderOverlay::HandleResourceNodesUpdated);
		RebuildProducedItemEntries(Subsystem->GetCachedProducedItems());
	}

	const bool bInitialProduced = SubsystemPtr.IsValid() ? SubsystemPtr->GetShowProduced() : true;
	const bool bInitialStored = SubsystemPtr.IsValid() ? SubsystemPtr->GetShowStored() : true;
	const bool bInitialNodes = SubsystemPtr.IsValid() ? SubsystemPtr->GetShowResourceNodes() : false;
	const bool bInitialMercer = SubsystemPtr.IsValid() ? SubsystemPtr->GetShowMercerSpheres() : false;

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
				.WidthOverride(540.0f)
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
								.HintText(FText::FromString(TEXT("Search & toggle produced items...")))
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
							.Padding(FMargin(0.0f, 0.0f, 14.0f, 0.0f))
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

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(FMargin(0.0f, 0.0f, 14.0f, 0.0f))
							[
								SNew(SCheckBox)
								.IsChecked(bInitialNodes ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
								.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnResourceNodesCheckChanged)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Resource Nodes")))
									.ColorAndOpacity(FLinearColor(0.45f, 0.85f, 1.00f, 1.0f))
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SCheckBox)
								.IsChecked(bInitialMercer ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
								.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnMercerSpheresCheckChanged)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Mercer Spheres")))
									.ColorAndOpacity(FLinearColor(0.88f, 0.45f, 1.00f, 1.0f))
								]
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
						[
							SAssignNew(ResourceNodeFilterBox, SWrapBox)
							.UseAllottedSize(true)
							.InnerSlotPadding(FVector2D(4.0f, 4.0f))
							.Visibility(this, &SMapItemFinderOverlay::GetResourceNodeFilterVisibility)
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
						.MaxHeight(280.0f)
						.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
						[
							SNew(SBox)
							.Visibility(this, &SMapItemFinderOverlay::GetDropdownVisibility)
							[
								SAssignNew(ResultsListView, SListView<TSharedPtr<FMapFinderItemEntry>>)
								.ListItemsSource(&FilteredItems)
								.OnGenerateRow(this, &SMapItemFinderOverlay::OnGenerateItemRow)
								.OnMouseButtonClick(this, &SMapItemFinderOverlay::OnItemRowClicked)
								.SelectionMode(ESelectionMode::None)
							]
						]
					]
				]
			]
		]
	];

	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		if (Subsystem->GetCachedResourceNodes().Num() > 0)
		{
			RebuildResourceNodeFilterBar(Subsystem->GetCachedResourceNodes());
		}
		if (Subsystem->IsFinderModeActive() && Subsystem->GetSelectedItemClasses().Num() > 0)
		{
			RefreshStatusText(true);
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
		if (ResourceNodesDelegateHandle.IsValid())
		{
			Subsystem->OnResourceNodesUpdated.Remove(ResourceNodesDelegateHandle);
		}
	}
}

void SMapItemFinderOverlay::RebuildProducedItemEntries(
	const TArray<FMapFinderProducedItem>& ProducedItems)
{
	ProducedItemEntries.Empty();

	TSet<TSubclassOf<UFGItemDescriptor>> Seen;
	for (const FMapFinderProducedItem& Item : ProducedItems)
	{
		if (!Item.ItemClass || Seen.Contains(Item.ItemClass))
		{
			continue;
		}
		Seen.Add(Item.ItemClass);

		const FString Name = UFGItemDescriptor::GetItemName(Item.ItemClass).ToString();
		if (Name.IsEmpty())
		{
			continue;
		}

		FString CategoryName;
		float CategoryPriority = 500000.0f;

		if (!Item.bFromManufacturer)
		{
			CategoryName = TEXT("Extracted Resources");
			CategoryPriority = 999999.0f;
		}
		else if (TSubclassOf<UFGCategory> CategoryClass = UFGItemDescriptor::GetCategory(Item.ItemClass))
		{
			CategoryName = UFGCategory::GetCategoryName(CategoryClass).ToString();
			CategoryPriority = UFGCategory::GetMenuPriority(CategoryClass);
		}

		if (CategoryName.IsEmpty())
		{
			CategoryName = TEXT("Other Recipes");
		}

		TSharedPtr<FMapFinderItemEntry> Entry = MakeShared<FMapFinderItemEntry>();
		Entry->bIsHeader = false;
		Entry->ItemClass = Item.ItemClass;
		Entry->DisplayName = Name;
		Entry->CategoryName = CategoryName;
		Entry->CategoryPriority = CategoryPriority;

		UTexture2D* IconTex = UFGItemDescriptor::GetSmallIcon(Item.ItemClass);
		if (!IconTex)
		{
			IconTex = UFGItemDescriptor::GetBigIcon(Item.ItemClass);
		}
		if (IconTex)
		{
			Entry->IconBrush.SetResourceObject(IconTex);
			Entry->IconBrush.SetImageSize(FVector2D(20.0, 20.0));
			Entry->IconBrush.DrawAs = ESlateBrushDrawType::Image;
		}
		else
		{
			Entry->IconBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		}

		ProducedItemEntries.Add(Entry);
	}

	ProducedItemEntries.Sort([](const TSharedPtr<FMapFinderItemEntry>& A, const TSharedPtr<FMapFinderItemEntry>& B)
	{
		if (!FMath::IsNearlyEqual(A->CategoryPriority, B->CategoryPriority))
		{
			return A->CategoryPriority < B->CategoryPriority;
		}
		if (A->CategoryName != B->CategoryName)
		{
			return A->CategoryName < B->CategoryName;
		}
		return A->DisplayName < B->DisplayName;
	});

	const FString CurrentQuery = SearchBox.IsValid() ? SearchBox->GetText().ToString() : FString();
	UpdateFilteredItems(CurrentQuery, bDropdownOpen);
}

void SMapItemFinderOverlay::RebuildResourceNodeFilterBar(const TArray<FMapFinderResourceNode>& Nodes)
{
	if (!ResourceNodeFilterBox.IsValid())
	{
		return;
	}

	ResourceNodeFilterBox->ClearChildren();

	TArray<TPair<FString, TSubclassOf<UFGItemDescriptor>>> ResourceTypes;
	TSet<TSubclassOf<UFGItemDescriptor>> Seen;

	for (const FMapFinderResourceNode& Node : Nodes)
	{
		if (!Node.ResourceClass || Seen.Contains(Node.ResourceClass))
		{
			continue;
		}
		Seen.Add(Node.ResourceClass);
		const FString Name = UFGItemDescriptor::GetItemName(Node.ResourceClass).ToString();
		if (!Name.IsEmpty())
		{
			ResourceTypes.Add(TPair<FString, TSubclassOf<UFGItemDescriptor>>(Name, Node.ResourceClass));
		}
	}

	ResourceTypes.Sort([](const TPair<FString, TSubclassOf<UFGItemDescriptor>>& A,
		const TPair<FString, TSubclassOf<UFGItemDescriptor>>& B)
	{
		return A.Key < B.Key;
	});

	auto AddFilterButton = [this](const FString& Label, TSubclassOf<UFGItemDescriptor> ResourceClass)
	{
		ResourceNodeFilterBox->AddSlot()
		[
			SNew(SButton)
			.ContentPadding(FMargin(6.0f, 2.0f))
			.ButtonColorAndOpacity(this, &SMapItemFinderOverlay::GetNodeFilterButtonColor, ResourceClass)
			.OnClicked(this, &SMapItemFinderOverlay::OnSelectNodeResourceFilter, ResourceClass)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
			]
		];
	};

	ResourceNodeFilterBox->AddSlot()
	[
		SNew(SButton)
		.ContentPadding(FMargin(6.0f, 2.0f))
		.ButtonColorAndOpacity(FSlateColor(FLinearColor(0.28f, 0.20f, 0.20f, 1.0f)))
		.OnClicked(this, &SMapItemFinderOverlay::OnClearNodeResourceFilterClicked)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("None")))
		]
	];

	AddFilterButton(TEXT("All"), nullptr);
	for (const TPair<FString, TSubclassOf<UFGItemDescriptor>>& Pair : ResourceTypes)
	{
		AddFilterButton(Pair.Key, Pair.Value);
	}
}

void SMapItemFinderOverlay::UpdateFilteredItems(const FString& Query, bool bOpenIfNonEmpty)
{
	FilteredItems.Empty();
	const FString Trimmed = Query.TrimStartAndEnd();

	FString LastCategory;
	bool bHasAnyItem = false;

	for (const TSharedPtr<FMapFinderItemEntry>& Entry : ProducedItemEntries)
	{
		const bool bMatches = Trimmed.IsEmpty()
			|| Entry->DisplayName.Contains(Trimmed, ESearchCase::IgnoreCase)
			|| Entry->CategoryName.Contains(Trimmed, ESearchCase::IgnoreCase);

		if (!bMatches)
		{
			continue;
		}

		if (Entry->CategoryName != LastCategory)
		{
			LastCategory = Entry->CategoryName;
			TSharedPtr<FMapFinderItemEntry> Header = MakeShared<FMapFinderItemEntry>();
			Header->bIsHeader = true;
			Header->DisplayName = LastCategory;
			Header->CategoryName = LastCategory;
			FilteredItems.Add(Header);
		}

		FilteredItems.Add(Entry);
		bHasAnyItem = true;
	}

	bDropdownOpen = bOpenIfNonEmpty && bHasAnyItem;
	if (ResultsListView.IsValid())
	{
		ResultsListView->RequestListRefresh();
	}
}

void SMapItemFinderOverlay::ToggleItem(const TSharedPtr<FMapFinderItemEntry>& Entry)
{
	if (!Entry.IsValid() || Entry->bIsHeader)
	{
		return;
	}

	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->ToggleItemSelection(WorldPtr.Get(), Entry->ItemClass);
	}
}

void SMapItemFinderOverlay::RefreshStatusText(bool bFromServer)
{
	if (!StatusText.IsValid())
	{
		return;
	}

	UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get();
	if (!Subsystem)
	{
		return;
	}

	const TSet<TSubclassOf<UFGItemDescriptor>>& Selected = Subsystem->GetSelectedItemClasses();
	const FString ModeSuffix = bFromServer ? TEXT("") : TEXT(" (local client only)");

	if (Selected.Num() == 0)
	{
		StatusText->SetText(FText::FromString(
			FString::Printf(TEXT("%d produced item(s) available%s"), ProducedItemEntries.Num(), *ModeSuffix)));
		return;
	}

	int32 ProdSites = 0;
	int32 ProdMachines = 0;
	int32 StorageSites = 0;
	int32 StoredItems = 0;

	const TMap<TSubclassOf<UFGItemDescriptor>, TArray<FMapFinderCluster>>& ResultsByItem = Subsystem->GetResultsByItem();
	for (const TSubclassOf<UFGItemDescriptor>& ItemClass : Selected)
	{
		if (const TArray<FMapFinderCluster>* Clusters = ResultsByItem.Find(ItemClass))
		{
			for (const FMapFinderCluster& Cluster : *Clusters)
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
		}
	}

	FString Prefix;
	if (Selected.Num() == 1)
	{
		const TSubclassOf<UFGItemDescriptor> OnlyItem = *Selected.CreateConstIterator();
		Prefix = OnlyItem ? UFGItemDescriptor::GetItemName(OnlyItem).ToString() : TEXT("Item");
	}
	else
	{
		Prefix = FString::Printf(TEXT("%d items"), Selected.Num());
	}

	StatusText->SetText(FText::FromString(FString::Printf(
		TEXT("%s: %d prod site(s) (%d machines), %d storage site(s) (%d items)%s"),
		*Prefix, ProdSites, ProdMachines, StorageSites, StoredItems, *ModeSuffix)));
}

void SMapItemFinderOverlay::HandleProducedItemsUpdated(
	const TArray<FMapFinderProducedItem>& ProducedItems,
	bool bFromServer)
{
	RebuildProducedItemEntries(ProducedItems);

	if (!SubsystemPtr.IsValid() || SubsystemPtr->GetSelectedItemClasses().Num() == 0)
	{
		RefreshStatusText(bFromServer);
	}
}

void SMapItemFinderOverlay::HandleResourceNodesUpdated(
	const TArray<FMapFinderResourceNode>& Nodes,
	bool bFromServer)
{
	RebuildResourceNodeFilterBar(Nodes);
}

void SMapItemFinderOverlay::HandleResultsUpdated(bool bFromServer)
{
	RefreshStatusText(bFromServer);
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
		if (StatusText.IsValid() && Subsystem->GetSelectedItemClasses().Num() == 0)
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

FReply SMapItemFinderOverlay::OnSelectNodeResourceFilter(TSubclassOf<UFGItemDescriptor> ResourceClass)
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->ToggleResourceNodeFilter(WorldPtr.Get(), ResourceClass);
	}
	return FReply::Handled();
}

FReply SMapItemFinderOverlay::OnClearNodeResourceFilterClicked()
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->ClearResourceNodeSelection(WorldPtr.Get());
	}
	return FReply::Handled();
}

void SMapItemFinderOverlay::OnSearchTextChanged(const FText& NewText)
{
	const FString Query = NewText.ToString();
	UpdateFilteredItems(Query, !Query.TrimStartAndEnd().IsEmpty());
}

void SMapItemFinderOverlay::OnSearchTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		for (const TSharedPtr<FMapFinderItemEntry>& Entry : FilteredItems)
		{
			if (Entry.IsValid() && !Entry->bIsHeader)
			{
				ToggleItem(Entry);
				break;
			}
		}
	}
}

TSharedRef<ITableRow> SMapItemFinderOverlay::OnGenerateItemRow(
	TSharedPtr<FMapFinderItemEntry> Entry,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	if (Entry.IsValid() && Entry->bIsHeader)
	{
		return SNew(STableRow<TSharedPtr<FMapFinderItemEntry>>, OwnerTable)
			.ShowSelection(false)
			.Padding(FMargin(4.0f, 6.0f, 4.0f, 2.0f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.95f))
				.Padding(FMargin(6.0f, 2.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Entry->DisplayName))
					.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.20f, 1.0f))
				]
			];
	}

	const TSubclassOf<UFGItemDescriptor> EntryItemClass = Entry.IsValid() ? Entry->ItemClass : nullptr;

	return SNew(STableRow<TSharedPtr<FMapFinderItemEntry>>, OwnerTable)
		.ShowSelection(false)
		.Padding(FMargin(8.0f, 3.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
			[
				SNew(SCheckBox)
				.IsChecked(this, &SMapItemFinderOverlay::IsItemChecked, EntryItemClass)
				.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnItemCheckboxChanged, EntryItemClass)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 8.0f, 0.0f))
			[
				SNew(SBox)
				.WidthOverride(20.0f)
				.HeightOverride(20.0f)
				[
					SNew(SImage)
					.Image(Entry.IsValid() ? &Entry->IconBrush : nullptr)
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Entry.IsValid() ? Entry->DisplayName : TEXT("")))
			]
		];
}

void SMapItemFinderOverlay::OnItemRowClicked(TSharedPtr<FMapFinderItemEntry> Entry)
{
	ToggleItem(Entry);
}

void SMapItemFinderOverlay::OnItemCheckboxChanged(
	ECheckBoxState NewState,
	TSubclassOf<UFGItemDescriptor> ItemClass)
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->ToggleItemSelection(WorldPtr.Get(), ItemClass);
	}
}

ECheckBoxState SMapItemFinderOverlay::IsItemChecked(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	return (SubsystemPtr.IsValid() && SubsystemPtr->IsItemSelected(ItemClass))
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
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
	RefreshStatusText(true);
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

void SMapItemFinderOverlay::OnResourceNodesCheckChanged(ECheckBoxState NewState)
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->SetShowResourceNodes(WorldPtr.Get(), NewState == ECheckBoxState::Checked);
	}
}

void SMapItemFinderOverlay::OnMercerSpheresCheckChanged(ECheckBoxState NewState)
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->SetShowMercerSpheres(WorldPtr.Get(), NewState == ECheckBoxState::Checked);
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

EVisibility SMapItemFinderOverlay::GetResourceNodeFilterVisibility() const
{
	return (SubsystemPtr.IsValid() && SubsystemPtr->IsFinderModeActive() && SubsystemPtr->GetShowResourceNodes())
		? EVisibility::Visible
		: EVisibility::Collapsed;
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

FSlateColor SMapItemFinderOverlay::GetNodeFilterButtonColor(TSubclassOf<UFGItemDescriptor> ResourceClass) const
{
	const bool bIsSelected = SubsystemPtr.IsValid()
		&& SubsystemPtr->IsResourceNodeFilterSelected(ResourceClass);
	return bIsSelected
		? FSlateColor(FLinearColor(0.25f, 0.70f, 0.95f, 1.0f))
		: FSlateColor(FLinearColor(0.18f, 0.21f, 0.26f, 1.0f));
}

#endif
