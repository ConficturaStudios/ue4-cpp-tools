// Fill out your copyright notice in the Description page of Project Settings.


#include "CppToolsUtil.h"

#define LOCTEXT_NAMESPACE "CppToolsUtil"


FString CppToolsUtil::CppToolsContentDir() {
    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("CppTools"))->GetContentDir();
    return ContentDir;
}

bool CppToolsUtil::ReadCustomTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason) {
    const FString FullFileName = CppToolsContentDir() / TEXT("Editor") / TEXT("Templates") / TemplateFileName;
    if (FFileHelper::LoadFileToString(OutFileContents, *FullFileName))
    {
        return true;
    }

    FFormatNamedArguments Args;
    Args.Add(TEXT("FullFileName"), FText::FromString(FullFileName));
    OutFailReason = FText::Format(LOCTEXT("FailedToReadTemplateFile", "Failed to read template file \"{FullFileName}\""), Args);
    return false;
}

FString CppToolsUtil::GetCopyrightLine() {
    const FString CopyrightNotice = GetDefault<UGeneralProjectSettings>()->CopyrightNotice;
    if (!CopyrightNotice.IsEmpty())
    {
        return FString(TEXT("// ")) + CopyrightNotice;
    }
    else
    {
        return FString();
    }
}

FString CppToolsUtil::GetModuleAPIMacro(const FString& ModuleName, bool bIsPrivate) {
    return bIsPrivate ? FString() : ModuleName.ToUpper() + "_API ";
}

bool CppToolsUtil::GenerateModuleBuildFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& PublicDependencyModuleNames, const TArray<FString>& PrivateDependencyModuleNames, FText& OutFailReason, bool bUseExplicitOrSharedPCHs) {
    FString Template;
    if (!ReadCustomTemplateFile(TEXT("Module.Build.cs.template"), Template, OutFailReason))
    {
        return false;
    }

    FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *GetCopyrightLine(), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%PUBLIC_DEPENDENCY_MODULE_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(PublicDependencyModuleNames), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%PRIVATE_DEPENDENCY_MODULE_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(PrivateDependencyModuleNames), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);

    const FString PCHUsage = bUseExplicitOrSharedPCHs ? TEXT("UseExplicitOrSharedPCHs") : TEXT("UseSharedPCHs");
    FinalOutput = FinalOutput.Replace(TEXT("%PCH_USAGE%"), *PCHUsage, ESearchCase::CaseSensitive);

    return GameProjectUtils::WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

bool CppToolsUtil::GenerateModuleHeaderFile(const FString& NewHeaderFileName, const FString& ModuleName, const TArray<FString>& PublicHeaderIncludes, FText& OutFailReason) {
    FString Template;
    if (!ReadCustomTemplateFile(TEXT("Module.h.template"), Template, OutFailReason))
    {
        return false;
    }

    FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *GetCopyrightLine(), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%CLASS_MODULE_API_MACRO%"), *GetModuleAPIMacro(ModuleName, false), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%PUBLIC_HEADER_INCLUDES%"), *GameProjectUtils::MakeIncludeList(PublicHeaderIncludes), ESearchCase::CaseSensitive);

    return GameProjectUtils::WriteOutputFile(NewHeaderFileName, FinalOutput, OutFailReason);
}

bool CppToolsUtil::GenerateModuleCPPFile(const FString& NewCPPFileName, const FString& ModuleName, const FString& StartupSourceCode, const FString& ShutdownSourceCode, FText& OutFailReason) {
    FString Template;
    if (!ReadCustomTemplateFile(TEXT("Module.cpp.template"), Template, OutFailReason))
    {
        return false;
    }

    FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *GetCopyrightLine(), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_STARTUP_CODE%"), *StartupSourceCode, ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_SHUTDOWN_CODE%"), *StartupSourceCode, ESearchCase::CaseSensitive);

    return GameProjectUtils::WriteOutputFile(NewCPPFileName, FinalOutput, OutFailReason);
}

GameProjectUtils::EAddCodeToProjectResult CppToolsUtil::GenerateModule(const FString& ModulePath, const FString& ModuleName, const EHostType::Type& Type, bool bUsePCH, TArray<FString>& CreatedFiles, FText& OutFailReason) {

    //TODO: Add data validation

    TArray<FModuleDescriptor> GeneratedModules;

    FScopedSlowTask SlowTask(8, LOCTEXT("AddingModuleToProject", "Adding module to project..."));
    SlowTask.MakeDialog();

    SlowTask.EnterProgressFrame();

    // Module.Build.cs
    {
        const FString BuildFilename = ModulePath / ModuleName + TEXT(".Build.cs");
        TArray<FString> PublicDependencyModuleNames;
        PublicDependencyModuleNames.Add(TEXT("Core"));
        PublicDependencyModuleNames.Add(TEXT("CoreUObject"));
        PublicDependencyModuleNames.Add(TEXT("Engine"));
        PublicDependencyModuleNames.Add(TEXT("InputCore"));
        TArray<FString> PrivateDependencyModuleNames;
        if (CppToolsUtil::GenerateModuleBuildFile(BuildFilename, ModuleName, PublicDependencyModuleNames, PrivateDependencyModuleNames, OutFailReason, bUsePCH)) {
            GeneratedModules.Add(FModuleDescriptor(*ModuleName, Type));
            CreatedFiles.Add(BuildFilename);
        }
        else {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }

    SlowTask.EnterProgressFrame();

    auto Modifier = FProjectDescriptorModifier::CreateLambda(
        [&GeneratedModules](FProjectDescriptor& Descriptor)
        {
            // See GameProjectUtils.cpp L3920
            bool bNeedsUpdate = false;

            bNeedsUpdate |= AppendProjectModules(Descriptor, &GeneratedModules);
            //bNeedsUpdate |= UpdateRequiredAdditionalDependencies(Descriptor, RequiredDependencies, ModuleInfo.ModuleName);

            return bNeedsUpdate;
        });

    UpdateGameProject(&Modifier);

    SlowTask.EnterProgressFrame();

    // Module Header
    {
        const FString HeaderFilename = ModulePath / "Public" / ModuleName + TEXT(".h");
        TArray<FString> PublicHeaderIncludes;
        if (CppToolsUtil::GenerateModuleHeaderFile(HeaderFilename, ModuleName, PublicHeaderIncludes, OutFailReason)) {
            CreatedFiles.Add(HeaderFilename);
        }
        else {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }

    SlowTask.EnterProgressFrame();

    // Module Source
    {
        const FString SourceFilename = ModulePath / "Private" / ModuleName + TEXT(".cpp");
        FString StartupSource;
        FString ShutdownSource;
        if (CppToolsUtil::GenerateModuleCPPFile(SourceFilename, ModuleName, StartupSource, ShutdownSource, OutFailReason)) {
            CreatedFiles.Add(SourceFilename);
        }
        else {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }

    SlowTask.EnterProgressFrame();

    FString ProjectFileName = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*FPaths::GetProjectFilePath());
    //FString Arguments = FString::Printf(TEXT("%s %s %s -Plugin=\"%s\" -Project=\"%s\" -Progress -NoHotReloadFromIDE"), FPlatformMisc::GetUBTTargetName(), FModuleManager::Get().GetUBTConfiguration(), FPlatformMisc::GetUBTPlatform(), *UPluginFilePath, *ProjectFileName);
    FString Arguments = FString::Printf(TEXT("%s %s %s -Project=\"%s\" -Progress -NoHotReloadFromIDE"), FPlatformMisc::GetUBTTargetName(), FModuleManager::Get().GetUBTConfiguration(), FPlatformMisc::GetUBTPlatform(), *ProjectFileName);

    if (!FDesktopPlatformModule::Get()->RunUnrealBuildTool(LOCTEXT("Compiling", "Compiling..."), FPaths::RootDir(), Arguments, GWarn)) {
        // Failed to compile
    }

    FModuleManager::Get().ResetModulePathsCache();

    SlowTask.EnterProgressFrame();

    TArray<FString> CreatedFilesForExternalAppRead;
    CreatedFilesForExternalAppRead.Reserve(CreatedFiles.Num());
    for (const FString& CreatedFile : CreatedFiles)
    {
        CreatedFilesForExternalAppRead.Add(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*CreatedFile));
    }

    FSourceCodeNavigation::AddSourceFiles(CreatedFilesForExternalAppRead);

    SlowTask.EnterProgressFrame();

    // Mark the files for add in SCC
    ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
    if (ISourceControlModule::Get().IsEnabled() && SourceControlProvider.IsAvailable())
    {
        SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), CreatedFilesForExternalAppRead);
    }

    SlowTask.EnterProgressFrame(1.0f, LOCTEXT("CompilingCPlusPlusCode", "Compiling new C++ code.  Please wait..."));

    IHotReloadInterface& HotReloadSupport = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
    if (!HotReloadSupport.RecompileModule(*ModuleName, *GWarn, ERecompileModuleFlags::ReloadAfterRecompile | ERecompileModuleFlags::ForceCodeProject))
    {
        OutFailReason = LOCTEXT("FailedToCompileNewModule", "Failed to compile newly created module.");
        return GameProjectUtils::EAddCodeToProjectResult::FailedToHotReload;
    }

    FSourceCodeNavigation::AccessOnNewModuleAdded().Broadcast(*ModuleName);

    return GameProjectUtils::EAddCodeToProjectResult::Succeeded;
}

bool CppToolsUtil::UpdateGameProjectFile(const FString& ProjectFile, const FString& EngineIdentifier, const FProjectDescriptorModifier* Modifier, FText& OutFailReason) {

    // See GameProjectUtils.cpp L3661

    GameProjectUtils::TryMakeProjectFileWriteable(ProjectFile);

    FProjectDescriptor Descriptor;
    if (Descriptor.Load(ProjectFile, OutFailReason)) {
        if (Modifier && Modifier->IsBound() && !Modifier->Execute(Descriptor)) {
            return true;
        }
        return Descriptor.Save(ProjectFile, OutFailReason) && FDesktopPlatformModule::Get()->SetEngineIdentifierForProject(ProjectFile, EngineIdentifier);
    }
    return false;
}

bool CppToolsUtil::UpdateGameProject(const FProjectDescriptorModifier* Modifier) {

    // See GameProjectUtils.cpp L3568

    const FString& ProjectFilename = FPaths::GetProjectFilePath();
    const FString& ShortFilename = FPaths::GetCleanFilename(ProjectFilename);

    FText FailReason;
    FText UpdateMessage;
    SNotificationItem::ECompletionState NewCompletionState;

    if (UpdateGameProjectFile(ProjectFilename, FDesktopPlatformModule::Get()->GetCurrentEngineIdentifier(), Modifier, FailReason)) {
        // Project Updated Successfully
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        UpdateMessage = FText::Format(LOCTEXT("ProjectFileUpdateComplete", "{ShortFilename} was successfully updated."), Args);
        NewCompletionState = SNotificationItem::CS_Success;
        return true;
    }
    else {
        // The user chose to update, but the update failed. Notify the user.
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        Args.Add(TEXT("FailReason"), FailReason);
        UpdateMessage = FText::Format(LOCTEXT("ProjectFileUpdateFailed", "{ShortFilename} failed to update. {FailReason}"), Args);
        NewCompletionState = SNotificationItem::CS_Fail;
        return false;
    }

    /*if ( UpdateGameProjectNotification.IsValid() )
	{
		UpdateGameProjectNotification.Pin()->SetCompletionState(NewCompletionState);
		UpdateGameProjectNotification.Pin()->SetText(UpdateMessage);
		UpdateGameProjectNotification.Pin()->ExpireAndFadeout();
		UpdateGameProjectNotification.Reset();
	}*/
}

bool CppToolsUtil::AppendProjectModules(FProjectDescriptor& Descriptor, const TArray<FModuleDescriptor>* Modules) {
    // See GameProjectUtils.cpp L1070
    if (Modules == nullptr) return false;

    for (int32 Idx = 0; Idx < Modules->Num(); Idx++)
    {
        auto Element = (*Modules)[Idx];
        bool bContains = false;
        for (int32 I = 0; I < Descriptor.Modules.Num(); I++) {
            if (Descriptor.Modules[I].Name == (*Modules)[Idx].Name) {
                bContains = true;
                break;
            }
        }
        if (!bContains) Descriptor.Modules.Add(Element);
    }

    GameProjectUtils::ResetCurrentProjectModulesCache();

    return true;
}


#undef LOCTEXT_NAMESPACE