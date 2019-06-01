#include "HL2EditorPrivatePCH.h"

#include "VTFFactory.h"

#include "Runtime/Core/Public/Misc/FeedbackContext.h"
#include "Runtime/Engine/Classes/EditorFramework/AssetImportData.h"
#include "Runtime/Engine/Classes/Engine/VolumeTexture.h"
#include "Runtime/Engine/Classes/Engine/TextureCube.h"
#include "Runtime/JsonUtilities/Public/JsonObjectConverter.h"

UVTFFactory::UVTFFactory()
{
	// We only want to create assets by importing files.
	// Set this to true if you want to be able to create new, empty Assets from
	// the editor.
	bCreateNew = false;

	// Our Asset will be imported from a text file (xml), not a binary file
	bText = false;

	// Allows us to actually use the "Import" button in the Editor for this Asset
	bEditorImport = true;

	// Tells the Editor which file types we can import
	Formats.Add(TEXT("vtf;Valve Texture Format Files"));

	// Tells the Editor which Asset type this UFactory can import
	SupportedClass = UTexture2D::StaticClass();
}

UVTFFactory::~UVTFFactory()
{
}

// Begin UFactory Interface

/** Imports the OpenStreetMapFile from the text of the .osm xml file. */
UObject* UVTFFactory::FactoryCreateBinary(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	const TCHAR* Type,
	const uint8*& Buffer,
	const uint8* BufferEnd,
	FFeedbackContext* Warn
)
{
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

	// if the texture already exists, remember the user settings
	UTexture* ExistingTexture = FindObject<UTexture>(InParent, *InName.ToString());
	UTexture2D* ExistingTexture2D = FindObject<UTexture2D>(InParent, *InName.ToString());

	TextureAddress						ExistingAddressX = TA_Wrap;
	TextureAddress						ExistingAddressY = TA_Wrap;
	TextureFilter						ExistingFilter = TF_Default;
	TextureGroup						ExistingLODGroup = TEXTUREGROUP_World;
	TextureCompressionSettings			ExistingCompressionSettings = TC_Default;
	int32								ExistingLODBias = 0;
	int32								ExistingNumCinematicMipLevels = 0;
	bool								ExistingNeverStream = false;
	bool								ExistingSRGB = false;
	bool								ExistingPreserveBorder = false;
	bool								ExistingNoCompression = false;
	bool								ExistingNoAlpha = false;
	bool								ExistingDeferCompression = false;
	bool 								ExistingDitherMipMapAlpha = false;
	bool 								ExistingFlipGreenChannel = false;
	float								ExistingAdjustBrightness = 1.0f;
	float								ExistingAdjustBrightnessCurve = 1.0f;
	float								ExistingAdjustVibrance = 0.0f;
	float								ExistingAdjustSaturation = 1.0f;
	float								ExistingAdjustRGBCurve = 1.0f;
	float								ExistingAdjustHue = 0.0f;
	float								ExistingAdjustMinAlpha = 0.0f;
	float								ExistingAdjustMaxAlpha = 1.0f;
	FVector4							ExistingAlphaCoverageThresholds = FVector4(0, 0, 0, 0);
	TextureMipGenSettings				ExistingMipGenSettings = TextureMipGenSettings(0);

	bUsingExistingSettings = true;

	if (ExistingTexture && bUsingExistingSettings)
	{
		// save settings
		if (ExistingTexture2D)
		{
			ExistingAddressX = ExistingTexture2D->AddressX;
			ExistingAddressY = ExistingTexture2D->AddressY;
		}
		ExistingFilter = ExistingTexture->Filter;
		ExistingLODGroup = ExistingTexture->LODGroup;
		ExistingCompressionSettings = ExistingTexture->CompressionSettings;
		ExistingLODBias = ExistingTexture->LODBias;
		ExistingNumCinematicMipLevels = ExistingTexture->NumCinematicMipLevels;
		ExistingNeverStream = ExistingTexture->NeverStream;
		ExistingSRGB = ExistingTexture->SRGB;
		ExistingPreserveBorder = ExistingTexture->bPreserveBorder;
		ExistingNoCompression = ExistingTexture->CompressionNone;
		ExistingNoAlpha = ExistingTexture->CompressionNoAlpha;
		ExistingDeferCompression = ExistingTexture->DeferCompression;
		ExistingFlipGreenChannel = ExistingTexture->bFlipGreenChannel;
		ExistingDitherMipMapAlpha = ExistingTexture->bDitherMipMapAlpha;
		ExistingAlphaCoverageThresholds = ExistingTexture->AlphaCoverageThresholds;
		ExistingAdjustBrightness = ExistingTexture->AdjustBrightness;
		ExistingAdjustBrightnessCurve = ExistingTexture->AdjustBrightnessCurve;
		ExistingAdjustVibrance = ExistingTexture->AdjustVibrance;
		ExistingAdjustSaturation = ExistingTexture->AdjustSaturation;
		ExistingAdjustRGBCurve = ExistingTexture->AdjustRGBCurve;
		ExistingAdjustHue = ExistingTexture->AdjustHue;
		ExistingAdjustMinAlpha = ExistingTexture->AdjustMinAlpha;
		ExistingAdjustMaxAlpha = ExistingTexture->AdjustMaxAlpha;
		ExistingMipGenSettings = ExistingTexture->MipGenSettings;
	}

	if (ExistingTexture2D)
	{
		// Update with new settings, which should disable streaming...
		ExistingTexture2D->UpdateResource();
	}

	FTextureReferenceReplacer RefReplacer(ExistingTexture);

	UTexture* Texture = ImportTexture(InClass, InParent, InName, Flags, Type, Buffer, BufferEnd, Warn);

	if (!Texture)
	{
		if (ExistingTexture)
		{
			// We failed to import over the existing texture. Make sure the resource is ready in the existing texture.
			ExistingTexture->UpdateResource();
		}

		Warn->Logf(ELogVerbosity::Error, TEXT("Texture import failed"));
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	//Replace the reference for the new texture with the existing one so that all current users still have valid references.
	RefReplacer.Replace(Texture);

	// Start with the value that the loader suggests.
	CompressionSettings = Texture->CompressionSettings;

	// Figure out whether we're using a normal map LOD group.
	bool bIsNormalMapLODGroup = false;
	if (LODGroup == TEXTUREGROUP_WorldNormalMap
		|| LODGroup == TEXTUREGROUP_CharacterNormalMap
		|| LODGroup == TEXTUREGROUP_VehicleNormalMap
		|| LODGroup == TEXTUREGROUP_WeaponNormalMap)
	{
		// Change from default to normal map.
		if (CompressionSettings == TC_Default)
		{
			CompressionSettings = TC_Normalmap;
		}
		bIsNormalMapLODGroup = true;
	}

	// Propagate options.
	Texture->CompressionSettings = CompressionSettings;

	// Packed normal map
	if (Texture->IsNormalMap())
	{
		Texture->SRGB = 0;
		if (!bIsNormalMapLODGroup)
		{
			LODGroup = TEXTUREGROUP_WorldNormalMap;
		}
	}

	if (!FCString::Stricmp(Type, TEXT("ies")))
	{
		LODGroup = TEXTUREGROUP_IESLightProfile;
	}

	Texture->LODGroup = LODGroup;

	// Revert the LODGroup to the default if it was forcibly set by the texture being a normal map.
	// This handles the case where multiple textures are being imported consecutively and
	// LODGroup unexpectedly changes because some textures were normal maps and others weren't.
	if (LODGroup == TEXTUREGROUP_WorldNormalMap && !bIsNormalMapLODGroup)
	{
		LODGroup = TEXTUREGROUP_World;
	}

	Texture->CompressionNone = NoCompression;
	Texture->CompressionNoAlpha = NoAlpha;
	Texture->DeferCompression = bDeferCompression;
	Texture->bDitherMipMapAlpha = bDitherMipMapAlpha;
	Texture->AlphaCoverageThresholds = AlphaCoverageThresholds;

	if (Texture->MipGenSettings == TMGS_FromTextureGroup)
	{
		// unless the loader suggest a different setting
		Texture->MipGenSettings = MipGenSettings;
	}

	Texture->bPreserveBorder = bPreserveBorder;

	Texture->AssetImportData->Update(CurrentFilename, FileHash.IsValid() ? &FileHash : nullptr);

	UTexture2D* Texture2D = Cast<UTexture2D>(Texture);

	// Restore user set options
	if (ExistingTexture && bUsingExistingSettings)
	{
		if (Texture2D)
		{
			Texture2D->AddressX = ExistingAddressX;
			Texture2D->AddressY = ExistingAddressY;
		}

		Texture->Filter = ExistingFilter;
		Texture->LODGroup = ExistingLODGroup;
		Texture->CompressionSettings = ExistingCompressionSettings;
		Texture->LODBias = ExistingLODBias;
		Texture->NumCinematicMipLevels = ExistingNumCinematicMipLevels;
		Texture->NeverStream = ExistingNeverStream;
		Texture->SRGB = ExistingSRGB;
		Texture->bPreserveBorder = ExistingPreserveBorder;
		Texture->CompressionNone = ExistingNoCompression;
		Texture->CompressionNoAlpha = ExistingNoAlpha;
		Texture->DeferCompression = ExistingDeferCompression;
		Texture->bDitherMipMapAlpha = ExistingDitherMipMapAlpha;
		Texture->AlphaCoverageThresholds = ExistingAlphaCoverageThresholds;
		Texture->bFlipGreenChannel = ExistingFlipGreenChannel;
		Texture->AdjustBrightness = ExistingAdjustBrightness;
		Texture->AdjustBrightnessCurve = ExistingAdjustBrightnessCurve;
		Texture->AdjustVibrance = ExistingAdjustVibrance;
		Texture->AdjustSaturation = ExistingAdjustSaturation;
		Texture->AdjustRGBCurve = ExistingAdjustRGBCurve;
		Texture->AdjustHue = ExistingAdjustHue;
		Texture->AdjustMinAlpha = ExistingAdjustMinAlpha;
		Texture->AdjustMaxAlpha = ExistingAdjustMaxAlpha;
		Texture->MipGenSettings = ExistingMipGenSettings;
	}
	else
	{
		Texture->bFlipGreenChannel = (bFlipNormalMapGreenChannel && Texture->IsNormalMap());
		// save user option
		GConfig->SetBool(TEXT("/Script/UnrealEd.EditorEngine"), TEXT("FlipNormalMapGreenChannel"), bFlipNormalMapGreenChannel, GEngineIni);
	}

	if (Texture2D)
	{
		// The texture has been imported and has no editor specific changes applied so we clear the painted flag.
		Texture2D->bHasBeenPaintedInEditor = false;
	}

	// Automatically detect if the texture is a normal map and configure its properties accordingly
	// NormalMapIdentification::HandleAssetPostImport(this, Texture); // TODO

	if (IsAutomatedImport())
	{
		// Apply Auto import settings 
		// Should be applied before post edit change
		// ApplyAutoImportSettings(Texture); // TODO
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Texture);

	// Invalidate any materials using the newly imported texture. (occurs if you import over an existing texture)
	Texture->PostEditChange();

	// Invalidate any volume texture that was built on this texture.
	if (Texture2D)
	{
		for (TObjectIterator<UVolumeTexture> It; It; ++It)
		{
			UVolumeTexture* VolumeTexture = *It;
			if (VolumeTexture && VolumeTexture->Source2DTexture == Texture2D)
			{
				VolumeTexture->UpdateSourceFromSourceTexture();
				VolumeTexture->UpdateResource();
			}
		}
	}

	if (bCreateMaterial)
	{
		Warn->Logf(ELogVerbosity::Warning, TEXT("Creating auto material from VTF not supported"));
	}

	return Texture;
}

/** Returns whether or not the given class is supported by this factory. */
bool UVTFFactory::DoesSupportClass(UClass* Class)
{
	return Class == UTexture2D::StaticClass() || Class == UTextureCube::StaticClass();
}

/** Returns true if this factory can deal with the file sent in. */
bool UVTFFactory::FactoryCanImport(const FString& Filename)
{
	FString Extension = FPaths::GetExtension(Filename);
	return Extension.Compare("vtf", ESearchCase::IgnoreCase) == 0;
}

// End UFactory Interface

void UVTFFactory::ApplyAutoImportSettings(UTexture* Texture)
{
	if (AutomatedImportSettings.IsValid())
	{
		FJsonObjectConverter::JsonObjectToUStruct(AutomatedImportSettings.ToSharedRef(), Texture->GetClass(), Texture, 0, CPF_InstancedReference);
	}
}

UTexture* UVTFFactory::ImportTexture(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	bool bAllowNonPowerOfTwo = false;
	GConfig->GetBool(TEXT("TextureImporter"), TEXT("AllowNonPowerOfTwoTextures"), bAllowNonPowerOfTwo, GEditorIni);

	// Validate it.
	const int32 Length = BufferEnd - Buffer;

	//if (!IsImportResolutionValid(PngImageWrapper->GetWidth(), PngImageWrapper->GetHeight(), bAllowNonPowerOfTwo, Warn))
	//{
	//	return nullptr;
	//}

	//// Select the texture's source format
	//ETextureSourceFormat TextureFormat = TSF_Invalid;
	//int32 BitDepth = PngImageWrapper->GetBitDepth();
	//ERGBFormat Format = PngImageWrapper->GetFormat();

	//if (Format == ERGBFormat::Gray)
	//{
	//	if (BitDepth <= 8)
	//	{
	//		TextureFormat = TSF_G8;
	//		Format = ERGBFormat::Gray;
	//		BitDepth = 8;
	//	}
	//	else if (BitDepth == 16)
	//	{
	//		// TODO: TSF_G16?
	//		TextureFormat = TSF_RGBA16;
	//		Format = ERGBFormat::RGBA;
	//		BitDepth = 16;
	//	}
	//}
	//else if (Format == ERGBFormat::RGBA || Format == ERGBFormat::BGRA)
	//{
	//	if (BitDepth <= 8)
	//	{
	//		TextureFormat = TSF_BGRA8;
	//		Format = ERGBFormat::BGRA;
	//		BitDepth = 8;
	//	}
	//	else if (BitDepth == 16)
	//	{
	//		TextureFormat = TSF_RGBA16;
	//		Format = ERGBFormat::RGBA;
	//		BitDepth = 16;
	//	}
	//}

	//if (TextureFormat == TSF_Invalid)
	//{
	//	Warn->Logf(ELogVerbosity::Error, TEXT("PNG file contains data in an unsupported format."));
	//	return nullptr;
	//}

	//UTexture2D* Texture = CreateTexture2D(InParent, Name, Flags);
	//if (Texture)
	//{
	//	Texture->Source.Init(
	//		PngImageWrapper->GetWidth(),
	//		PngImageWrapper->GetHeight(),
	//		/*NumSlices=*/ 1,
	//		/*NumMips=*/ 1,
	//		TextureFormat
	//	);
	//	Texture->SRGB = BitDepth < 16;
	//	const TArray<uint8>* RawPNG = nullptr;
	//	if (PngImageWrapper->GetRaw(Format, BitDepth, RawPNG))
	//	{
	//		uint8* MipData = Texture->Source.LockMip(0);
	//		FMemory::Memcpy(MipData, RawPNG->GetData(), RawPNG->Num());

	//		bool bFillPNGZeroAlpha = true;
	//		GConfig->GetBool(TEXT("TextureImporter"), TEXT("FillPNGZeroAlpha"), bFillPNGZeroAlpha, GEditorIni);

	//		if (bFillPNGZeroAlpha)
	//		{
	//			// Replace the pixels with 0.0 alpha with a color value from the nearest neighboring color which has a non-zero alpha
	//			FillZeroAlphaPNGData(Texture->Source, MipData);
	//		}
	//	}
	//	else
	//	{
	//		Warn->Logf(ELogVerbosity::Error, TEXT("Failed to decode PNG."));
	//		Texture->Source.UnlockMip(0);
	//		Texture->MarkPendingKill();
	//		return nullptr;
	//	}
	//	Texture->Source.UnlockMip(0);
	//}

	//return Texture;

	return nullptr;
}