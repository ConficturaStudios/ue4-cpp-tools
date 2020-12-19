// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Widgets/Workflow/SWizard.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SGridPanel.h"
#include "SourceCodeNavigation.h"
#include "EditorStyleSet.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "ModuleDescriptor.h"
#include "Interfaces/IPluginManager.h"
#include "Interfaces/IProjectManager.h"

#include "GameProjectUtils.h"

#include "CppToolsUtil.h"

struct FCreateModuleTarget {
    TSharedPtr<IPlugin> Plugin;
};

DECLARE_DELEGATE_ThreeParams(FOnCreateModule, FString, FCreateModuleTarget, EHostType::Type);

#define S_DECLARE_CHECKBOX(name) \
bool name; \
ECheckboxState Is##name##Checked() const { return name ? ECheckboxState::Checked : ECheckboxState::Unchecked; } \
void On##name##Changed(ECheckboxState InCheckedState) { name = (InCheckedState == ECheckBoxState::Checked); }

/**
 *
 */
class SCreateModuleDialog : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SCreateModuleDialog)
        : _ModuleName(TEXT("NewModule"))
        , _ModuleTarget(NULL)
        , _ModuleType(NULL)
        , _ModuleLoadingPhase(NULL)
    {}

    /** The name of the new module to create. */
    SLATE_ARGUMENT(FString, ModuleName)
    /** The plugin to add the module to, or NULL if added to the game. */
    SLATE_ARGUMENT(TSharedPtr<FCreateModuleTarget>, ModuleTarget)
    /** The type of module to create. */
    SLATE_ARGUMENT(TSharedPtr<EHostType::Type>, ModuleType)
    /** The loading phase of the module to create. */
    SLATE_ARGUMENT(TSharedPtr<ELoadingPhase::Type>, ModuleLoadingPhase)

    /** Event called when module is added. */
    SLATE_EVENT(FOnCreateModule, OnCreateModule)

    SLATE_END_ARGS()

    /** Constructs this widget with InArgs */
    void Construct(const FArguments& InArgs);

private:

    /** Handler for when cancel is clicked */
    void CancelClicked();

    /** Returns true if Finish is allowed */
    bool CanFinish() const;

    /** Handler for when finish is clicked */
    void FinishClicked();


    /** Gets the visibility of the global error label */
    EVisibility GetGlobalErrorLabelVisibility() const;
    /** Gets the text to display in the global error label */
    FText GetGlobalErrorLabelText() const;


    /** Gets the visibility of the name error label */
    EVisibility GetNameErrorLabelVisibility() const;
    /** Gets the text to display in the name error label */
    FText GetNameErrorLabelText() const;


    /** Returns the text in the module name edit box */
    FText OnGetModuleNameText() const;
    /** Handler for when the text in the module name edit box has changed */
    void OnModuleNameTextChanged(const FText& NewText);
    /** Handler for when the text in the module name edit box is committed */
    void OnModuleNameTextCommitted(const FText& NewText, ETextCommit::Type CommitType);


    /** Get the combo box text for the currently selected module target */
    FText OnGetModuleTargetComboText() const;
    /** Called when the currently selected module target is changed */
    void OnModuleTargetChanged(TSharedPtr<FCreateModuleTarget> Value, ESelectInfo::Type SelectInfo);
    /** Create the widget to use as the combo box entry for the given module target */
    TSharedRef<SWidget> MakeWidgetForModuleTargetCombo(TSharedPtr<FCreateModuleTarget> Value);


    /** Get the combo box text for the currently selected module type */
    FText OnGetModuleTypeComboText() const;
    /** Called when the currently selected module type is changed */
    void OnModuleTypeChanged(TSharedPtr<EHostType::Type> Value, ESelectInfo::Type SelectInfo);
    /** Create the widget to use as the combo box entry for the given module type */
    TSharedRef<SWidget> MakeWidgetForModuleTypeCombo(TSharedPtr<EHostType::Type> Value);


    /** Get the combo box text for the currently selected module loading phase */
    FText OnGetModuleLoadingPhaseComboText() const;
    /** Called when the currently selected module loading phase is changed */
    void OnModuleLoadingPhaseChanged(TSharedPtr<ELoadingPhase::Type> Value, ESelectInfo::Type SelectInfo);
    /** Create the widget to use as the combo box entry for the given module loading phase */
    TSharedRef<SWidget> MakeWidgetForModuleLoadingPhaseCombo(TSharedPtr<ELoadingPhase::Type> Value);

    
    /** Returns the text in the module path box */
    FText OnGetModulePathText() const;

    FString GetModulePath() const;


    /** Checks the current module name for validity and updates cached values accordingly */
    void UpdateInputValidity();

    /** Closes the window that contains this widget */
    void CloseContainingWindow();



    TSharedPtr<SWizard> MainWizard;
    FOnCreateModule OnCreateModule;

    /** The editable text box to enter the current name */
    TSharedPtr<SEditableTextBox> ModuleNameEditBox;
    /** The combobox for selecting the module type. */
    TSharedPtr<SComboBox<TSharedPtr<EHostType::Type>>> ModuleTypesCombo;
    /** The combobox for selecting the module loading phase. */
    TSharedPtr<SComboBox<TSharedPtr<ELoadingPhase::Type>>> ModuleLoadingPhasesCombo;
    /** The combobox for selecting the module target. */
    TSharedPtr<SComboBox<TSharedPtr<FCreateModuleTarget>>> ModuleTargetsCombo;


    TArray<TSharedPtr<EHostType::Type>> AvailableModuleTypes;
    TArray<TSharedPtr<ELoadingPhase::Type>> AvailableModuleLoadingPhases;
    TArray<TSharedPtr<FCreateModuleTarget>> AvailableTargets;


    FString ModuleName;
    TSharedPtr<FCreateModuleTarget> ModuleTarget;
    TSharedPtr<EHostType::Type> ModuleType;
    TSharedPtr<ELoadingPhase::Type> ModuleLoadingPhase;

    /** Was the last input validity check successful? */
    bool bLastInputValidityCheckSuccessful;
    /** The error text from the last validity check */
    FText LastInputValidityErrorText;

};
