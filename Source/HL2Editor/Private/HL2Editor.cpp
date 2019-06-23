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
#include "Engine/TextureCube.h"

DEFINE_LOG_CATEGORY(LogHL2Editor);

#define LOCTEXT_NAMESPACE ""

void HL2EditorImpl::StartupModule()
{
	FUtilMenuStyle::Initialize();

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
		FUtilMenuCommands::Get().ConvertSkyboxes,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::ConvertSkyboxes),
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
	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	levelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(myExtender);

	FUtilMenuCommands::Unregister();

	FUtilMenuStyle::Shutdown();
}

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

TSharedRef<SWidget> HL2EditorImpl::GenerateUtilityMenu(TSharedRef<FUICommandList> commandList)
{
	FMenuBuilder menuBuilder(true, commandList);

	menuBuilder.BeginSection("Asset Import");
	{
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportTextures);
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportMaterials);
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().ConvertSkyboxes);
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
	FScopedSlowTask loopProgress(groupedFilesToImport.Num(), LOCTEXT("TexturesImporting", "Importing vtfs..."));
	loopProgress.MakeDialog();
	for (const auto& pair : groupedFilesToImport)
	{
		FString dir = pair.Key;
		if (FPaths::MakePathRelativeTo(dir, *rootPath))
		{
			loopProgress.EnterProgressFrame();
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
	FScopedSlowTask loopProgress(groupedFilesToImport.Num(), LOCTEXT("MaterialsImporting", "Importing vmts..."));
	loopProgress.MakeDialog();
	for (const auto& pair : groupedFilesToImport)
	{
		FString dir = pair.Key;
		if (FPaths::MakePathRelativeTo(dir, *rootPath))
		{
			loopProgress.EnterProgressFrame();
			TArray<UObject*> importedAssets = assetTools.ImportAssets(pair.Value, IHL2Runtime::Get().GetHL2MaterialBasePath() / dir);
			UE_LOG(LogHL2Editor, Log, TEXT("Imported %d assets to '%s'"), importedAssets.Num(), *dir);
		}
	}
}

void HL2EditorImpl::ConvertSkyboxes()
{
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();

	TArray<FAssetData> assetDatas;
	FString texturePath = IHL2Runtime::Get().GetHL2TextureBasePath() / TEXT("skybox");
	assetRegistry.GetAssetsByPath(FName(*texturePath), assetDatas);

	TArray<FString> skyboxNames;

	for (const FAssetData& assetData : assetDatas)
	{
		FString assetName = assetData.AssetName.ToString();
		if (assetData.GetClass() == UTexture2D::StaticClass() && assetName.EndsWith(TEXT("bk")))
		{
			assetName.RemoveFromEnd(TEXT("bk"));
			skyboxNames.AddUnique(assetName);
		}
	}

	FScopedSlowTask loopProgress(skyboxNames.Num(), LOCTEXT("SkyboxesConverting", "Converting skyboxes to cubemaps..."));
	loopProgress.MakeDialog();
	for (const FString& skyboxName : skyboxNames)
	{
		loopProgress.EnterProgressFrame();
		FString packageName = IHL2Runtime::Get().GetHL2TextureBasePath() / TEXT("skybox") / skyboxName;
		FAssetData existingAsset = assetRegistry.GetAssetByObjectPath(FName(*(packageName + TEXT(".") + skyboxName)));
		if (existingAsset.IsValid())
		{
			UTextureCube* existingTexture = CastChecked<UTextureCube>(existingAsset.GetAsset());
			ConvertSkybox(existingTexture, skyboxName);
			existingTexture->PostEditChange();
			existingTexture->MarkPackageDirty();
		}
		else
		{
			UPackage* package = CreatePackage(nullptr, *packageName);
			UTextureCube* cubeMap = NewObject<UTextureCube>(package, FName(*skyboxName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			ConvertSkybox(cubeMap, skyboxName);
			cubeMap->PostEditChange();
			FAssetRegistryModule::AssetCreated(cubeMap);
			cubeMap->MarkPackageDirty();
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
	FBSPImporter importer(fileName);
	if (importer.Load())
	{
		importer.ImportToCurrentLevel();
	}
}

void HL2EditorImpl::ConvertSkybox(UTextureCube* texture, const FString& skyboxName)
{
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();

	// Fetch all 6 faces
	const FString& hl2TextureBasePath = IHL2Runtime::Get().GetHL2TextureBasePath();
	UTexture2D* textures[6] =
	{
		CastChecked<UTexture2D>(assetRegistry.GetAssetByObjectPath(FName(*(hl2TextureBasePath / TEXT("skybox") / skyboxName + TEXT("ft.") + skyboxName + TEXT("ft")))).GetAsset()), // +X, -90
		CastChecked<UTexture2D>(assetRegistry.GetAssetByObjectPath(FName(*(hl2TextureBasePath / TEXT("skybox") / skyboxName + TEXT("bk.") + skyboxName + TEXT("bk")))).GetAsset()), // -X, +90
		CastChecked<UTexture2D>(assetRegistry.GetAssetByObjectPath(FName(*(hl2TextureBasePath / TEXT("skybox") / skyboxName + TEXT("lf.") + skyboxName + TEXT("lf")))).GetAsset()), // +Y, +180
		CastChecked<UTexture2D>(assetRegistry.GetAssetByObjectPath(FName(*(hl2TextureBasePath / TEXT("skybox") / skyboxName + TEXT("rt.") + skyboxName + TEXT("rt")))).GetAsset()), // -Y, +0
		CastChecked<UTexture2D>(assetRegistry.GetAssetByObjectPath(FName(*(hl2TextureBasePath / TEXT("skybox") / skyboxName + TEXT("up.") + skyboxName + TEXT("up")))).GetAsset()), // +Z, +0
		CastChecked<UTexture2D>(assetRegistry.GetAssetByObjectPath(FName(*(hl2TextureBasePath / TEXT("skybox") / skyboxName + TEXT("dn.") + skyboxName + TEXT("dn")))).GetAsset())  // -Z, +0
	};

	// Determine size
	const bool hdr = skyboxName.EndsWith(TEXT("hdr"));
	const int width = textures[0]->Source.GetSizeX();
	const int height = textures[0]->Source.GetSizeY();
	check(width == height);

	// Initialise texture data
	if (hdr)
	{
		texture->SRGB = false;
		texture->CompressionSettings = TextureCompressionSettings::TC_HDR;
	}
	else
	{
		texture->SRGB = true;
		texture->CompressionSettings = TextureCompressionSettings::TC_Default;
	}
	texture->Source.Init(width, height, 6, 1, hdr ? ETextureSourceFormat::TSF_RGBA16 : ETextureSourceFormat::TSF_BGRA8);

	// Copy texture data
	int mipSize = texture->Source.CalcMipSize(0) / (hdr ? 12 : 6);
	uint8* dstData = hdr ? new uint8[mipSize * 6] : texture->Source.LockMip(0);
	for (int i = 0; i < 6; ++i)
	{
		UTexture* srcTex = textures[i];
		const uint8* srcData = srcTex->Source.LockMip(0);
		switch (i)
		{
			case 0:
			{
				// Rotate 90 degrees CCW
				const uint32* srcPixels = (uint32*)srcData;
				uint32* dstPixels = (uint32*)(dstData + i * mipSize);
				for (int y = 0; y < height; ++y)
				{
					for (int x = 0; x < width; ++x)
					{
						dstPixels[y * height + x] = srcPixels[x * height + (height - (y + 1))];
					}
				}
				break;
			}
			case 1:
			{
				// Rotate 90 degrees CW
				const uint32* srcPixels = (uint32*)srcData;
				uint32* dstPixels = (uint32*)(dstData + i * mipSize);
				for (int y = 0; y < height; ++y)
				{
					for (int x = 0; x < width; ++x)
					{
						dstPixels[y * height + x] = srcPixels[(width - (x + 1)) * height + y];
					}
				}
				break;
			}
			case 2:
			{
				// Rotate 180 degrees
				const uint32* srcPixels = (uint32*)srcData;
				uint32* dstPixels = (uint32*)(dstData + i * mipSize);
				for (int y = 0; y < height; ++y)
				{
					for (int x = 0; x < width; ++x)
					{
						dstPixels[y * height + x] = srcPixels[(height - (y + 1)) * height + (width - (x + 1))];
					}
				}
				break;
			}
			default:
				FMemory::Memcpy(dstData + i * mipSize, srcData, mipSize);
				break;
		}		
		srcTex->Source.UnlockMip(0);
	}
	if (hdr)
	{
		uint8* mipDstData = texture->Source.LockMip(0);
		for (int i = 0; i < 6; ++i)
		{
			uint64* dstPixels = (uint64*)(mipDstData + i * mipSize * 2);
			uint32* srcPixels = (uint32*)(dstData + i * mipSize);
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					dstPixels[y * height + x] = HDRDecompress(srcPixels[y * height + x]);
				}
			}
		}
		delete[] dstData;
	}
	texture->Source.UnlockMip(0);
}

uint64 HL2EditorImpl::HDRDecompress(uint32 pixel)
{
	const uint8 ldrA = pixel >> 0x18;
	const uint8 ldrR = (pixel >> 0x10) & 0xff;
	const uint8 ldrG = (pixel >> 0x8) & 0xff;
	const uint8 ldrB = pixel & 0xff;

	const uint16 hdrR = (uint16)FMath::Clamp((uint32)ldrR * (uint32)ldrA * 16, 0u, 0xffffu);
	const uint16 hdrG = (uint16)FMath::Clamp((uint32)ldrG * (uint32)ldrA * 16, 0u, 0xffffu);
	const uint16 hdrB = (uint16)FMath::Clamp((uint32)ldrB * (uint32)ldrA * 16, 0u, 0xffffu);
	const uint16 hdrA = 0xffffu;

	return ((uint64)hdrA << 0x30u) | ((uint64)hdrB << 0x20u) | ((uint64)hdrG << 0x10u) | (uint64)hdrR;
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(HL2EditorImpl, HL2Editor)