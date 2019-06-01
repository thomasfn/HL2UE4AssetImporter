/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef VTFWRAPPER_H
#define VTFWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

//
// Memory managment routines.
//

bool vlImageIsBound();
bool vlBindImage(unsigned int uiImage);

bool vlCreateImage(unsigned int *uiImage);
void vlDeleteImage(unsigned int uiImage);

//
// Library routines.  (Basically class wrappers.)
//

void vlImageCreateDefaultCreateStructure(SVTFCreateOptions *VTFCreateOptions);

bool vlImageCreate(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiFrames, unsigned int uiFaces, unsigned int uiSlices, VTFImageFormat ImageFormat, bool bThumbnail, bool bMipmaps, bool bNullImageData);
bool vlImageCreateSingle(unsigned int uiWidth, unsigned int uiHeight, unsigned char *lpImageDataRGBA8888, SVTFCreateOptions *VTFCreateOptions);
bool vlImageCreateMultiple(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiFrames, unsigned int uiFaces, unsigned int uiSlices, unsigned char **lpImageDataRGBA8888, SVTFCreateOptions *VTFCreateOptions);
void vlImageDestroy();

bool vlImageIsLoaded();

bool vlImageLoad(const char *cFileName, bool bHeaderOnly);
bool vlImageLoadLump(const void *lpData, unsigned int uiBufferSize, bool bHeaderOnly);
bool vlImageLoadProc(void *pUserData, bool bHeaderOnly);

bool vlImageSave(const char *cFileName);
bool vlImageSaveLump(void *lpData, unsigned int uiBufferSize, unsigned int *uiSize);
bool vlImageSaveProc(void *pUserData);

//
// Image routines.
//

unsigned int vlImageGetHasImage();

unsigned int vlImageGetMajorVersion();
unsigned int vlImageGetMinorVersion();
unsigned int vlImageGetSize();

unsigned int vlImageGetWidth();
unsigned int vlImageGetHeight();
unsigned int vlImageGetDepth();

unsigned int vlImageGetFrameCount();
unsigned int vlImageGetFaceCount();
unsigned int vlImageGetMipmapCount();

unsigned int vlImageGetStartFrame();
void vlImageSetStartFrame(unsigned int uiStartFrame);

unsigned int vlImageGetFlags();
void vlImageSetFlags(unsigned int uiFlags);

bool vlImageGetFlag(VTFImageFlag ImageFlag);
void vlImageSetFlag(VTFImageFlag ImageFlag, bool bState);

float vlImageGetBumpmapScale();
void vlImageSetBumpmapScale(float sBumpmapScale);

void vlImageGetReflectivity(float *sX, float *sY, float *sZ);
void vlImageSetReflectivity(float sX, float sY, float sZ);

VTFImageFormat vlImageGetFormat();

unsigned char *vlImageGetData(unsigned int uiFrame, unsigned int uiFace, unsigned int uiSlice, unsigned int uiMipmapLevel);
void vlImageSetData(unsigned int uiFrame, unsigned int uiFace, unsigned int uiSlice, unsigned int uiMipmapLevel, unsigned char *lpData);

//
// Thumbnail routines.
//

bool vlImageGetHasThumbnail();

unsigned int vlImageGetThumbnailWidth();
unsigned int vlImageGetThumbnailHeight();

VTFImageFormat vlImageGetThumbnailFormat();

unsigned char *vlImageGetThumbnailData();
void vlImageSetThumbnailData(unsigned char *lpData);

//
// Resource routines.
//

bool vlImageGetSupportsResources();

unsigned int vlImageGetResourceCount();
unsigned int vlImageGetResourceType(unsigned int uiIndex);
bool vlImageGetHasResource(unsigned int uiType);

void *vlImageGetResourceData(unsigned int uiType, unsigned int *uiSize);
void *vlImageSetResourceData(unsigned int uiType, unsigned int uiSize, void *lpData);

//
// Helper routines.
//

bool vlImageGenerateMipmaps(unsigned int uiFace, unsigned int uiFrame, VTFMipmapFilter MipmapFilter, VTFSharpenFilter SharpenFilter);
bool vlImageGenerateAllMipmaps(VTFMipmapFilter MipmapFilter, VTFSharpenFilter SharpenFilter);

bool vlImageGenerateThumbnail();

bool vlImageGenerateNormalMap(unsigned int uiFrame, VTFKernelFilter KernelFilter, VTFHeightConversionMethod HeightConversionMethod, VTFNormalAlphaResult NormalAlphaResult);
bool vlImageGenerateAllNormalMaps(VTFKernelFilter KernelFilter, VTFHeightConversionMethod HeightConversionMethod, VTFNormalAlphaResult NormalAlphaResult);

bool vlImageGenerateSphereMap();

bool vlImageComputeReflectivity();

//
// Conversion routines.
//

SVTFImageFormatInfo const *vlImageGetImageFormatInfo(VTFImageFormat ImageFormat);
bool vlImageGetImageFormatInfoEx(VTFImageFormat ImageFormat, SVTFImageFormatInfo *VTFImageFormatInfo);

unsigned int vlImageComputeImageSize(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDepth, unsigned int uiMipmaps, VTFImageFormat ImageFormat);

unsigned int vlImageComputeMipmapCount(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDepth);
void vlImageComputeMipmapDimensions(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDepth, unsigned int uiMipmapLevel, unsigned int *uiMipmapWidth, unsigned int *uiMipmapHeight, unsigned int *uiMipmapDepth);
unsigned int vlImageComputeMipmapSize(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDepth, unsigned int uiMipmapLevel, VTFImageFormat ImageFormat);

bool vlImageConvertToRGBA8888(unsigned char *lpSource, unsigned char *lpDest, unsigned int uiWidth, unsigned int uiHeight, VTFImageFormat SourceFormat);
bool vlImageConvertFromRGBA8888(unsigned char *lpSource, unsigned char *lpDest, unsigned int uiWidth, unsigned int uiHeight, VTFImageFormat DestFormat);

bool vlImageConvert(unsigned char *lpSource, unsigned char *lpDest, unsigned int uiWidth, unsigned int uiHeight, VTFImageFormat SourceFormat, VTFImageFormat DestFormat);

bool vlImageConvertToNormalMap(unsigned char *lpSourceRGBA8888, unsigned char *lpDestRGBA8888, unsigned int uiWidth, unsigned int uiHeight, VTFKernelFilter KernelFilter, VTFHeightConversionMethod HeightConversionMethod, VTFNormalAlphaResult NormalAlphaResult, unsigned char bMinimumZ, float sScale, bool bWrap, bool bInvertX, bool bInvertY);

bool vlImageResize(unsigned char *lpSourceRGBA8888, unsigned char *lpDestRGBA8888, unsigned int uiSourceWidth, unsigned int uiSourceHeight, unsigned int uiDestWidth, unsigned int uiDestHeight, VTFMipmapFilter ResizeFilter, VTFSharpenFilter SharpenFilter);

void vlImageCorrectImageGamma(unsigned char *lpImageDataRGBA8888, unsigned int uiWidth, unsigned int uiHeight, float sGammaCorrection);
void vlImageComputeImageReflectivity(unsigned char *lpImageDataRGBA8888, unsigned int uiWidth, unsigned int uiHeight, float *sX, float *sY, float *sZ);

void vlImageFlipImage(unsigned char *lpImageDataRGBA8888, unsigned int uiWidth, unsigned int uiHeight);
void vlImageMirrorImage(unsigned char *lpImageDataRGBA8888, unsigned int uiWidth, unsigned int uiHeight);

#ifdef __cplusplus
}
#endif

#endif