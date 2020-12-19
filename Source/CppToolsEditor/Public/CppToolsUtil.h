// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Interfaces/IPluginManager.h"

#include "DesktopPlatformModule.h"

#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"

#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/HotReloadInterface.h"
#include "Misc/FeedbackContext.h"

#include "Framework/Notifications/NotificationManager.h"

#include "GeneralProjectSettings.h"
#include "SourceCodeNavigation.h"
#include "ProjectDescriptor.h"

#include "GameProjectUtils.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"

#include "Widgets/Notifications/SNotificationList.h"

DECLARE_DELEGATE_RetVal_OneParam(bool, FPluginDescriptorModifier, FPluginDescriptor&);

/**
 * 
 */
class CPPTOOLSEDITOR_API CppToolsUtil
{
public:

    /** Gets the Content directory of the CppTools Plugin. */
    static FString CppToolsContentDir();

    static bool CheckoutFile(const FString& Filename, FText& OutFailReason);

    static void TryMakePluginFileWriteable(const FString& PluginFile);

    static void PushNotification(const FText& Text, const SNotificationItem::ECompletionState& CompletionState);

    /** Reads in a custom template file from the CppTools Content folder. */
    static bool ReadCustomTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason);

    static FString GetCopyrightLine();
    static FString GetModuleAPIMacro(const FString& ModuleName, bool bIsPrivate);

    static bool GenerateModuleBuildFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& PublicDependencyModuleNames,
        const TArray<FString>& PrivateDependencyModuleNames, FText& OutFailReason, bool bUseExplicitOrSharedPCHs);
    static bool GenerateModuleHeaderFile(const FString& NewHeaderFileName, const FString& ModuleName, const TArray<FString>& PublicHeaderIncludes,
        FText& OutFailReason);
    static bool GenerateModuleCPPFile(const FString& NewCPPFileName, const FString& ModuleName, const FString& StartupSourceCode,
        const FString& ShutdownSourceCode, FText& OutFailReason);

    static bool InsertDependencyIntoPrimaryBuild(const FString& ModuleName, TSharedPtr<IPlugin> Target, const bool& bIsEditor, FText& OutFailReason);
    static bool InsertDependencyIntoTarget(const FString& ModuleName, const bool& bIsEditor, FText& OutFailReason);

    static GameProjectUtils::EAddCodeToProjectResult GenerateModule(const FString& ModulePath, TSharedPtr<IPlugin> Target,
        const FString& ModuleName, const EHostType::Type& Type, bool bUsePCH, TArray<FString>& CreatedFiles, FText& OutFailReason);
    
private:

    static bool UpdateGameProjectFile(const FString& ProjectFile, const FString& EngineIdentifier, const FProjectDescriptorModifier* Modifier, FText& OutFailReason);
    static bool UpdateGameProject(const FProjectDescriptorModifier* Modifier);

    static bool AppendProjectModules(FProjectDescriptor& Descriptor, const TArray<FModuleDescriptor>* Modules);

    static bool UpdatePluginFile(const FString& PluginFile, const FPluginDescriptorModifier* Modifier, FText& OutFailReason);
    static bool UpdatePlugin(const TSharedPtr<IPlugin>&, const FPluginDescriptorModifier* Modifier);

    static bool AppendPluginModules(FPluginDescriptor& Descriptor, const TArray<FModuleDescriptor>* Modules);

};
