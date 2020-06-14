/*
 * VTFLib
 * Copyright (C) 2005-2011 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VTFWrapper.h"
#include "VTFLib.h"
#include "VTFFile.h"

using namespace VTFLib;

//
// vlImageBound()
// Returns true if an image is bound, false otherwise.
//
bool vlImageIsBound()
{
	if(!bInitialized)
	{
		LastError.Set("VTFLib not initialized.");
		return false;
	}

	return Image != 0;
}

//
// vlBindImage()
// Bind an image to operate on.
// All library routines will use this image.
//
bool vlBindImage(unsigned int uiImage)
{
	if(!bInitialized)
	{
		LastError.Set("VTFLib not initialized.");
		return false;
	}

	if(uiImage >= (unsigned int)ImageFVector->Num() || (*ImageFVector)[uiImage] == 0)
	{
		LastError.Set("Invalid image.");
		return false;
	}

	if(Image == (*ImageFVector)[uiImage])	// If it is already bound do nothing.
		return true;

	Image = (*ImageFVector)[uiImage];

	return true;
}

//
// vlCreateImage()
// Create an image to work on.
//
bool vlCreateImage(unsigned int *uiImage)
{
	if(!bInitialized)
	{
		LastError.Set("VTFLib not initialized.");
		return false;
	}

	ImageFVector->Add(new CVTFFile());
	*uiImage = (unsigned int)ImageFVector->Num() - 1;

	return true;
}

//
// vlDeleteImage()
// Delete an image and all resources associated with it.
//
void vlDeleteImage(unsigned int uiImage)
{
	if(!bInitialized)
		return;

	if(uiImage >= (unsigned int)ImageFVector->Num())
		return;

	if((*ImageFVector)[uiImage] == 0)
		return;

	if((*ImageFVector)[uiImage] == Image)
	{
		Image = 0;
	}

	delete (*ImageFVector)[uiImage];
	(*ImageFVector)[uiImage] = 0;
}

void vlImageCreateDefaultCreateStructure(SVTFCreateOptions *VTFCreateOptions)
{
	VTFCreateOptions->uiVersion[0] = VTF_MAJOR_VERSION;
	VTFCreateOptions->uiVersion[1] = VTF_MINOR_VERSION_DEFAULT;

	VTFCreateOptions->ImageFormat = IMAGE_FORMAT_RGBA8888;

	VTFCreateOptions->uiFlags = 0;
	VTFCreateOptions->uiStartFrame = 0;
	VTFCreateOptions->sBumpScale = 1.0f;
	VTFCreateOptions->sReflectivity[0] = 1.0f;
	VTFCreateOptions->sReflectivity[1] = 1.0f;
	VTFCreateOptions->sReflectivity[2] = 1.0f;

	VTFCreateOptions->bMipmaps = true;
	VTFCreateOptions->MipmapFilter = MIPMAP_FILTER_BOX;
	VTFCreateOptions->MipmapSharpenFilter = SHARPEN_FILTER_NONE;

	VTFCreateOptions->bResize = false;
	VTFCreateOptions->ResizeMethod = RESIZE_NEAREST_POWER2;
	VTFCreateOptions->ResizeFilter = MIPMAP_FILTER_TRIANGLE;
	VTFCreateOptions->ResizeSharpenFilter = SHARPEN_FILTER_NONE;
	VTFCreateOptions->uiResizeWidth = 0;
	VTFCreateOptions->uiResizeHeight = 0;

	VTFCreateOptions->bResizeClamp = true;
	VTFCreateOptions->uiResizeClampWidth = 4096;
	VTFCreateOptions->uiResizeClampHeight = 4096;

	VTFCreateOptions->bThumbnail = true;
	VTFCreateOptions->bReflectivity = true;

	VTFCreateOptions->bGammaCorrection = false;
	VTFCreateOptions->sGammaCorrection = 2.0f;

	VTFCreateOptions->bNormalMap = false;
	VTFCreateOptions->KernelFilter = KERNEL_FILTER_3X3;
	VTFCreateOptions->HeightConversionMethod = HEIGHT_CONVERSION_METHOD_AVERAGE_RGB;
	VTFCreateOptions->NormalAlphaResult = NORMAL_ALPHA_RESULT_WHITE;
	VTFCreateOptions->bNormalMinimumZ = 0;
	VTFCreateOptions->sNormalScale = 2.0f;
	VTFCreateOptions->bNormalWrap = false;
	VTFCreateOptions->bNormalInvertX = false;
	VTFCreateOptions->bNormalInvertY = false;
	VTFCreateOptions->bNormalInvertZ = false;

	VTFCreateOptions->bSphereMap = true;
}

bool vlImageCreate(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiFrames, unsigned int uiFaces, unsigned int uiSlices, VTFImageFormat ImageFormat, bool bThumbnail, bool bMipmaps, bool bNullImageData)
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->Create(uiWidth, uiHeight, uiFrames, uiFaces, uiSlices, ImageFormat, bThumbnail, bMipmaps, bNullImageData);
}

bool vlImageCreateSingle(unsigned int uiWidth, unsigned int uiHeight, unsigned char *lpImageDataRGBA8888, SVTFCreateOptions *VTFCreateOptions)
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->Create(uiWidth, uiHeight, lpImageDataRGBA8888, *VTFCreateOptions);
}

bool vlImageCreateMultiple(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiFrames, unsigned int uiFaces, unsigned int uiSlices, unsigned char **lpImageDataRGBA8888, SVTFCreateOptions *VTFCreateOptions)
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->Create(uiWidth, uiHeight, uiFrames, uiFaces, uiSlices, lpImageDataRGBA8888, *VTFCreateOptions);
}

void vlImageDestroy()
{
	if(Image == 0)
		return;

	Image->Destroy();
}

bool vlImageIsLoaded()
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->IsLoaded();
}

bool vlImageLoad(const char *cFileName, bool bHeaderOnly)
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->Load(cFileName, bHeaderOnly);
}

bool vlImageLoadLump(const void *lpData, unsigned int uiBufferSize, bool bHeaderOnly)
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->Load(lpData, uiBufferSize, bHeaderOnly);
}

bool vlImageLoadProc(void *pUserData, bool bHeaderOnly)
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->Load(pUserData, bHeaderOnly);
}

bool vlImageSaveLump(void *lpData, unsigned int uiBufferSize, unsigned int *uiSize)
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->Save(lpData, uiBufferSize, *uiSize);
}

bool vlImageSaveProc(void *pUserData)
{
	if(Image == 0)
	{
		LastError.Set("No image bound.");
		return false;
	}

	return Image->Save(pUserData);
}

unsigned int vlImageGetMajorVersion()
{
	if(Image == 0)
		return 0;

	return Image->GetMajorVersion();
}

unsigned int vlImageGetMinorVersion()
{
	if(Image == 0)
		return 0;

	return Image->GetMinorVersion();
}

unsigned int vlImageGetSize()
{
	if(Image == 0)
		return 0;

	return Image->GetSize();
}

unsigned int vlImageGetHasImage()
{
	if(Image == 0)
		return false;

	return Image->GetHasImage();
}

unsigned int vlImageGetWidth()
{
	if(Image == 0)
		return 0;

	return Image->GetWidth();
}

unsigned int vlImageGetHeight()
{
	if(Image == 0)
		return 0;

	return Image->GetHeight();
}

unsigned int vlImageGetDepth()
{
	if(Image == 0)
		return 0;

	return Image->GetDepth();
}

unsigned int vlImageGetFrameCount()
{
	if(Image == 0)
		return 0;

	return Image->GetFrameCount();
}

unsigned int vlImageGetFaceCount()
{
	if(Image == 0)
		return 0;

	return Image->GetFaceCount();
}

unsigned int vlImageGetMipmapCount()
{
	if(Image == 0)
		return 0;

	return Image->GetMipmapCount();
}

unsigned int vlImageGetStartFrame()
{
	if(Image == 0)
		return 0;

	return Image->GetStartFrame();
}

void vlImageSetStartFrame(unsigned int uiStartFrame)
{
	if(Image == 0)
		return;

	Image->SetStartFrame(uiStartFrame);
}

unsigned int vlImageGetFlags()
{
	if(Image == 0)
		return 0;

	return Image->GetFlags();
}

void vlImageSetFlags(unsigned int uiFlags)
{
	if(Image == 0)
		return;

	Image->SetFlags(uiFlags);
}

bool vlImageGetFlag(VTFImageFlag ImageFlag)
{
	if(Image == 0)
		return false;

	return Image->GetFlag(ImageFlag);
}

void vlImageSetFlag(VTFImageFlag ImageFlag, bool bState)
{
	if(Image == 0)
		return;

	Image->SetFlag(ImageFlag, bState);
}

float vlImageGetBumpmapScale()
{
	if(Image == 0)
		return 0.0f;

	return Image->GetBumpmapScale();
}

void vlImageSetBumpmapScale(float sBumpmapScale)
{
	if(Image == 0)
		return;

	Image->SetBumpmapScale(sBumpmapScale);
}

void vlImageGetReflectivity(float *sX, float *sY, float *sZ)
{
	if(Image == 0)
		return;

	Image->GetReflectivity(*sX, *sY, *sZ);
}

void vlImageSetReflectivity(float sX, float sY, float sZ)
{
	if(Image == 0)
		return;

	Image->SetReflectivity(sX, sY, sZ);
}

VTFImageFormat vlImageGetFormat()
{
	if(Image == 0)
		return IMAGE_FORMAT_NONE;

	return Image->GetFormat();
}

unsigned char *vlImageGetData(unsigned int uiFrame, unsigned int uiFace, unsigned int uiSlice, unsigned int uiMipmapLevel)
{
	if(Image == 0)
		return 0;

	return Image->GetData(uiFrame, uiFace, uiSlice, uiMipmapLevel);
}

void vlImageSetData(unsigned int uiFrame, unsigned int uiFace, unsigned int uiSlice, unsigned int uiMipmapLevel, unsigned char *lpData)
{
	if(Image == 0)
		return;

	Image->SetData(uiFrame, uiFace, uiSlice, uiMipmapLevel, lpData);
}

bool vlImageGetHasThumbnail()
{
	if(Image == 0)
		return false;

	return Image->GetHasThumbnail();
}

unsigned int vlImageGetThumbnailWidth()
{
	if(Image == 0)
		return 0;

	return Image->GetThumbnailWidth();
}

unsigned int vlImageGetThumbnailHeight()
{
	if(Image == 0)
		return 0;

	return Image->GetThumbnailHeight();
}

VTFImageFormat vlImageGetThumbnailFormat()
{
	if(Image == 0)
		return IMAGE_FORMAT_NONE;

	return Image->GetThumbnailFormat();
}

unsigned char *vlImageGetThumbnailData()
{
	if(Image == 0)
		return 0;

	return Image->GetThumbnailData();
}

void vlImageSetThumbnailData(unsigned char *lpData)
{
	if(Image == 0)
		return;

	Image->SetThumbnailData(lpData);
}

bool vlImageGetSupportsResources()
{
	if(Image == 0)
		return false;

	return Image->GetSupportsResources();
}

unsigned int vlImageGetResourceCount()
{
	if(Image == 0)
		return 0;

	return Image->GetResourceCount();
}

unsigned int vlImageGetResourceType(unsigned int uiIndex)
{
	if(Image == 0)
		return 0;

	return Image->GetResourceType(uiIndex);
}

bool vlImageGetHasResource(unsigned int uiType)
{
	if(Image == 0)
		return false;

	return Image->GetHasResource(uiType);
}

void *vlImageGetResourceData(unsigned int uiType, unsigned int *uiSize)
{
	if(Image == 0)
		return 0;

	return Image->GetResourceData(uiType, *uiSize);
}

void *vlImageSetResourceData(unsigned int uiType, unsigned int uiSize, void *lpData)
{
	if(Image == 0)
		return 0;

	return Image->SetResourceData(uiType, uiSize, lpData);
}

bool vlImageGenerateMipmaps(unsigned int uiFace, unsigned int uiFrame, VTFMipmapFilter MipmapFilter, VTFSharpenFilter SharpenFilter)
{
	if(Image == 0)
		return false;

	return Image->GenerateMipmaps(uiFace, uiFrame, MipmapFilter, SharpenFilter);
}

bool vlImageGenerateAllMipmaps(VTFMipmapFilter MipmapFilter, VTFSharpenFilter SharpenFilter)
{
	if(Image == 0)
		return false;

	return Image->GenerateMipmaps(MipmapFilter, SharpenFilter);
}

bool vlImageGenerateThumbnail()
{
	if(Image == 0)
		return false;

	return Image->GenerateThumbnail();
}

bool vlImageGenerateNormalMap(unsigned int uiFrame, VTFKernelFilter KernelFilter, VTFHeightConversionMethod HeightConversionMethod, VTFNormalAlphaResult NormalAlphaResult)
{
	if(Image == 0)
		return false;

	return Image->GenerateNormalMap(uiFrame, KernelFilter, HeightConversionMethod, NormalAlphaResult);
}

bool vlImageGenerateAllNormalMaps(VTFKernelFilter KernelFilter, VTFHeightConversionMethod HeightConversionMethod, VTFNormalAlphaResult NormalAlphaResult)
{
	if(Image == 0)
		return false;

	return Image->GenerateNormalMap(KernelFilter, HeightConversionMethod, NormalAlphaResult);
}

bool vlImageGenerateSphereMap()
{
	if(Image == 0)
		return false;

	return Image->GenerateSphereMap();
}

bool vlImageComputeReflectivity()
{
	if(Image == 0)
		return false;

	return Image->ComputeReflectivity();
}

SVTFImageFormatInfo const *vlImageGetImageFormatInfo(VTFImageFormat ImageFormat)
{
	return &CVTFFile::GetImageFormatInfo(ImageFormat);
}

bool vlImageGetImageFormatInfoEx(VTFImageFormat ImageFormat, SVTFImageFormatInfo *vtfImageFormatInfo)
{
	if(ImageFormat >= 0 && ImageFormat < IMAGE_FORMAT_COUNT)
	{
		FMemory::FMemory::Memcpy(vtfImageFormatInfo, &CVTFFile::GetImageFormatInfo(ImageFormat), sizeof(SVTFImageFormatInfo));
		return true;
	}

	return false;
}

unsigned int vlImageComputeImageSize(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDepth, unsigned int uiMipmaps, VTFImageFormat ImageFormat)
{
	return CVTFFile::ComputeImageSize(uiWidth, uiHeight, uiDepth, uiMipmaps, ImageFormat);
}

unsigned int vlImageComputeMipmapCount(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDepth)
{
	return CVTFFile::ComputeMipmapCount(uiWidth, uiHeight, uiDepth);
}

void vlImageComputeMipmapDimensions(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDepth, unsigned int uiMipmapLevel, unsigned int *uiMipmapWidth, unsigned int *uiMipmapHeight, unsigned int *uiMipmapDepth)
{
	CVTFFile::ComputeMipmapDimensions(uiWidth, uiHeight, uiDepth, uiMipmapLevel, *uiMipmapWidth, *uiMipmapHeight, *uiMipmapDepth);
}

unsigned int vlImageComputeMipmapSize(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDepth, unsigned int uiMipmapLevel, VTFImageFormat ImageFormat)
{
	return CVTFFile::ComputeMipmapSize(uiWidth, uiHeight, uiDepth, uiMipmapLevel, ImageFormat);
}

bool vlImageConvertToRGBA8888(unsigned char *lpSource, unsigned char *lpDest, unsigned int uiWidth, unsigned int uiHeight, VTFImageFormat SourceFormat)
{
	return CVTFFile::ConvertToRGBA8888(lpSource, lpDest, uiWidth, uiHeight, SourceFormat);
}

bool vlImageConvertFromRGBA8888(unsigned char *lpSource, unsigned char *lpDest, unsigned int uiWidth, unsigned int uiHeight, VTFImageFormat DestFormat)
{
	return CVTFFile::ConvertFromRGBA8888(lpSource, lpDest, uiWidth, uiHeight, DestFormat);
}

bool vlImageConvert(unsigned char *lpSource, unsigned char *lpDest, unsigned int uiWidth, unsigned int uiHeight, VTFImageFormat SourceFormat, VTFImageFormat DestFormat)
{
	return CVTFFile::Convert(lpSource, lpDest, uiWidth, uiHeight, SourceFormat, DestFormat);
}

bool vlImageConvertToNormalMap(unsigned char *lpSourceRGBA8888, unsigned char *lpDestRGBA8888, unsigned int uiWidth, unsigned int uiHeight, VTFKernelFilter KernelFilter, VTFHeightConversionMethod HeightConversionMethod, VTFNormalAlphaResult NormalAlphaResult, unsigned char bMinimumZ, float sScale, bool bWrap, bool bInvertX, bool bInvertY)
{
	return CVTFFile::ConvertToNormalMap(lpSourceRGBA8888, lpDestRGBA8888, uiWidth, uiHeight, KernelFilter, HeightConversionMethod, NormalAlphaResult, bMinimumZ, sScale, bWrap, bInvertX, bInvertY);
}

bool vlImageResize(unsigned char *lpSourceRGBA8888, unsigned char *lpDestRGBA8888, unsigned int uiSourceWidth, unsigned int uiSourceHeight, unsigned int uiDestWidth, unsigned int uiDestHeight, VTFMipmapFilter ResizeFilter, VTFSharpenFilter SharpenFilter)
{
	return CVTFFile::Resize(lpSourceRGBA8888, lpDestRGBA8888, uiSourceWidth, uiSourceHeight, uiDestWidth, uiDestHeight, ResizeFilter, SharpenFilter);
}

void vlImageCorrectImageGamma(unsigned char *lpImageDataRGBA8888, unsigned int uiWidth, unsigned int uiHeight, float sGammaCorrection)
{
	CVTFFile::CorrectImageGamma(lpImageDataRGBA8888, uiWidth, uiHeight, sGammaCorrection);
}

void vlImageComputeImageReflectivity(unsigned char *lpImageDataRGBA8888, unsigned int uiWidth, unsigned int uiHeight, float *sX, float *sY, float *sZ)
{
	CVTFFile::ComputeImageReflectivity(lpImageDataRGBA8888, uiWidth, uiHeight, *sX, *sY, *sZ);
}

void vlImageFlipImage(unsigned char *lpImageDataRGBA8888, unsigned int uiWidth, unsigned int uiHeight)
{
	CVTFFile::FlipImage(lpImageDataRGBA8888, uiWidth, uiHeight);
}

void vlImageMirrorImage(unsigned char *lpImageDataRGBA8888, unsigned int uiWidth, unsigned int uiHeight)
{
	CVTFFile::FlipImage(lpImageDataRGBA8888, uiWidth, uiHeight);
}