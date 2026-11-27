#if !UE_SERVER

#include "SMapItemFinderOverlay.h"

#include "FGCategory.h"
#include "MapItemFinderSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
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

namespace
{
	// Native Satisfactory Map UI palette
	const FLinearColor ColorPanelBorder(0.24f, 0.24f, 0.24f, 0.96f);
	const FLinearColor ColorPanelBg(0.11f, 0.11f, 0.11f, 0.95f);
	const FLinearColor ColorHeaderBar(0.32f, 0.32f, 0.32f, 1.0f);
	const FLinearColor ColorSearchBar(0.34f, 0.34f, 0.34f, 1.0f);
	const FLinearColor ColorFooterBg(0.14f, 0.14f, 0.14f, 0.98f);
	const FLinearColor ColorButtonNeutral(0.45f, 0.45f, 0.45f, 1.0f);
	const FLinearColor ColorChipUnselected(0.20f, 0.20f, 0.20f, 1.0f);
	const FLinearColor ColorAccentOrange(0.89f, 0.48f, 0.12f, 1.0f);
	const FLinearColor ColorTextPrimary(0.92f, 0.92f, 0.92f, 1.0f);
	const FLinearColor ColorTextMuted(0.75f, 0.75f, 0.75f, 1.0f);

	const FButtonStyle& GetFlatButtonStyle()
	{
		static FButtonStyle Style = []()
		{
			FButtonStyle S = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
			S.Normal = *White;
			S.Normal.TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
			S.Hovered = *White;
			S.Hovered.TintColor = FLinearColor(1.15f, 1.15f, 1.15f, 1.0f);
			S.Pressed = *White;
			S.Pressed.TintColor = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);
			S.NormalPadding = FMargin(0.0f);
			S.PressedPadding = FMargin(0.0f);
			return S;
		}();
		return Style;
	}

	const FEditableTextBoxStyle& GetFlatSearchBoxStyle()
	{
		static FEditableTextBoxStyle Style = []()
		{
			FEditableTextBoxStyle S = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
			S.BackgroundImageNormal = *White;
			S.BackgroundImageNormal.TintColor = ColorSearchBar;
			S.BackgroundImageHovered = *White;
			S.BackgroundImageHovered.TintColor = FLinearColor(0.38f, 0.38f, 0.38f, 1.0f);
			S.BackgroundImageFocused = *White;
			S.BackgroundImageFocused.TintColor = FLinearColor(0.28f, 0.28f, 0.28f, 1.0f);
			S.BackgroundImageReadOnly = *White;
			S.BackgroundImageReadOnly.TintColor = ColorSearchBar;
			S.ForegroundColor = ColorTextPrimary;
			S.FocusedForegroundColor = FLinearColor::White;
			S.Padding = FMargin(8.0f, 6.0f);
			return S;
		}();
		return Style;
	}

	TSharedRef<SWidget> MakeSectionHeader(const FString& Title)
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(ColorHeaderBar)
			.Padding(FMargin(8.0f, 4.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Title))
				.ColorAndOpacity(ColorTextPrimary)
			];
	}
}

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
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	ChildSlot
	[
		SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(0.0f, 148.0f, 22.0f, 160.0f))
		[
			SNew(SVerticalBox)

			// Right-aligned toggle button mirroring native "Hide Menu" on the left
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
			[
				SNew(SBox)
				.WidthOverride(150.0f)
				.HeightOverride(42.0f)
				[
					SNew(SButton)
					.ButtonStyle(&GetFlatButtonStyle())
					.ButtonColorAndOpacity(this, &SMapItemFinderOverlay::GetToggleFinderButtonColor)
					.OnClicked(this, &SMapItemFinderOverlay::OnToggleFinderModeClicked)
					.ContentPadding(FMargin(0.0f))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SMapItemFinderOverlay::GetToggleFinderButtonText)
						.ColorAndOpacity(ColorTextPrimary)
					]
				]
			]

			// Right-side Finder panel styled like the native Map left menu
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.HAlign(HAlign_Right)
			[
				SNew(SBox)
				.WidthOverride(320.0f)
				.Visibility(this, &SMapItemFinderOverlay::GetFinderPanelVisibility)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(ColorPanelBorder)
					.Padding(FMargin(1.0f))
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(ColorPanelBg)
						.Padding(FMargin(0.0f))
						[
							SNew(SVerticalBox)

							// Search bar (matching native "Search...")
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
							[
								SAssignNew(SearchBox, SEditableTextBox)
								.Style(&GetFlatSearchBoxStyle())
								.HintText(FText::FromString(TEXT("Search...")))
								.OnTextChanged(this, &SMapItemFinderOverlay::OnSearchTextChanged)
								.OnTextCommitted(this, &SMapItemFinderOverlay::OnSearchTextCommitted)
							]

							// Display Filters section
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								MakeSectionHeader(TEXT("Map Item Finder"))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(8.0f, 6.0f))
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
								[
									SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SCheckBox)
										.IsChecked(bInitialProduced ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
										.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnProducedCheckChanged)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Produced")))
											.ColorAndOpacity(ColorTextPrimary)
										]
									]

									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SCheckBox)
										.IsChecked(bInitialStored ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
										.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnStoredCheckChanged)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Stored / Depot")))
											.ColorAndOpacity(ColorTextPrimary)
										]
									]
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SCheckBox)
										.IsChecked(bInitialNodes ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
										.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnResourceNodesCheckChanged)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Resource Nodes")))
											.ColorAndOpacity(ColorTextPrimary)
										]
									]

									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SCheckBox)
										.IsChecked(bInitialMercer ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
										.OnCheckStateChanged(this, &SMapItemFinderOverlay::OnMercerSpheresCheckChanged)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Mercer Spheres")))
											.ColorAndOpacity(ColorTextPrimary)
										]
									]
								]
							]

							// Resource Nodes filter chips (shown when Resource Nodes is checked)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)
								.Visibility(this, &SMapItemFinderOverlay::GetResourceNodeFilterVisibility)

								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									MakeSectionHeader(TEXT("Resource Nodes"))
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(8.0f, 6.0f))
								[
									SAssignNew(ResourceNodeFilterBox, SWrapBox)
									.UseAllottedSize(true)
									.InnerSlotPadding(FVector2D(4.0f, 4.0f))
								]
							]

							// Scrollable Produced Items list (always visible inside sidebar, like native menu)
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SAssignNew(ResultsListView, SListView<TSharedPtr<FMapFinderItemEntry>>)
								.ListItemsSource(&FilteredItems)
								.OnGenerateRow(this, &SMapItemFinderOverlay::OnGenerateItemRow)
								.OnMouseButtonClick(this, &SMapItemFinderOverlay::OnItemRowClicked)
								.SelectionMode(ESelectionMode::None)
							]

							// Bottom footer bar (matching native "Show Stamps on Map" footer)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(ColorPanelBorder)
								.Padding(FMargin(0.0f, 1.0f, 0.0f, 0.0f))
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(ColorFooterBg)
									.Padding(FMargin(8.0f, 6.0f))
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
										.FillWidth(1.0f)
										.VAlign(VAlign_Center)
										.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
										[
											SAssignNew(StatusText, STextBlock)
											.ColorAndOpacity(ColorTextMuted)
											.AutoWrapText(true)
											.Text(FText::GetEmpty())
										]

										+ SHorizontalBox::Slot()
										.AutoWidth()
										.VAlign(VAlign_Center)
										[
											SNew(SButton)
											.ButtonStyle(&GetFlatButtonStyle())
											.ButtonColorAndOpacity(ColorButtonNeutral)
											.ContentPadding(FMargin(10.0f, 4.0f))
											.OnClicked(this, &SMapItemFinderOverlay::OnClearClicked)
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("Clear")))
												.ColorAndOpacity(ColorTextPrimary)
											]
										]
									]
								]
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
		if (Subsystem->IsFinderModeActive())
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
			Entry->IconBrush.SetImageSize(FVector2D(18.0, 18.0));
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
	UpdateFilteredItems(CurrentQuery);
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
			.ButtonStyle(&GetFlatButtonStyle())
			.ContentPadding(FMargin(6.0f, 3.0f))
			.ButtonColorAndOpacity(this, &SMapItemFinderOverlay::GetNodeFilterButtonColor, ResourceClass)
			.OnClicked(this, &SMapItemFinderOverlay::OnSelectNodeResourceFilter, ResourceClass)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.ColorAndOpacity(ColorTextPrimary)
			]
		];
	};

	ResourceNodeFilterBox->AddSlot()
	[
		SNew(SButton)
		.ButtonStyle(&GetFlatButtonStyle())
		.ContentPadding(FMargin(6.0f, 3.0f))
		.ButtonColorAndOpacity(FSlateColor(FLinearColor(0.28f, 0.22f, 0.22f, 1.0f)))
		.OnClicked(this, &SMapItemFinderOverlay::OnClearNodeResourceFilterClicked)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("None")))
			.ColorAndOpacity(ColorTextPrimary)
		]
	];

	AddFilterButton(TEXT("All"), nullptr);
	for (const TPair<FString, TSubclassOf<UFGItemDescriptor>>& Pair : ResourceTypes)
	{
		AddFilterButton(Pair.Key, Pair.Value);
	}
}

void SMapItemFinderOverlay::UpdateFilteredItems(const FString& Query)
{
	FilteredItems.Empty();
	const FString Trimmed = Query.TrimStartAndEnd();

	FString LastCategory;

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
	}

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
	const FString ModeSuffix = bFromServer ? TEXT("") : TEXT(" (local)");

	if (Selected.Num() == 0)
	{
		StatusText->SetText(FText::FromString(
			FString::Printf(TEXT("%d produced item(s)%s"), ProducedItemEntries.Num(), *ModeSuffix)));
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
		TEXT("%s: %d prod (%d), %d storage (%d)%s"),
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

FReply SMapItemFinderOverlay::OnToggleFinderModeClicked()
{
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		const bool bEnable = !Subsystem->IsFinderModeActive();
		if (bEnable && StatusText.IsValid() && Subsystem->GetSelectedItemClasses().Num() == 0)
		{
			StatusText->SetText(FText::FromString(TEXT("Loading produced items...")));
		}
		Subsystem->SetFinderMode(WorldPtr.Get(), bEnable);
	}
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
	UpdateFilteredItems(NewText.ToString());
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
			.Padding(FMargin(0.0f, 4.0f, 0.0f, 2.0f))
			[
				MakeSectionHeader(Entry->DisplayName)
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
				.WidthOverride(18.0f)
				.HeightOverride(18.0f)
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
				.ColorAndOpacity(ColorTextPrimary)
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

EVisibility SMapItemFinderOverlay::GetResourceNodeFilterVisibility() const
{
	return (SubsystemPtr.IsValid() && SubsystemPtr->IsFinderModeActive() && SubsystemPtr->GetShowResourceNodes())
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

FText SMapItemFinderOverlay::GetToggleFinderButtonText() const
{
	const bool bIsFinder = SubsystemPtr.IsValid() && SubsystemPtr->IsFinderModeActive();
	return FText::FromString(bIsFinder ? TEXT("Hide Finder") : TEXT("Show Finder"));
}

FSlateColor SMapItemFinderOverlay::GetToggleFinderButtonColor() const
{
	return FSlateColor(ColorButtonNeutral);
}

FSlateColor SMapItemFinderOverlay::GetNodeFilterButtonColor(TSubclassOf<UFGItemDescriptor> ResourceClass) const
{
	const bool bIsSelected = SubsystemPtr.IsValid()
		&& SubsystemPtr->IsResourceNodeFilterSelected(ResourceClass);
	return bIsSelected
		? FSlateColor(ColorAccentOrange)
		: FSlateColor(ColorChipUnselected);
}

#endif
