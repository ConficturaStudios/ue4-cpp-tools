// Fill out your copyright notice in the Description page of Project Settings.


#include "CppToolsUtil.h"

#include "Editor/EditorPerProjectUserSettings.h"
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "CppToolsUtil"


FString CppToolsUtil::CppToolsContentDir() {
    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("CppTools"))->GetContentDir();
    return ContentDir;
}

bool CppToolsUtil::CheckoutFile(const FString& Filename, FText& OutFailReason)
{
    // See GameProjectUtils.cpp L3700
    
    if ( !ensure(Filename.Len()) )
    {
        OutFailReason = LOCTEXT("NoFilename", "The filename was not specified.");
        return false;
    }

    if ( !ISourceControlModule::Get().IsEnabled() )
    {
        OutFailReason = LOCTEXT("SCCDisabled", "Source control is not enabled. Enable source control in the preferences menu.");
        return false;
    }

    FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(Filename);
    ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
    FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
    TArray<FString> FilesToBeCheckedOut;
    FilesToBeCheckedOut.Add(AbsoluteFilename);

    bool bSuccessfullyCheckedOut = false;
    OutFailReason = LOCTEXT("SCCStateInvalid", "Could not determine source control state.");

    if(SourceControlState.IsValid())
    {
        if(SourceControlState->IsCheckedOut() || SourceControlState->IsAdded() || !SourceControlState->IsSourceControlled())
        {
            // Already checked out or opened for add... or not in the depot at all
            bSuccessfullyCheckedOut = true;
        }
        else if(SourceControlState->CanCheckout() || SourceControlState->IsCheckedOutOther())
        {
            bSuccessfullyCheckedOut = (SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), FilesToBeCheckedOut) == ECommandResult::Succeeded);
            if (!bSuccessfullyCheckedOut)
            {
                OutFailReason = LOCTEXT("SCCCheckoutFailed", "Failed to check out the project file.");
            }
        }
        else if(!SourceControlState->IsCurrent())
        {
            OutFailReason = LOCTEXT("SCCNotCurrent", "The project file is not at head revision.");
        }
    }

    return bSuccessfullyCheckedOut;
}

void CppToolsUtil::TryMakePluginFileWriteable(const FString& PluginFile)
{
    // See GameProjectUtils.cpp L3629

    // First attempt to check out the file if SCC is enabled
    if ( ISourceControlModule::Get().IsEnabled() )
    {
        FText FailReason;
        CheckoutFile(PluginFile, FailReason);
    }

    // Check if it's writable
    if(FPlatformFileManager::Get().GetPlatformFile().IsReadOnly(*PluginFile))
    {
        FText ShouldMakePluginWriteable = LOCTEXT("ShouldMakePluginWriteable_Message", "'{PluginFilename}' is read-only and cannot be updated. Would you like to make it writeable?");

        FFormatNamedArguments Arguments;
        Arguments.Add( TEXT("PluginFilename"), FText::FromString(PluginFile));

        if(FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(ShouldMakePluginWriteable, Arguments)) == EAppReturnType::Yes)
        {
            FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*PluginFile, false);
        }
    }
}

void CppToolsUtil::PushNotification(const FText& Text,
    const SNotificationItem::ECompletionState& CompletionState = SNotificationItem::ECompletionState::CS_None)
{
    const FNotificationInfo Notification(Text);
    auto SNotify = FSlateNotificationManager::Get().AddNotification(Notification);
    SNotify->SetCompletionState(CompletionState);
    SNotify->ExpireAndFadeout();
    SNotify.Reset();
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

bool CppToolsUtil::GenerateModuleBuildFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& PublicDependencyModuleNames,
    const TArray<FString>& PrivateDependencyModuleNames, FText& OutFailReason, bool bUseExplicitOrSharedPCHs) {
    
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

bool CppToolsUtil::InsertDependencyIntoPrimaryBuild(const FString& ModuleName, TSharedPtr<IPlugin> Target,
    const bool& bIsEditor, FText& OutFailReason)
{
    FString FileContents;
    
    FString TargetName = !Target.IsValid() ? FApp::GetProjectName() : Target->GetName();
    if (bIsEditor)
    {
        TargetName += "Editor";
    }
    const FString SourcePath = (!Target.IsValid()) ? FPaths::GameSourceDir() : Target->GetBaseDir() / "Source";
    
    const FString TargetPath = SourcePath / TargetName / TargetName + ".Build.cs";
    
    if (FFileHelper::LoadFileToString(FileContents, *TargetPath))
    {

        // Check for existing ExtraModuleNames.AddRange command
        const FRegexPattern PublicDependenciesPattern(
            TEXT("PublicDependencyModuleNames\\.AddRange\\([\\s]*new string\\[\\][\\s]*\\{[\\s]*(.*)[\\s]*\\}[\\s]*\\);"));
        FRegexMatcher PublicDependenciesMatcher(PublicDependenciesPattern, FileContents);

        if (PublicDependenciesMatcher.FindNext())
        {
            int32 start = PublicDependenciesMatcher.GetMatchBeginning();
            int32 end = PublicDependenciesMatcher.GetMatchEnding();

            FileContents.RemoveAt(start, end - start);
            
            FString InsertionText = TEXT("PublicDependencyModuleNames.AddRange( new string[] { MODULES } );");
            InsertionText = InsertionText.Replace(TEXT("MODULES"),
                *(PublicDependenciesMatcher.GetCaptureGroup(1) + ", \"" + ModuleName + "\""), ESearchCase::CaseSensitive);
            
            FileContents.InsertAt(start, InsertionText);

            FFileHelper::SaveStringToFile(FileContents, *TargetPath);

            return true;
        }
    }

    // Issue modifying or finding file

    FFormatNamedArguments Args;
    Args.Add(TEXT("FullFileName"), FText::FromString(TargetPath));
    OutFailReason = FText::Format(LOCTEXT("FailedToReadBuildFile", "Failed to update \"{FullFileName}\""), Args);
    return false;
}

bool CppToolsUtil::InsertDependencyIntoTarget(const FString& ModuleName, const bool& bIsEditor, FText& OutFailReason)
{
    FString FileContents;
    
    FString PrimaryGameTargetName = FApp::GetProjectName();
    if (bIsEditor)
    {
        PrimaryGameTargetName += "Editor";
    }
    
    const FString TargetPath = FPaths::GameSourceDir() / PrimaryGameTargetName + ".Target.cs";
    
    if (FFileHelper::LoadFileToString(FileContents, *TargetPath))
    {

        // Check for existing ExtraModuleNames.AddRange command
        const FRegexPattern ExtraModulesPattern(
            TEXT("ExtraModuleNames\\.AddRange\\([\\s]*new string\\[\\][\\s]*\\{[\\s]*(.*)[\\s]*\\}[\\s]*\\);"));
        FRegexMatcher ExtraModulesMatcher(ExtraModulesPattern, FileContents);

        if (ExtraModulesMatcher.FindNext())
        {
            int32 start = ExtraModulesMatcher.GetMatchBeginning();
            int32 end = ExtraModulesMatcher.GetMatchEnding();

            FileContents.RemoveAt(start, end - start);
            
            FString InsertionText = TEXT("ExtraModuleNames.AddRange( new string[] { MODULES } );");
            InsertionText = InsertionText.Replace(TEXT("MODULES"),
                *(ExtraModulesMatcher.GetCaptureGroup(1) + ", \"" + ModuleName + "\""), ESearchCase::CaseSensitive);
            
            FileContents.InsertAt(start, InsertionText);

            FFileHelper::SaveStringToFile(FileContents, *TargetPath);

            return true;
        }
        
        // Insert new ExtraModuleNames.AddRange command if not found
        /*const FRegexPattern TargetConstructorPattern(TEXT("public[\\s]+") + PrimaryGameTargetName
            + TEXT("[\\s]*\\(.*\\)[\\s]*:[\\s]*base\\(.*\\)[\\s]*\\{.*\\}"));
        FRegexMatcher TargetConstructorMatcher(TargetConstructorPattern, FileContents);

        if (TargetConstructorMatcher.FindNext())
        {
            int32 end = TargetConstructorMatcher.GetMatchEnding();
            
            FString InsertionText = TEXT("\tExtraModuleNames.AddRange( new string[] { MODULES } );");
            InsertionText += LINE_TERMINATOR;
            InsertionText += "\t";
            InsertionText = InsertionText.Replace(TEXT("MODULES"),
                *(PrimaryGameTargetName + ", " + ModuleName), ESearchCase::CaseSensitive);
            
            FileContents.InsertAt(end - 1, InsertionText);

            FFileHelper::SaveStringToFile(FileContents, *TargetPath);

            return true;
        }*/
    }

    // Issue modifying or finding file

    FFormatNamedArguments Args;
    Args.Add(TEXT("FullFileName"), FText::FromString(TargetPath));
    OutFailReason = FText::Format(LOCTEXT("FailedToReadTargetFile", "Failed to update \"{FullFileName}\""), Args);
    return false;
}

GameProjectUtils::EAddCodeToProjectResult CppToolsUtil::GenerateModule(const FString& ModulePath, TSharedPtr<IPlugin> Target,
    const FString& ModuleName, const EHostType::Type& Type, bool bUsePCH, TArray<FString>& CreatedFiles, FText& OutFailReason)
{

    //TODO: Add data validation

    TArray<FModuleDescriptor> GeneratedModules;

    FScopedSlowTask SlowTask(11, LOCTEXT("AddingModuleToProject", "Adding module to project..."));
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

    // Add to primary game module .Build.cs
    {
        if (!InsertDependencyIntoPrimaryBuild(ModuleName, Target, Type == EHostType::Editor, OutFailReason))
        {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }
    
    SlowTask.EnterProgressFrame();

    // Add to project target
    {
        if (!InsertDependencyIntoTarget(ModuleName, Type == EHostType::Editor, OutFailReason))
        {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }

    SlowTask.EnterProgressFrame();

    if (!Target.IsValid())
    {
        // Update .uproject file
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
    }
    else
    {
        // Update .uplugin file
        auto Modifier = FPluginDescriptorModifier::CreateLambda(
            [&GeneratedModules](FPluginDescriptor& Descriptor)
            {
                bool bNeedsUpdate = false;
                bNeedsUpdate |= AppendPluginModules(Descriptor, &GeneratedModules);
                return bNeedsUpdate;
            });

        UpdatePlugin(Target, &Modifier);
    }

    SlowTask.EnterProgressFrame();

    // Rebuild project

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

    SlowTask.EnterProgressFrame();

    // Mark the files for add in SCC
    ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
    if (ISourceControlModule::Get().IsEnabled() && SourceControlProvider.IsAvailable())
    {
        SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), CreatedFilesForExternalAppRead);
    }

    SlowTask.EnterProgressFrame();

    // Attempt to add files to solution
    if (!FSourceCodeNavigation::AddSourceFiles(CreatedFilesForExternalAppRead))
	{
		// Generate project files if we happen to be using a project file.
		if ( !FDesktopPlatformModule::Get()->GenerateProjectFiles(FPaths::RootDir(), FPaths::GetProjectFilePath(), GWarn) )
		{
			OutFailReason = LOCTEXT("FailedToGenerateProjectFiles", "Failed to generate project files.");
			return GameProjectUtils::EAddCodeToProjectResult::FailedToHotReload;
		}
	}

    SlowTask.EnterProgressFrame(1.0f, LOCTEXT("CompilingCPlusPlusCode", "Compiling new C++ code.  Please wait..."));

    // Hot reload files
    // See: GameProjectUtils.cpp L4043
    
    IHotReloadInterface& HotReloadSupport = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
    if (!HotReloadSupport.RecompileModule(*ModuleName, *GWarn, ERecompileModuleFlags::ReloadAfterRecompile))
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
        PushNotification(UpdateMessage, NewCompletionState);
        return true;
    }
    else {
        // The user chose to update, but the update failed. Notify the user.
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        Args.Add(TEXT("FailReason"), FailReason);
        UpdateMessage = FText::Format(LOCTEXT("ProjectFileUpdateFailed", "{ShortFilename} failed to update. {FailReason}"), Args);
        NewCompletionState = SNotificationItem::CS_Fail;
        PushNotification(UpdateMessage, NewCompletionState);
        return false;
    }
}

bool CppToolsUtil::AppendProjectModules(FProjectDescriptor& Descriptor, const TArray<FModuleDescriptor>* Modules) {
    // See GameProjectUtils.cpp L1070
    if (Modules == nullptr) return false;

    for (int32 Idx = 0; Idx < Modules->Num(); Idx++)
    {
        auto Element = (*Modules)[Idx];
        bool bContains = false;
        bool bUpdatedPrimary = false;
        for (int32 I = 0; I < Descriptor.Modules.Num(); I++) {
            if (!bContains && Descriptor.Modules[I].Name == Element.Name) {
                bContains = true;
                if (bUpdatedPrimary) break;
            }
            if (!bUpdatedPrimary && Descriptor.Modules[I].Name == FApp::GetProjectName())
            {
                auto& AdditionalDependencies = Descriptor.Modules[I].AdditionalDependencies;
                if (!AdditionalDependencies.Contains(Element.Name.ToString()))
                {
                    AdditionalDependencies.Add(Element.Name.ToString());
                }
                bUpdatedPrimary = true;
                if (bContains) break;
            }
        }
        if (!bContains) Descriptor.Modules.Add(Element);
    }

    GameProjectUtils::ResetCurrentProjectModulesCache();

    return true;
}

bool CppToolsUtil::UpdatePluginFile(const FString& PluginFile, const FPluginDescriptorModifier* Modifier, FText& OutFailReason)
{
    TryMakePluginFileWriteable(PluginFile);

    FPluginDescriptor Descriptor;
    if (Descriptor.Load(PluginFile, OutFailReason)) {
        if (Modifier && Modifier->IsBound() && !Modifier->Execute(Descriptor)) {
            return true;
        }
        return Descriptor.Save(PluginFile, OutFailReason);
    }
    return false;
}

bool CppToolsUtil::UpdatePlugin(const TSharedPtr<IPlugin>& Plugin, const FPluginDescriptorModifier* Modifier)
{
    if (!Plugin.IsValid()) return false;
    
    const FString& PluginFilename = Plugin->GetDescriptorFileName();
    const FString& ShortFilename = FPaths::GetCleanFilename(PluginFilename);

    FText FailReason;
    FText UpdateMessage;
    SNotificationItem::ECompletionState NewCompletionState;

    if (UpdatePluginFile(PluginFilename, Modifier, FailReason)) {
        // Plugin Updated Successfully
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        UpdateMessage = FText::Format(LOCTEXT("PluginFileUpdateComplete", "{ShortFilename} was successfully updated."), Args);
        NewCompletionState = SNotificationItem::CS_Success;
        PushNotification(UpdateMessage, NewCompletionState);
        return true;
    }
    else {
        // The user chose to update, but the update failed. Notify the user.
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        Args.Add(TEXT("FailReason"), FailReason);
        UpdateMessage = FText::Format(LOCTEXT("PluginFileUpdateFailed", "{ShortFilename} failed to update. {FailReason}"), Args);
        NewCompletionState = SNotificationItem::CS_Fail;
        PushNotification(UpdateMessage, NewCompletionState);
        return false;
    }
}

bool CppToolsUtil::AppendPluginModules(FPluginDescriptor& Descriptor, const TArray<FModuleDescriptor>* Modules)
{
    if (Modules == nullptr) return false;

    for (int32 Idx = 0; Idx < Modules->Num(); Idx++)
    {
        auto Element = (*Modules)[Idx];
        bool bContains = false;
        bool bUpdatedPrimary = false;
        for (int32 I = 0; I < Descriptor.Modules.Num(); I++) {
            if (!bContains && Descriptor.Modules[I].Name == Element.Name) {
                bContains = true;
                if (bUpdatedPrimary) break;
            }
            if (!bUpdatedPrimary && Descriptor.Modules[I].Name == FApp::GetProjectName())
            {
                auto& AdditionalDependencies = Descriptor.Modules[I].AdditionalDependencies;
                if (!AdditionalDependencies.Contains(Element.Name.ToString()))
                {
                    AdditionalDependencies.Add(Element.Name.ToString());
                }
                bUpdatedPrimary = true;
                if (bContains) break;
            }
        }
        if (!bContains) Descriptor.Modules.Add(Element);
    }

    GameProjectUtils::ResetCurrentProjectModulesCache();

    return true;
}

#undef LOCTEXT_NAMESPACE
