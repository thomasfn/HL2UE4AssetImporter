#include "VTFFactory.h"

#include "Runtime/Core/Public/Misc/FeedbackContext.h"
#include "Runtime/Engine/Classes/EditorFramework/AssetImportData.h"
#include "Runtime/Engine/Classes/Engine/VolumeTexture.h"
#include "Runtime/Engine/Classes/Engine/TextureCube.h"
#include "Runtime/JsonUtilities/Public/JsonObjectConverter.h"

#include "VTFLib/VTFLib.h"

UVTFFactory::UVTFFactory()
{
	// We only want to create assets by importing files.
	// Set this to true if you want to be able to create new, empty Assets from
	// the editor.
	bCreateNew = false;

	// Our Asset will be imported from a binary file
	bText = false;

	// Allows us to actually use the "Import" button in the Editor for this Asset
	bEditorImport = true;

	// Tells the Editor which file types we can import
	Formats.Add(TEXT("vtf;Valve Texture Format Files"));

	// Tells the Editor which Asset type this UFactory can import
	SupportedClass = UTexture2D::StaticClass();

	++ImportPriority;
}

UVTFFactory::~UVTFFactory()
{
	
}

// Begin UFactory Interface

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

	if (IsAutomatedImport())
	{
		// Apply Auto import settings 
		// Should be applied before post edit change
		ApplyAutoImportSettings(Texture);
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

bool UVTFFactory::IsImportResolutionValid(int32 Width, int32 Height, bool bAllowNonPowerOfTwo, FFeedbackContext* Warn)
{
	// Calculate the maximum supported resolution utilizing the global max texture mip count
	// (Note, have to subtract 1 because 1x1 is a valid mip-size; this means a GMaxTextureMipCount of 4 means a max resolution of 8x8, not 2^4 = 16x16)
	const int32 MaximumSupportedResolution = 1 << (GMaxTextureMipCount - 1);

	bool bValid = true;

	// Check if the texture is above the supported resolution and prompt the user if they wish to continue if it is
	if (Width > MaximumSupportedResolution || Height > MaximumSupportedResolution)
	{
		if (EAppReturnType::Yes != FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(
			NSLOCTEXT("UnrealEd", "Warning_LargeTextureImport", "Attempting to import {0} x {1} texture, proceed?\nLargest supported texture size: {2} x {3}"),
			FText::AsNumber(Width), FText::AsNumber(Height), FText::AsNumber(MaximumSupportedResolution), FText::AsNumber(MaximumSupportedResolution))))
		{
			bValid = false;
		}
	}

	const bool bIsPowerOfTwo = FMath::IsPowerOfTwo(Width) && FMath::IsPowerOfTwo(Height);
	// Check if the texture dimensions are powers of two
	if (!bAllowNonPowerOfTwo && !bIsPowerOfTwo)
	{
		Warn->Log(ELogVerbosity::Error, *NSLOCTEXT("UnrealEd", "Warning_TextureNotAPowerOfTwo", "Cannot import texture with non-power of two dimensions").ToString());
		bValid = false;
	}

	return bValid;
}

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
	const uint32 Length = BufferEnd - Buffer;

	// Throw it at VTFLib
	VTFLib::CVTFFile vtfFile;
	if (!vtfFile.Load(Buffer, Length))
	{
		FString err = VTFLib::LastError.Get();
		Warn->Logf(ELogVerbosity::Error, TEXT("Failed to load VTF: %s"), *err);
		return nullptr;
	}

	if (!IsImportResolutionValid(vtfFile.GetWidth(), vtfFile.GetHeight(), bAllowNonPowerOfTwo, Warn))
	{
		return nullptr;
	}

	if (vtfFile.GetFrameCount() > 1)
	{
		Warn->Logf(ELogVerbosity::Warning, TEXT("Animated VTFs are not yet supported"));
	}

	if (vtfFile.GetFaceCount() == 1)
	{
		UTexture2D* texture = CreateTexture2D(InParent, Name, Flags);
		if (texture == nullptr)
		{
			return nullptr;
		}

		if (vtfFile.GetFlag(VTFImageFlag::TEXTUREFLAGS_NORMAL))
		{
			texture->CompressionSettings = TC_Normalmap;
			texture->SRGB = false;
			texture->LODGroup = TEXTUREGROUP_WorldNormalMap;
			texture->bFlipGreenChannel = bFlipNormalMapGreenChannel;
		}
		else
		{
			texture->SRGB = true;
		}

		bool hdr;
		switch (vtfFile.GetFormat())
		{
			case VTFImageFormat::IMAGE_FORMAT_RGBA16161616:
			case VTFImageFormat::IMAGE_FORMAT_RGBA16161616F:
			case VTFImageFormat::IMAGE_FORMAT_RGBA32323232F:
			case VTFImageFormat::IMAGE_FORMAT_RGB323232F:
				hdr = true;
				break;
			default:
				hdr = false;
				break;
		}

		texture->Source.Init(
			vtfFile.GetWidth(),
			vtfFile.GetHeight(),
			/*NumSlices=*/ 1,
			/*NumMips=*/ 1,
			hdr ? TSF_RGBA16 : TSF_BGRA8
		);

		uint8* mipData = texture->Source.LockMip(0);
		bool success = VTFLib::CVTFFile::Convert(vtfFile.GetData(0, 0, 0, 0), mipData, vtfFile.GetWidth(), vtfFile.GetHeight(), vtfFile.GetFormat(), hdr ? VTFImageFormat::IMAGE_FORMAT_RGBA16161616 : VTFImageFormat::IMAGE_FORMAT_BGRA8888);

		if (!success)
		{
			texture->Source.UnlockMip(0);
			FString err = VTFLib::LastError.Get();
			Warn->Logf(ELogVerbosity::Error, TEXT("Failed to convert VTF image data to RGBA8: %s"), *err);
			return nullptr;
		}

		// If it's a normal map, identify any alpha - we need to extract this into it's own texture as normal maps can't have alpha
		if (texture->CompressionSettings == TC_Normalmap)
		{
			bool alphaFound = false;
			for (uint32 y = 0; y < vtfFile.GetHeight(); ++y)
			{
				for (uint32 x = 0; x < vtfFile.GetWidth(); ++x)
				{
					uint32 pixel = ((uint32*)mipData)[y * vtfFile.GetWidth() + x];
					uint8 alpha = pixel >> 0x18;
					if (alpha != 255)
					{
						alphaFound = true;
						break;
					}
				}
				if (alphaFound)
				{
					break;
				}
			}
			if (alphaFound)
			{
				FString alphaTexPathName = InParent->GetPathName() + TEXT("_a");
				UPackage* alphaTexPackage = CreatePackage(nullptr, *alphaTexPathName);
				UTexture* alphaTex = ExtractAlpha(alphaTexPackage, FName(*FPaths::GetCleanFilename(alphaTexPathName)), RF_Public | RF_Standalone, vtfFile.GetWidth(), vtfFile.GetHeight(), mipData, Warn);
				if (alphaTex != nullptr)
				{
					alphaTex->PostEditChange();
					FAssetRegistryModule::AssetCreated(alphaTex);
					alphaTex->MarkPackageDirty();
				}
			}
		}

		texture->Source.UnlockMip(0);

		return texture;
	}
	else
	{
		int faceCount = (int)vtfFile.GetFaceCount();
		if (faceCount > 6)
		{
			// Some cubemaps have 7 faces, the 7th being a spheremap, we can ignore that one
			faceCount = 6;
		}
		else if (faceCount < 6)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("Cubemap VTF only has %d faces"), faceCount);
			return nullptr;
		}

		UTextureCube* texture = CreateTextureCube(InParent, Name, Flags);
		if (texture == nullptr)
		{
			return nullptr;
		}

		bool hdr;
		switch (vtfFile.GetFormat())
		{
		case VTFImageFormat::IMAGE_FORMAT_RGBA16161616:
		case VTFImageFormat::IMAGE_FORMAT_RGBA16161616F:
		case VTFImageFormat::IMAGE_FORMAT_RGBA32323232F:
		case VTFImageFormat::IMAGE_FORMAT_RGB323232F:
			hdr = true;
			break;
		default:
			hdr = false;
			break;
		}

		texture->Source.Init(
			vtfFile.GetWidth(),
			vtfFile.GetHeight(),
			/*NumSlices=*/ faceCount,
			/*NumMips=*/ 1,
			hdr ? TSF_RGBA16 : TSF_BGRA8
		);

		int mipSize = texture->Source.CalcMipSize(0) / faceCount;
		uint8* mipData = texture->Source.LockMip(0);
		for (int i = 0; i < faceCount; ++i)
		{
			bool success = VTFLib::CVTFFile::Convert(vtfFile.GetData(0, (uint32)faceCount, 0, 0), mipData + mipSize * i, vtfFile.GetWidth(), vtfFile.GetHeight(), vtfFile.GetFormat(), hdr ? VTFImageFormat::IMAGE_FORMAT_RGBA16161616 : VTFImageFormat::IMAGE_FORMAT_BGRA8888);
			if (!success)
			{
				FString err = VTFLib::LastError.Get();
				Warn->Logf(ELogVerbosity::Error, TEXT("Failed to convert VTF image data to RGBA8: %s"), *err);
				return nullptr;
			}
		}
		texture->Source.UnlockMip(0);

		return texture;
	}
}

UTexture* UVTFFactory::ExtractAlpha(UObject* inParent, FName name, EObjectFlags flags, int width, int height, const uint8* data, FFeedbackContext* warn)
{
	UTexture2D* texture = FindObject<UTexture2D>(inParent, *name.ToString());
	if (texture == nullptr)
	{
		texture = NewObject<UTexture2D>(inParent, name, flags);
	}
	else
	{
		texture->ReleaseResource();
	}

	texture->CompressionSettings = TC_Alpha;
	texture->SRGB = false;
	
	texture->Source.Init(
		width,
		height,
		/*NumSlices=*/ 1,
		/*NumMips=*/ 1,
		TSF_BGRA8
	);

	uint8* mipData = texture->Source.LockMip(0);
	FMemory::Memcpy(mipData, data, width * height * 4);
	texture->Source.UnlockMip(0);

	return texture;
}