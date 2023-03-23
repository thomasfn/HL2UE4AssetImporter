#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2SkyboxConverter, Log, All);

#pragma region FSkyboxTextureData

enum class ESkyboxTextureDataFormat : uint8
{
	// RGBA, 8 bits per channel
	RGBA8,
	// RGBA, 8 bits per channel, compressed HDR
	RGBA8_CompressedHDR,
	// RGBA, 8 bits per channel
	BGRA8,
	// RGBA, 8 bits per channel, compressed HDR
	BGRA8_CompressedHDR,
	// RGBA, 16 bits per channel
	RGBA16,
	// RGBA, 16 bits per channel, floating point
	RGBA16F
};

enum class ESkyboxTextureDataWrapMode : uint8
{
	Wrap,
	Clamp
};

enum class ESkyboxTextureDataFilterMode : uint8
{
	Nearest,
	Linear
};

struct FSkyboxTextureData
{
private:

	int width, height;
	ESkyboxTextureDataFormat format;
	TArray<uint8> rawData;

public:

	inline FSkyboxTextureData(int width, int height, ESkyboxTextureDataFormat format, const uint8* src = nullptr);
	inline FSkyboxTextureData(UTexture2D* texture, bool compressedHDR);
	inline FSkyboxTextureData();

	inline int GetTexelSizeBits() const;
	inline int GetTexelSizeBytes() const;

	inline int GetScanlineSizeBits() const;
	inline int GetScanlineSizeBytes() const;

	inline int GetWidth() const;
	inline int GetHeight() const;

	inline ESkyboxTextureDataFormat GetFormat() const;

	inline TArrayView<uint8> GetRawData();
	inline TArrayView<const uint8> GetRawData() const;

	inline TArrayView<uint8> TexelAt(int x, int y);
	inline TArrayView<const uint8> TexelAt(int x, int y) const;

	inline TArrayView<uint8> ScanlineAt(int y);
	inline TArrayView<const uint8> ScanlineAt(int y) const;

	inline void CopyTexel(int x, int y, FSkyboxTextureData& dst, int dstX, int dstY) const;

	FLinearColor ReadTexel(int x, int y) const;
	void WriteTexel(int x, int y, const FLinearColor& texel);

	FLinearColor Sample(
		float u, float v,
		ESkyboxTextureDataWrapMode wrapU = ESkyboxTextureDataWrapMode::Clamp, ESkyboxTextureDataWrapMode wrapV = ESkyboxTextureDataWrapMode::Clamp,
		ESkyboxTextureDataFilterMode filterMode = ESkyboxTextureDataFilterMode::Linear
	) const;

	FSkyboxTextureData Convert(ESkyboxTextureDataFormat newFormat) const;

	FSkyboxTextureData Rotate(int quarterTurns) const;

private:

	static uint64 HDRDecompress(uint32 pixel);

};

inline FSkyboxTextureData::FSkyboxTextureData(int width, int height, ESkyboxTextureDataFormat format, const uint8* src)
	: width(width), height(height), format(format)
{
	const int texelSize = GetTexelSizeBytes();
	if (src != nullptr)
	{
		rawData.AddUninitialized(width * height * texelSize);
		FMemory::Memcpy(rawData.GetData(), src, width * height * texelSize);
	}
	else
	{
		rawData.AddZeroed(width * height * texelSize);
	}
}

inline FSkyboxTextureData::FSkyboxTextureData(UTexture2D* texture, bool compressedHDR)
{
	width = texture->Source.GetSizeX();
	height = texture->Source.GetSizeY();
	switch (texture->Source.GetFormat())
	{
//	case TSF_RGBA8: format = compressedHDR ? ESkyboxTextureDataFormat::RGBA8_CompressedHDR : ESkyboxTextureDataFormat::RGBA8; break;
	case TSF_BGRA8: format = compressedHDR ? ESkyboxTextureDataFormat::BGRA8_CompressedHDR : ESkyboxTextureDataFormat::BGRA8; break;
	case TSF_RGBA16: format = ESkyboxTextureDataFormat::RGBA16; break;
	case TSF_RGBA16F: format = ESkyboxTextureDataFormat::RGBA16F; break;
	default:
		checkf(false, TEXT("Unsupported texture format %i"), texture->Source.GetFormat());
		break;
	}
	const int texelSize = GetTexelSizeBytes();
	uint8* mipData = texture->Source.LockMip(0);
	rawData.AddUninitialized(width * height * texelSize);
	FMemory::Memcpy(rawData.GetData(), mipData, width * height * texelSize);
	texture->Source.UnlockMip(0);
	check(rawData.Num() == texture->Source.CalcMipSize(0));
}

inline FSkyboxTextureData::FSkyboxTextureData()
	: FSkyboxTextureData(0, 0, ESkyboxTextureDataFormat::RGBA8, nullptr)
{ }

inline int FSkyboxTextureData::GetTexelSizeBits() const
{
	switch (format)
	{
	case ESkyboxTextureDataFormat::RGBA8:
	case ESkyboxTextureDataFormat::RGBA8_CompressedHDR:
	case ESkyboxTextureDataFormat::BGRA8:
	case ESkyboxTextureDataFormat::BGRA8_CompressedHDR:
		return 32;
	case ESkyboxTextureDataFormat::RGBA16:
	case ESkyboxTextureDataFormat::RGBA16F:
		return 64;
	default:
		checkf(false, TEXT("Unknown texture format %i"), format);
		return 0;
	}
}
inline int FSkyboxTextureData::GetTexelSizeBytes() const { return GetTexelSizeBits() >> 3; }

inline int FSkyboxTextureData::GetScanlineSizeBits() const { return GetTexelSizeBits() * width; }
inline int FSkyboxTextureData::GetScanlineSizeBytes() const { return GetTexelSizeBytes() * width; }

inline int FSkyboxTextureData::GetWidth() const { return width; }
inline int FSkyboxTextureData::GetHeight() const { return height; }

inline ESkyboxTextureDataFormat FSkyboxTextureData::GetFormat() const { return format; }

inline TArrayView<uint8> FSkyboxTextureData::GetRawData() { return rawData; }

inline TArrayView<const uint8> FSkyboxTextureData::GetRawData() const { return rawData; }

inline TArrayView<uint8> FSkyboxTextureData::TexelAt(int x, int y)
{
	check(x >= 0 && x < width);
	check(y >= 0 && y < height);
	const int texelSize = GetTexelSizeBytes();
	return TArrayView<uint8>(rawData).Slice(y * GetScanlineSizeBytes() + x * texelSize, texelSize);
}

inline TArrayView<const uint8> FSkyboxTextureData::TexelAt(int x, int y) const
{
	check(x >= 0 && x < width);
	check(y >= 0 && y < height);
	const int texelSize = GetTexelSizeBytes();
	return TArrayView<const uint8>(rawData).Slice(y * GetScanlineSizeBytes() + x * texelSize, texelSize);
}

inline TArrayView<uint8> FSkyboxTextureData::ScanlineAt(int y)
{
	check(y >= 0 && y < height);
	const int scanlineSize = GetScanlineSizeBytes();
	return TArrayView<uint8>(rawData).Slice(scanlineSize * y, scanlineSize);
}

inline TArrayView<const uint8> FSkyboxTextureData::ScanlineAt(int y) const
{
	check(y >= 0 && y < height);
	const int scanlineSize = GetScanlineSizeBytes();
	return TArrayView<const uint8>(rawData).Slice(scanlineSize * y, scanlineSize);
}

inline void FSkyboxTextureData::CopyTexel(int x, int y, FSkyboxTextureData& dst, int dstX, int dstY) const
{
	TArrayView<uint8> dstTexel = dst.TexelAt(dstX, dstY);
	TArrayView<const uint8> srcTexel = TexelAt(x, y);
	check(dstTexel.Num() == srcTexel.Num());
	for (int i = 0; i < srcTexel.Num(); ++i)
	{
		dstTexel[i] = srcTexel[i];
	}
}

#pragma endregion

class FSkyboxConverter
{
public:

	static bool IsSkyboxTexture(const FString& textureName, FString& outSkyboxName);

	static void ConvertSkybox(UTextureCube* texture, const FString& skyboxName);

private:

	static UTexture2D* GetFace(const FString& skyboxName, const FString& face);

};