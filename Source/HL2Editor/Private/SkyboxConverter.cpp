#include "SkyboxConverter.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "VMTMaterial.h"

DEFINE_LOG_CATEGORY(LogHL2SkyboxConverter);

#pragma region FSkyboxTextureData

FLinearColor FSkyboxTextureData::ReadTexel(int x, int y) const
{
	TArrayView<const uint8> rawTexel = TexelAt(x, y);
	switch (format)
	{
	case ESkyboxTextureDataFormat::RGBA8:
	{
		check(rawTexel.Num() == 4);
		return FLinearColor(rawTexel[0] / 255.0f, rawTexel[1] / 255.0f, rawTexel[2] / 255.0f, rawTexel[3] / 255.0f);
	}
	case ESkyboxTextureDataFormat::BGRA8:
	{
		check(rawTexel.Num() == 4);
		return FLinearColor(rawTexel[2] / 255.0f, rawTexel[1] / 255.0f, rawTexel[0] / 255.0f, rawTexel[3] / 255.0f);
	}
	case ESkyboxTextureDataFormat::RGBA8_CompressedHDR:
	{
		check(rawTexel.Num() == 4);
		const uint32* compressedTexel = reinterpret_cast<const uint32*>(rawTexel.GetData());
		const uint64 decompressed = HDRDecompress(*compressedTexel);
		const uint16* hdrTexel = reinterpret_cast<const uint16*>(&decompressed);
		return FLinearColor(hdrTexel[0] / 65535.0f, hdrTexel[1] / 65535.0f, hdrTexel[2] / 65535.0f, hdrTexel[3] / 65535.0f);
	}
	case ESkyboxTextureDataFormat::BGRA8_CompressedHDR:
	{
		check(rawTexel.Num() == 4);
		const uint32* compressedTexel = reinterpret_cast<const uint32*>(rawTexel.GetData());
		// TODO: Transpose the bytes before passing to HDRDecompress?
		const uint64 decompressed = HDRDecompress(*compressedTexel);
		const uint16* hdrTexel = reinterpret_cast<const uint16*>(&decompressed);
		return FLinearColor(hdrTexel[0] / 65535.0f, hdrTexel[1] / 65535.0f, hdrTexel[2] / 65535.0f, hdrTexel[3] / 65535.0f);
	}
	case ESkyboxTextureDataFormat::RGBA16:
	{
		check(rawTexel.Num() == 8);
		const uint16* hdrTexel = reinterpret_cast<const uint16*>(rawTexel.GetData());
		return FLinearColor(hdrTexel[0] / 65535.0f, hdrTexel[1] / 65535.0f, hdrTexel[2] / 65535.0f, hdrTexel[3] / 65535.0f);
	}
	case ESkyboxTextureDataFormat::RGBA16F:
	{
		check(rawTexel.Num() == 8);
		const FFloat16Color* f16color = reinterpret_cast<const FFloat16Color*>(rawTexel.GetData());
		return FLinearColor(f16color->R, f16color->G, f16color->B, f16color->A);
	}
	default:
		checkf(false, TEXT("Unsupported texture format %i"), format);
		return FLinearColor();
	}
}

void FSkyboxTextureData::WriteTexel(int x, int y, const FLinearColor& texel)
{
	TArrayView<uint8> rawTexel = TexelAt(x, y);
	switch (format)
	{
	case ESkyboxTextureDataFormat::RGBA8:
	{
		check(rawTexel.Num() == 4);
		rawTexel[0] = (uint8)FMath::Clamp(texel.R * 255.0f, 0.0f, 255.0f);
		rawTexel[1] = (uint8)FMath::Clamp(texel.G * 255.0f, 0.0f, 255.0f);
		rawTexel[2] = (uint8)FMath::Clamp(texel.B * 255.0f, 0.0f, 255.0f);
		rawTexel[3] = (uint8)FMath::Clamp(texel.A * 255.0f, 0.0f, 255.0f);
		break;
	}
	case ESkyboxTextureDataFormat::BGRA8:
	{
		check(rawTexel.Num() == 4);
		rawTexel[2] = (uint8)FMath::Clamp(texel.R * 255.0f, 0.0f, 255.0f);
		rawTexel[1] = (uint8)FMath::Clamp(texel.G * 255.0f, 0.0f, 255.0f);
		rawTexel[0] = (uint8)FMath::Clamp(texel.B * 255.0f, 0.0f, 255.0f);
		rawTexel[3] = (uint8)FMath::Clamp(texel.A * 255.0f, 0.0f, 255.0f);
		break;
	}
	case ESkyboxTextureDataFormat::RGBA8_CompressedHDR:
	case ESkyboxTextureDataFormat::BGRA8_CompressedHDR:
	{
		uint32* compressedTexel = reinterpret_cast<uint32*>(rawTexel.GetData());
		// TODO: Add compression to HDR support (do we really need it though?)
		*compressedTexel = 0;
		break;
	}
	case ESkyboxTextureDataFormat::RGBA16:
	{
		check(rawTexel.Num() == 8);
		uint16* hdrTexel = reinterpret_cast<uint16*>(rawTexel.GetData());
		hdrTexel[0] = (uint16)FMath::Clamp(texel.R * 65535.0f, 0.0f, 65535.0f);
		hdrTexel[1] = (uint16)FMath::Clamp(texel.G * 65535.0f, 0.0f, 65535.0f);
		hdrTexel[2] = (uint16)FMath::Clamp(texel.B * 65535.0f, 0.0f, 65535.0f);
		hdrTexel[3] = (uint16)FMath::Clamp(texel.A * 65535.0f, 0.0f, 65535.0f);
		break;
	}
	case ESkyboxTextureDataFormat::RGBA16F:
	{
		check(rawTexel.Num() == 8);
		FFloat16Color* f16color = reinterpret_cast<FFloat16Color*>(rawTexel.GetData());
		f16color->R = texel.R;
		f16color->G = texel.G;
		f16color->B = texel.B;
		f16color->A = texel.A;
		break;
	}
	default:
		checkf(false, TEXT("Unsupported texture format %i"), format);
		break;
	}
}

inline int NormaliseCoord(int x, ESkyboxTextureDataWrapMode wrapMode, int width)
{
	if (wrapMode == ESkyboxTextureDataWrapMode::Clamp)
	{
		return x < 0 ? 0 : x >= width ? width - 1 : x;
	}
	else
	{
		return x < 0 ? (x % width) + width : x % width;
	}
}

FLinearColor FSkyboxTextureData::Sample(float u, float v, ESkyboxTextureDataWrapMode wrapU, ESkyboxTextureDataWrapMode wrapV, ESkyboxTextureDataFilterMode filterMode) const
{
	const float x = u * (width - 1);
	const float y = v * (width - 1);
	const FLinearColor c00 = ReadTexel(NormaliseCoord((int)u, wrapU, width), NormaliseCoord((int)v, wrapV, height));
	const FLinearColor c01 = ReadTexel(NormaliseCoord((int)u, wrapU, width), NormaliseCoord((int)v + 1, wrapV, height));
	const FLinearColor c10 = ReadTexel(NormaliseCoord((int)u + 1, wrapU, width), NormaliseCoord((int)v, wrapV, height));
	const FLinearColor c11 = ReadTexel(NormaliseCoord((int)u + 1, wrapU, width), NormaliseCoord((int)v + 1, wrapV, height));
	const FLinearColor c00_c01 = FMath::Lerp(c00, c01, FMath::Frac(v));
	const FLinearColor c10_c11 = FMath::Lerp(c10, c11, FMath::Frac(v));
	return FMath::Lerp(c00_c01, c10_c11, FMath::Frac(u));
}

FSkyboxTextureData FSkyboxTextureData::Convert(ESkyboxTextureDataFormat newFormat) const
{
	if (newFormat == format) { return *this; }
	FSkyboxTextureData result(width, height, newFormat);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			result.WriteTexel(x, y, ReadTexel(x, y));
		}
	}
	return result;
}

FSkyboxTextureData FSkyboxTextureData::Rotate(int quarterTurns) const
{
	quarterTurns = quarterTurns < 0 ? (quarterTurns % 4) + 4 : quarterTurns % 4;
	if (quarterTurns == 0) { return *this; }
	FSkyboxTextureData result(width, height, format);
	switch (quarterTurns)
	{
	case 1:
		// 90 degrees CW
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				CopyTexel(y, width - (x + 1), result, x, y);
			}
		}
		break;
	case 2:
		// 180 degrees CW
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				CopyTexel(width - (x + 1), height - (y + 1), result, x, y);
			}
		}
		break;
	case 3:
		// 90 degrees CCW
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				CopyTexel(height - (y + 1), x, result, x, y);
			}
		}
		break;
	}
	return result;
}

uint64 FSkyboxTextureData::HDRDecompress(uint32 pixel)
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

#pragma endregion

#define LOCTEXT_NAMESPACE "HL2Importer"

const TCHAR* skyboxPostfixes[] =
{
	TEXT("ft"),
	TEXT("bk"),
	TEXT("lf"),
	TEXT("rt"),
	TEXT("up"),
	TEXT("dn")
};

bool FSkyboxConverter::IsSkyboxTexture(const FString& textureName, FString& outSkyboxName)
{
	for (int i = 0; i < 6; ++i)
	{
		if (textureName.EndsWith(skyboxPostfixes[i]))
		{
			outSkyboxName = textureName;
			outSkyboxName.RemoveFromEnd(skyboxPostfixes[i]);
			while (outSkyboxName.RemoveFromEnd(TEXT("_"))) { }
			return true;
		}
	}
	return false;
}

void FSkyboxConverter::ConvertSkybox(UTextureCube* texture, const FString& skyboxName)
{
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();

	// Fetch all 6 faces
	const FString& hl2TextureBasePath = IHL2Runtime::Get().GetHL2TextureBasePath();
	UTexture2D* textures[6] =
	{
		GetFace(skyboxName, TEXT("ft")), // +X, -90
		GetFace(skyboxName, TEXT("bk")), // -X, +90
		GetFace(skyboxName, TEXT("lf")), // +Y, +18
		GetFace(skyboxName, TEXT("rt")), // -Y, +0
		GetFace(skyboxName, TEXT("up")), // +Z, +0
		GetFace(skyboxName, TEXT("dn"))	 // -Z, +0
	};

	// Determine size
	const bool hdr = skyboxName.EndsWith(TEXT("hdr"));
	int width = 0, height = 0;
	for (int i = 0; i < 6; ++i)
	{
		width = FMath::Max(width, textures[i]->Source.GetSizeX());
		height = FMath::Max(height, textures[i]->Source.GetSizeY());
	}

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
	const int mipSize = texture->Source.CalcMipSize(0) / 6;
	uint8* dstData = texture->Source.LockMip(0);
	FScopedSlowTask progress(6, LOCTEXT("SkyboxConverterConverting", "Converting faces..."));
	progress.MakeDialog();
	for (int i = 0; i < 6; ++i)
	{
		// Pull texture data
		progress.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("SkyboxConverterConverting", "Face {0} of {1}"), i + 1, 6));
		const FSkyboxTextureData srcFace(textures[i], hdr);
		FSkyboxTextureData const* workingFace = &srcFace;

		// If it's shorter than expected...
		FSkyboxTextureData resizedFace;
		if (srcFace.GetHeight() < height)
		{
			// If it's narrower than expected...
			if (srcFace.GetWidth() < width)
			{
				// e.g. the bottom face of a 1024x1024 skybox might be a small 4x4, since you don't need high detail down there
				// Upsample it, also reformat to RGBA/BGRA whilst we're here, since we're doing the decoding work anyway
				resizedFace = FSkyboxTextureData(width, height, hdr ? ESkyboxTextureDataFormat::RGBA16 : ESkyboxTextureDataFormat::BGRA8);
				for (int y = 0; y < height; ++y)
				{
					const float v = y / (height - 1.0f);
					for (int x = 0; x < width; ++x)
					{
						const float u = x / (width - 1.0f);
						resizedFace.WriteTexel(x, y, workingFace->Sample(u, v));
					}
				}
				workingFace = &resizedFace;
			}
			else
			{
				check(srcFace.GetWidth() == width);
				// e.g. the side face of a 1024x1024 skybox might be a cropped 1024x512, since you don't need detail below the horizon
				// Extrude the bottom row down
				resizedFace = FSkyboxTextureData(width, height, workingFace->GetFormat());
				for (int y = 0; y < height; ++y)
				{
					const int srcScanlineIndex = y >= workingFace->GetHeight() ? workingFace->GetHeight() - 1 : y;
					TArrayView<const uint8> srcScanline = workingFace->ScanlineAt(srcScanlineIndex);
					TArrayView<uint8> dstScanline = resizedFace.ScanlineAt(y);
					FMemory::Memcpy(dstScanline.GetData(), srcScanline.GetData(), srcScanline.Num());
				}
				workingFace = &resizedFace;
			}
		}
		check(workingFace->GetWidth() == width && workingFace->GetHeight() == height);
		
		// Apply rotation
		FSkyboxTextureData rotatedFace;
		switch (i)
		{
		case 0:
			// Rotate 90 degrees CCW
			rotatedFace = workingFace->Rotate(-1);
			workingFace = &rotatedFace;
			break;
		case 1:
			// Rotate 90 degrees CW
			rotatedFace = workingFace->Rotate(1);
			workingFace = &rotatedFace;
			break;
		case 2:
		case 5:
			// Rotate 180 degrees
			rotatedFace = workingFace->Rotate(2);
			workingFace = &rotatedFace;
			break;
		}

		// Convert to expected format
		FSkyboxTextureData convertedFace;
		if (hdr && workingFace->GetFormat() != ESkyboxTextureDataFormat::RGBA16)
		{
			convertedFace = workingFace->Convert(ESkyboxTextureDataFormat::RGBA16);
			workingFace = &convertedFace;
		}
		if (!hdr && workingFace->GetFormat() != ESkyboxTextureDataFormat::BGRA8)
		{
			convertedFace = workingFace->Convert(ESkyboxTextureDataFormat::BGRA8);
			workingFace = &convertedFace;
		}

		// Copy to dst mip
		FMemory::Memcpy(dstData + mipSize * i, workingFace->GetRawData().GetData(), mipSize);
	}
	texture->Source.UnlockMip(0);
}

UTexture2D* FSkyboxConverter::GetFace(const FString& skyboxName, const FString& face)
{
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	const FString& hl2TextureBasePath = IHL2Runtime::Get().GetHL2TextureBasePath();
	const FString& hl2MaterialBasePath = IHL2Runtime::Get().GetHL2MaterialBasePath();

	// search for hl2/materials/skybox/{skyboxname}bk.{skyboxname}bk
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(FName(*(hl2MaterialBasePath / TEXT("skybox") / skyboxName + face + TEXT(".") + skyboxName + face)));
	if (!assetData.IsValid())
	{
		// search for hl2/textures/skybox/{skyboxname}_bk.{skyboxname}_bk
		assetData = assetRegistry.GetAssetByObjectPath(FName(*(hl2MaterialBasePath / TEXT("skybox") / skyboxName + TEXT("_") + face + TEXT(".") + skyboxName + TEXT("_") + face)));
	}
	if (assetData.IsValid())
	{
		UVMTMaterial* material = CastChecked<UVMTMaterial>(assetData.GetAsset());
		static const FName fnBaseTexture(TEXT("basetexture"));
		if (material->vmtTextures.Contains(fnBaseTexture))
		{
			const FName texturePath = material->vmtTextures[fnBaseTexture];
			assetData = assetRegistry.GetAssetByObjectPath(texturePath);
			if (assetData.IsValid())
			{
				return CastChecked<UTexture2D>(assetData.GetAsset());
			}
			else
			{
				UE_LOG(LogHL2SkyboxConverter, Warning, TEXT("Found a vmt material for skybox face %s (%s) but couldn't resolve the $basetexture within"), *face, *skyboxName);
			}
		}
	}

	// search for hl2/textures/skybox/{skyboxname}bk.{skyboxname}bk
	assetData = assetRegistry.GetAssetByObjectPath(FName(*(hl2TextureBasePath / TEXT("skybox") / skyboxName + face + TEXT(".") + skyboxName + face)));
	if (!assetData.IsValid())
	{
		// search for hl2/textures/skybox/{skyboxname}_bk.{skyboxname}_bk
		assetData = assetRegistry.GetAssetByObjectPath(FName(*(hl2TextureBasePath / TEXT("skybox") / skyboxName + TEXT("_") + face + TEXT(".") + skyboxName + TEXT("_") + face)));
	}
	return CastChecked<UTexture2D>(assetData.GetAsset());
}

#undef LOCTEXT_NAMESPACE