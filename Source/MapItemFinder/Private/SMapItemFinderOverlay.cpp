#if !UE_SERVER

#include "SMapItemFinderOverlay.h"

#include "FGRecipeManager.h"
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

	PopulateItems();

	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		ResultsDelegateHandle = Subsystem->OnResultsUpdated.AddSP(
			this, &SMapItemFinderOverlay::HandleResultsUpdated);
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
		.Padding(FMargin(0.0f, 18.0f, 0.0f, 0.0f))
		[
			SNew(SBox)
			.WidthOverride(420.0f)
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
							.HintText(FText::FromString(TEXT("Find item on map (produced or stored)...")))
							.OnTextChanged(this, &SMapItemFinderOverlay::OnSearchTextChanged)
							.OnTextCommitted(this, &SMapItemFinderOverlay::OnSearchTextCommitted)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(6.0f, 0.0f, 0.0f, 0.0f))
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
					.MaxHeight(220.0f)
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
	];

	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		if (Subsystem->GetActiveItemClass())
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
	}
}

void SMapItemFinderOverlay::PopulateItems()
{
	AllItems.Empty();

	UWorld* World = WorldPtr.Get();
	if (!World)
	{
		return;
	}

	AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(World);
	if (!RecipeManager)
	{
		return;
	}

	const TArray<TSubclassOf<UFGItemDescriptor>>& Descriptors =
		RecipeManager->GetAllAvailableItemDescriptors().Num() > 0
			? RecipeManager->GetAllAvailableItemDescriptors()
			: RecipeManager->GetAllItemDescriptors();

	TSet<TSubclassOf<UFGItemDescriptor>> Seen;
	for (const TSubclassOf<UFGItemDescriptor>& Descriptor : Descriptors)
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
		AllItems.Add(Entry);
	}

	AllItems.Sort([](const TSharedPtr<FMapFinderItemEntry>& A, const TSharedPtr<FMapFinderItemEntry>& B)
	{
		return A->DisplayName < B->DisplayName;
	});
}

void SMapItemFinderOverlay::UpdateFilteredItems(const FString& Query)
{
	FilteredItems.Empty();
	const FString Trimmed = Query.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		bDropdownOpen = false;
		if (ResultsListView.IsValid())
		{
			ResultsListView->RequestListRefresh();
		}
		return;
	}

	constexpr int32 MaxShown = 25;
	for (const TSharedPtr<FMapFinderItemEntry>& Entry : AllItems)
	{
		if (Entry->DisplayName.Contains(Trimmed, ESearchCase::IgnoreCase))
		{
			FilteredItems.Add(Entry);
			if (FilteredItems.Num() >= MaxShown)
			{
				break;
			}
		}
	}

	bDropdownOpen = FilteredItems.Num() > 0;
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

void SMapItemFinderOverlay::OnSearchTextChanged(const FText& NewText)
{
	UpdateFilteredItems(NewText.ToString());
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
	FilteredItems.Empty();
	if (SearchBox.IsValid())
	{
		SearchBox->SetText(FText::GetEmpty());
	}
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::GetEmpty());
	}
	if (UMapItemFinderSubsystem* Subsystem = SubsystemPtr.Get())
	{
		Subsystem->ClearPins(WorldPtr.Get());
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

EVisibility SMapItemFinderOverlay::GetDropdownVisibility() const
{
	return bDropdownOpen ? EVisibility::Visible : EVisibility::Collapsed;
}

#endif
