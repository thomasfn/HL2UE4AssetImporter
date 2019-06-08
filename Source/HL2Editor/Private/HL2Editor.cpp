#include "HL2EditorPrivatePCH.h"

#include "HL2Editor.h"

#include "IHL2Runtime.h"
#include "Engine/Texture.h"
#include "BSPImporter.h"
#include "Framework/SlateDelegates.h"
#include "UtilMenuCommands.h"
#include "UtilMenuStyle.h"
#include "DesktopPlatformModule.h"
#include "AssetToolsModule.h"
#include "PlatformFilemanager.h"
#include "VMTMaterial.h"
#include "MaterialUtils.h"

DEFINE_LOG_CATEGORY(LogHL2Editor);

void HL2EditorImpl::StartupModule()
{
	isLoading = true;

	FUtilMenuStyle::Initialize();

	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	handleFilesLoaded = assetRegistry.OnFilesLoaded().AddRaw(this, &HL2EditorImpl::HandleFilesLoaded);
	handleAssetAdded = assetRegistry.OnAssetAdded().AddRaw(this, &HL2EditorImpl::HandleAssetAdded);

	FUtilMenuCommands::Register();

	utilMenuCommandList = MakeShareable(new FUICommandList);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().BulkImportTextures,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::BulkImportTexturesClicked),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().BulkImportMaterials,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::BulkImportMaterialsClicked),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().ImportBSP,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::ImportBSPClicked),
		FCanExecuteAction()
	);

	myExtender = MakeShareable(new FExtender);
	myExtender->AddToolBarExtension("Content", EExtensionHook::After, NULL, FToolBarExtensionDelegate::CreateRaw(this, &HL2EditorImpl::AddToolbarExtension));

	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	levelEditorModule.GetToolBarExtensibilityManager()->AddExtender(myExtender);
}

void HL2EditorImpl::ShutdownModule()
{
	handleAssetAdded.Reset();
	handleFilesLoaded.Reset();

	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	levelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(myExtender);

	FUtilMenuCommands::Unregister();

	FUtilMenuStyle::Shutdown();
}

void HL2EditorImpl::HandleFilesLoaded()
{
	isLoading = false;
	handleFilesLoaded.Reset();
}

void HL2EditorImpl::HandleAssetAdded(const FAssetData& assetData)
{
	if (isLoading) { return; }
	if (Cast<UTexture>(assetData.GetAsset()))
	{
		TArray<UVMTMaterial*> materials;
		IHL2Runtime::Get().FindAllMaterialsThatReferenceTexture(assetData.ObjectPath, materials);
		for (UVMTMaterial* material : materials)
		{
			FMaterialUtils::TryResolveTextures(material);
			material->PostEditChange();
			material->MarkPackageDirty();
			UE_LOG(LogHL2Editor, Log, TEXT("Trying to resolve textures on '%s' as '%s' now exists"), *material->GetName(), *assetData.AssetName.ToString());
		}
	}
}

#define LOCTEXT_NAMESPACE ""

void HL2EditorImpl::AddToolbarExtension(FToolBarBuilder& builder)
{
	FUIAction UtilMenuAction;

	builder.AddComboButton(
		UtilMenuAction,
		FOnGetContent::CreateStatic(&HL2EditorImpl::GenerateUtilityMenu, utilMenuCommandList.ToSharedRef()),
		LOCTEXT("HL2UtilMenu_Label", "HL2"),
		LOCTEXT("HL2UtilMenu_Tooltip", "HL2 Importer Utilities"),
		FSlateIcon(FUtilMenuStyle::GetStyleSetName(), "HL2AssetImporter.UtilityMenu")
	);
}

#undef LOCTEXT_NAMESPACE

TSharedRef<SWidget> HL2EditorImpl::GenerateUtilityMenu(TSharedRef<FUICommandList> commandList)
{
	FMenuBuilder menuBuilder(true, commandList);

	menuBuilder.BeginSection("Asset Import");
	{
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportTextures);
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportMaterials);
	}
	menuBuilder.EndSection();

	menuBuilder.BeginSection("Map Import");
	{
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().ImportBSP);
	}
	menuBuilder.EndSection();

	return menuBuilder.MakeWidget();
}

void HL2EditorImpl::BulkImportTexturesClicked()
{
	// Ask user to select folder to import from
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	FString rootPath;
	if (!desktopPlatform->OpenDirectoryDialog(nullptr, TEXT("Choose VTF Location"), TEXT(""), rootPath)) { return; }
	rootPath += "/";

	// Scan folder
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> filesToImport;
	platformFile.FindFilesRecursively(filesToImport, *rootPath, TEXT(".vtf"));
	UE_LOG(LogHL2Editor, Log, TEXT("Importing %d files from '%s'..."), filesToImport.Num(), *rootPath);

	// Import all
	IAssetTools& assetTools = FAssetToolsModule::GetModule().Get();
	TMap<FString, TArray<FString>> groupedFilesToImport;
	GroupFileListByDirectory(filesToImport, groupedFilesToImport);
	for (const auto& pair : groupedFilesToImport)
	{
		FString dir = pair.Key;
		if (FPaths::MakePathRelativeTo(dir, *rootPath))
		{
			TArray<UObject*> importedAssets = assetTools.ImportAssets(pair.Value, IHL2Runtime::Get().GetHL2TextureBasePath() / dir);
			UE_LOG(LogHL2Editor, Log, TEXT("Imported %d assets to '%s'"), importedAssets.Num(), *dir);
		}
	}
}

void HL2EditorImpl::BulkImportMaterialsClicked()
{
	// Ask user to select folder to import from
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	FString rootPath;
	if (!desktopPlatform->OpenDirectoryDialog(nullptr, TEXT("Choose VMT Location"), TEXT(""), rootPath)) { return; }
	rootPath += "/";

	// Scan folder
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> filesToImport;
	platformFile.FindFilesRecursively(filesToImport, *rootPath, TEXT(".vmt"));
	UE_LOG(LogHL2Editor, Log, TEXT("Importing %d files from '%s'..."), filesToImport.Num(), *rootPath);

	// Import all
	IAssetTools& assetTools = FAssetToolsModule::GetModule().Get();
	TMap<FString, TArray<FString>> groupedFilesToImport;
	GroupFileListByDirectory(filesToImport, groupedFilesToImport);
	for (const auto& pair : groupedFilesToImport)
	{
		FString dir = pair.Key;
		if (FPaths::MakePathRelativeTo(dir, *rootPath))
		{
			TArray<UObject*> importedAssets = assetTools.ImportAssets(pair.Value, IHL2Runtime::Get().GetHL2MaterialBasePath() / dir);
			UE_LOG(LogHL2Editor, Log, TEXT("Imported %d assets to '%s'"), importedAssets.Num(), *dir);
		}
	}
}

void HL2EditorImpl::ImportBSPClicked()
{
	// Ask user to select BSP to import
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	TArray<FString> selectedFilenames;
	if (!desktopPlatform->OpenFileDialog(nullptr, "Import BSP", TEXT(""), TEXT(""), TEXT("bsp;Valve Map File"), EFileDialogFlags::None, selectedFilenames)) { return; }
	if (selectedFilenames.Num() != 1) { return; }
	const FString& fileName = selectedFilenames[0];

	// Run import
	if (!FBSPImporter::ImportToCurrentLevel(fileName))
	{
		return;
	}
}

void HL2EditorImpl::GroupFileListByDirectory(const TArray<FString>& files, TMap<FString, TArray<FString>>& outMap)
{
	for (const FString& file : files)
	{
		FString path = FPaths::GetPath(file);
		if (!outMap.Contains(path))
		{
			outMap.Add(path, TArray<FString>());
		}
		outMap[path].Add(file);
	}
}

IMPLEMENT_MODULE(HL2EditorImpl, HL2Editor)