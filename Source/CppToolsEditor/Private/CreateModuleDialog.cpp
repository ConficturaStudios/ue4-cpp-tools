// Fill out your copyright notice in the Description page of Project Settings.


#include "CreateModuleDialog.h"

#define LOCTEXT_NAMESPACE "CppToolsModuleDialog"

//BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SCreateModuleDialog::Construct(const FArguments& InArgs) {
    ModuleName = InArgs._ModuleName;
    ModuleTarget = InArgs._ModuleTarget;
    ModuleType = InArgs._ModuleType;

    const float EditableTextHeight = 26.0f;

    // Create array of module type options
    AvailableModuleTypes.Reserve(EHostType::Max);
    for (int i = 0; i < EHostType::Type::Max; i++) {
        AvailableModuleTypes.Emplace(MakeShareable(new EHostType::Type));
        *AvailableModuleTypes[i] = static_cast<EHostType::Type>(i);
    }
    if (ModuleType == NULL) ModuleType = AvailableModuleTypes[0]; // Set default module type


    // Create array of module targets
    AvailableTargets.Add(MakeShareable(new FCreateModuleTarget));
    AvailableTargets[0]->Plugin = NULL;

    // Get all plugins
    for (const auto& Plugin : IPluginManager::Get().GetDiscoveredPlugins()) {
        // Only use plugins from this game project
        if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project) {
            // Add each plugin to list of available targets
            int32 i = AvailableTargets.Add(MakeShareable(new FCreateModuleTarget));
            AvailableTargets[i]->Plugin = Plugin;
        }
    }
    if (ModuleTarget == NULL) ModuleTarget = AvailableTargets[0]; // Set default module target

    //SNewClassDialog.cpp as reference

    OnCreateModule = InArgs._OnCreateModule;

    ChildSlot
    [
        SNew(SBorder)
        .Padding(18)
        .BorderImage(FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush"))
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            [
                SAssignNew(MainWizard, SWizard)
                .ShowPageList(false)

                .ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
                .CancelButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
                .FinishButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
                .ButtonTextStyle(FEditorStyle::Get(), "LargeText")
                .ForegroundColor(FEditorStyle::Get().GetSlateColor("WhiteBrush"))

                .CanFinish(this, &SCreateModuleDialog::CanFinish)
                .FinishButtonText(LOCTEXT("FinishButtonText_Module", "Create Module"))
                .FinishButtonToolTip(
                    LOCTEXT("FinishButtonToolTip_Module", "Creates the files to add your new module.")
                )
                .OnCanceled(this, &SCreateModuleDialog::CancelClicked)
                .OnFinished(this, &SCreateModuleDialog::FinishClicked)
                .InitialPageIndex(0)
                .PageFooter()
                [
					// Get IDE information
					SNew(SBorder)
					.Visibility( this, &SCreateModuleDialog::GetGlobalErrorLabelVisibility )
					.BorderImage( FEditorStyle::GetBrush("NewClassDialog.ErrorLabelBorder") )
					.Padding(FMargin(0, 5))
					.Content()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("MessageLog.Warning"))
						]

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text( this, &SCreateModuleDialog::GetGlobalErrorLabelText )
							.TextStyle( FEditorStyle::Get(), "NewClassDialog.ErrorLabelFont" )
						]

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.AutoWidth()
						.Padding(5.f, 0.f)
					]
                ]

                + SWizard::Page()
                [
                    SNew(SVerticalBox)

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0)
                    [
                        SNew(STextBlock)
                        .TextStyle(FEditorStyle::Get(), "NewClassDialog.PageTitle")
                        .Text(LOCTEXT("ModuleNameTitle", "Name Your New Module"))
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2, 0, 8)
                    [
                        SNew(SSeparator)
                    ]

                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
                    .Padding(0, 10)
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0, 0, 0, 5)
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("ModuleNameDescription", "Enter a name for your new module. Module names may only contain alphanumeric characters, and may not contain a space."))
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0, 0, 0, 2)
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("ModuleNameDetails", "When you click the \"Create\" button below, a new module will be created."))
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0, 5)
                        [
                            SNew(SBox).HeightOverride(20)
                            [
                                SNew(SBorder)
                                .Visibility(this, &SCreateModuleDialog::GetNameErrorLabelVisibility)
                                .BorderImage(FEditorStyle::GetBrush("NewClassDialog.ErrorLabelBorder"))
                                .Content()
                                [
                                    SNew(STextBlock)
                                    .Text(this, &SCreateModuleDialog::GetNameErrorLabelText)
                                    .TextStyle(FEditorStyle::Get(), "NewClassDialog.ErrorLabelFont")
                                ]
                            ]
                        ]

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SBorder)
                            .BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop"))
                            .BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
                            .Padding(FMargin(6.0f, 4.0f, 7.0f, 4.0f))
                            [
                                SNew(SVerticalBox)

                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0)
                                [
                                    SNew(SGridPanel)
                                    .FillColumn(1, 1.0f)

                                    + SGridPanel::Slot(0, 0)
                                    .VAlign(VAlign_Center)
                                    .Padding(0, 0, 12, 0)
                                    [
                                        SNew(STextBlock)
                                        .TextStyle(FEditorStyle::Get(), "NewClassDialog.SelectedParentClassLabel")
                                        .Text(LOCTEXT("NameLabel", "Name"))
                                    ]

                                    + SGridPanel::Slot(1, 0)
                                    .Padding(0.0f, 3.0f)
                                    .VAlign(VAlign_Center)
                                    [
                                        SNew(SBox)
                                        .HeightOverride(EditableTextHeight)
                                        [
                                            SNew(SHorizontalBox)

                                            // Name Textbox
                                            + SHorizontalBox::Slot()
                                            .FillWidth(1.0f)
                                            [
                                                SAssignNew(ModuleNameEditBox, SEditableTextBox)
                                                .Text(this, &SCreateModuleDialog::OnGetModuleNameText)
                                                .OnTextChanged(this, &SCreateModuleDialog::OnModuleNameTextChanged)
                                                .OnTextCommitted(this, &SCreateModuleDialog::OnModuleNameTextCommitted)
                                            ]

                                            // Module Target Combobox
                                            + SHorizontalBox::Slot()
                                            .AutoWidth()
                                            .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                                            [
                                                SAssignNew(ModuleTargetsCombo, SComboBox<TSharedPtr<FCreateModuleTarget>>)
                                                .ToolTipText(LOCTEXT("ModuleTargetComboToolTip", "Choose the target location for your new module"))
                                                .OptionsSource(&AvailableTargets)
                                                .InitiallySelectedItem(ModuleTarget)
                                                .OnSelectionChanged(this, &SCreateModuleDialog::OnModuleTargetChanged)
                                                .OnGenerateWidget(this, &SCreateModuleDialog::MakeWidgetForModuleTargetCombo)
                                                [
                                                    SNew(STextBlock)
                                                    .Text(this, &SCreateModuleDialog::OnGetModuleTargetComboText)
                                                ]
                                            ]

                                            // Module Type Combobox
                                            + SHorizontalBox::Slot()
                                            .AutoWidth()
                                            .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                                            [
                                                SAssignNew(ModuleTypesCombo, SComboBox<TSharedPtr<EHostType::Type>>)
                                                .ToolTipText(LOCTEXT("ModuleComboToolTip", "Choose the type for your new module"))
                                                .OptionsSource(&AvailableModuleTypes)
                                                .InitiallySelectedItem(ModuleType)
                                                .OnSelectionChanged(this, &SCreateModuleDialog::OnModuleTypeChanged)
                                                .OnGenerateWidget(this, &SCreateModuleDialog::MakeWidgetForModuleTypeCombo)
                                                [
                                                    SNew(STextBlock)
                                                    .Text(this, &SCreateModuleDialog::OnGetModuleTypeComboText)
                                                ]
                                            ]

                                        ]
                                    ]

                                    + SGridPanel::Slot(0, 1)
                                    .VAlign(VAlign_Center)
                                    .Padding(0, 0, 12, 0)
                                    [
                                        SNew(STextBlock)
                                        .TextStyle(FEditorStyle::Get(), "NewClassDialog.SelectedParentClassLabel")
                                        .Text(LOCTEXT("PathLabel", "Path"))
                                    ]

                                    + SGridPanel::Slot(1, 1)
                                    .Padding(0.0f, 3.0f)
                                    .VAlign(VAlign_Center)
                                    [
                                        SNew(SBox)
                                        .VAlign(VAlign_Center)
                                        .HeightOverride(EditableTextHeight)
                                        [
                                            SNew(STextBlock)
                                            .Text(this, &SCreateModuleDialog::OnGetModulePathText)
                                        ]
                                    ]
                                ]
                            ]
                        ]

                        +SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f)
                        [
                            SNew(SBorder)
                            .Padding(FMargin(0.0f, 3.0f, 0.0f, 0.0f))
                            .BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryBottom"))
                            .BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
                        ]
                    ]
                ]
            ]
        ]
    ];
    UpdateInputValidity();
}

void SCreateModuleDialog::CancelClicked() {
    CloseContainingWindow();
}

bool SCreateModuleDialog::CanFinish() const {
    return bLastInputValidityCheckSuccessful;
}

void SCreateModuleDialog::FinishClicked() {

    //GameProjectUtils.cpp L3383
    //GameProjectUtils::GenerateGameModuleBuildFile(NewBuildFilename, ModuleName, PublicDependencies, PrivateDependencies, OutFailReason)
    //GameProjectUtils.cpp L3437
    //GameProjectUtils::GenerateEditorModuleBuildFile(NewBuildFilename, ModuleName, PublicDependencies, PrivateDependencies, OutFailReason)

    TArray<FString> CreatedFiles;
    FText OutFailReason;

    GameProjectUtils::EAddCodeToProjectResult AddModuleResult = CppToolsUtil::GenerateModule(GetModulePath(), ModuleTarget->Plugin, ModuleName, *ModuleType, false, CreatedFiles, OutFailReason);
    if (AddModuleResult == GameProjectUtils::EAddCodeToProjectResult::Succeeded) {

        OnCreateModule.ExecuteIfBound(ModuleName, *ModuleTarget, *ModuleType);

        // Reload current project to take into account any new state
        IProjectManager::Get().LoadProjectFile(FPaths::GetProjectFilePath());

        FNotificationInfo Notification(FText::Format(LOCTEXT("AddedModuleSuccessNotification", "Added new module {0}"), FText::FromString(ModuleName)));
        FSlateNotificationManager::Get().AddNotification(Notification);
        //UE_LOG(LogClass, Log, TEXT("Successfully generated module '%s'"), **ModuleName);
    }
    else if (AddModuleResult == GameProjectUtils::EAddCodeToProjectResult::FailedToHotReload) {
        OnCreateModule.ExecuteIfBound(ModuleName, *ModuleTarget, *ModuleType);

        // Failed to compile new code
        const FText Message = FText::Format(
            LOCTEXT("AddCodeFailed_HotReloadFailed", "Successfully added module '{0}', however you must recompile the '{1}' module before it will appear in the Content Browser. {2}\n\nWould you like to open the Output Log to see more details?")
            , FText::FromString(ModuleName), FText::FromString(ModuleName), OutFailReason);
        if (FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes)
        {
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
            FGlobalTabmanager::Get()->TryInvokeTab(FName("OutputLog"));
#elif ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25
            FGlobalTabmanager::Get()->InvokeTab(FName("OutputLog"));
#endif
        }

        // We did manage to add the code itself, so we can close the dialog.
        CloseContainingWindow();
    }
    else {
        // Failed to add code
        const FText Message = FText::Format(LOCTEXT("AddModuleFailed_AddModuleFailed", "Failed to add module '{0}'. {1}"), FText::FromString(ModuleName), OutFailReason);
        FMessageDialog::Open(EAppMsgType::Ok, Message);
        //UE_LOG(LogClass, Log, TEXT("Failed to generate module '%s'"), **ModuleName);
    }

    //UE_LOG(LogClass, Log, TEXT("Successfully generated module '%s'"), **ModuleName);

    CloseContainingWindow();
}

EVisibility SCreateModuleDialog::GetGlobalErrorLabelVisibility() const {
    return GetGlobalErrorLabelText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SCreateModuleDialog::GetGlobalErrorLabelText() const {
    if (!FSourceCodeNavigation::IsCompilerAvailable())
    {
        return FText::Format(LOCTEXT("NoCompilerFound", "No compiler was found. In order to use C++ code, you must first install {0}."), FSourceCodeNavigation::GetSuggestedSourceCodeIDE());
    }

    return FText::GetEmpty();
}

EVisibility SCreateModuleDialog::GetNameErrorLabelVisibility() const {
    return GetNameErrorLabelText().IsEmpty() ? EVisibility::Hidden : EVisibility::Visible;
}

FText SCreateModuleDialog::GetNameErrorLabelText() const
{
    if (!bLastInputValidityCheckSuccessful)
    {
        return LastInputValidityErrorText;
    }

    return FText::GetEmpty();
}



FText SCreateModuleDialog::OnGetModuleNameText() const {
    return FText::FromString(ModuleName);
}

void SCreateModuleDialog::OnModuleNameTextChanged(const FText& NewText) {
    ModuleName = NewText.ToString();
    UpdateInputValidity();
}

void SCreateModuleDialog::OnModuleNameTextCommitted(const FText& NewText, ETextCommit::Type CommitType) {
    if (CommitType == ETextCommit::OnEnter)
    {
        if (CanFinish())
        {
            FinishClicked();
        }
    }
}



FText SCreateModuleDialog::OnGetModuleTargetComboText() const {
    return FText::FromString(ModuleTarget->Plugin == NULL ? FString(FApp::GetProjectName()) + " (Game)" : ModuleTarget->Plugin->GetDescriptor().FriendlyName + " (Plugin)");
}

void SCreateModuleDialog::OnModuleTargetChanged(TSharedPtr<FCreateModuleTarget> Value, ESelectInfo::Type SelectInfo) {
    ModuleTarget = Value;
}

TSharedRef<SWidget> SCreateModuleDialog::MakeWidgetForModuleTargetCombo(TSharedPtr<FCreateModuleTarget> Value) {
    return SNew(STextBlock)
        .Text(FText::FromString(Value->Plugin == NULL ? FString(FApp::GetProjectName()) + "(Game)" : Value->Plugin->GetDescriptor().FriendlyName + " (Plugin)"));
}



FText SCreateModuleDialog::OnGetModuleTypeComboText() const {
    return FText::FromString(EHostType::ToString(*ModuleType));
}

void SCreateModuleDialog::OnModuleTypeChanged(TSharedPtr<EHostType::Type> Value, ESelectInfo::Type SelectInfo) {
    ModuleType = Value;
}

TSharedRef<SWidget> SCreateModuleDialog::MakeWidgetForModuleTypeCombo(TSharedPtr<EHostType::Type> Value) {
    return SNew(STextBlock)
        .Text(FText::FromString(EHostType::ToString(*Value)));
}



FText SCreateModuleDialog::OnGetModulePathText() const {
    return FText::FromString(GetModulePath());
}

FString SCreateModuleDialog::GetModulePath() const {
    FString Path = (ModuleTarget->Plugin == NULL) ? FPaths::GameSourceDir() : ModuleTarget->Plugin->GetBaseDir() / "Source";
    return FPaths::ConvertRelativePathToFull(Path / ModuleName / "");
}


void SCreateModuleDialog::UpdateInputValidity() {
    bLastInputValidityCheckSuccessful = true;
    FString IllegalNameCharacters;

    if (ModuleName.IsEmpty()) {
        bLastInputValidityCheckSuccessful = false;
        LastInputValidityErrorText = FText::FromString(TEXT("You must enter a name to create a new module"));
        return;
    }
    else if (!GameProjectUtils::NameContainsOnlyLegalCharacters(ModuleName, IllegalNameCharacters)) {
        bLastInputValidityCheckSuccessful = false;
        FFormatNamedArguments Args;
        Args.Add(TEXT("IllegalNameCharacters"), FText::FromString(IllegalNameCharacters));
        LastInputValidityErrorText = FText::Format(LOCTEXT("ModuleNameContainsIllegalCharacters", "The module name may not contain the following characters: '{IllegalNameCharacters}'"), Args);
        return;
    }
    
    if (!GameProjectUtils::ProjectHasCodeFiles()) {
        bLastInputValidityCheckSuccessful = false;
        LastInputValidityErrorText = FText::FromString(TEXT("A new module may not be added to a non-C++ project. Convert the project from Blueprint to C++ by adding a C++ class in the editor."));
        return;
    }

    if (FModuleManager::Get().ModuleExists(*ModuleName)) {
        bLastInputValidityCheckSuccessful = false;
        FFormatNamedArguments Args;
        Args.Add(TEXT("ModuleName"), FText::FromString(ModuleName));
        LastInputValidityErrorText = FText::Format(LOCTEXT("ModuleNameWarning", "Module '{ModuleName}' already exists. If this module was manually deleted, clean and rebuild the project."), Args);
        return;
    }

}


void SCreateModuleDialog::CloseContainingWindow()
{
    TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());

    if (ContainingWindow.IsValid())
    {
        ContainingWindow->RequestDestroyWindow();
    }
}

#undef LOCTEXT_NAMESPACE