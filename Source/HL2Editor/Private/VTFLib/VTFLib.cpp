/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VTFLib.h"
#include "VTFFile.h"
#include "VMTFile.h"

using namespace VTFLib;

namespace VTFLib
{
	bool bInitialized = false;
	Diagnostics::CError LastError;

	CVTFFile *Image = 0;
	CImageArray *ImageFVector3f = 0;

	CVMTFile *Material = 0;
	CMaterialArray *MaterialFVector3f = 0;

	unsigned int uiDXTQuality = DXT_QUALITY_HIGH;

	float sLuminanceWeightR = 0.299f;
	float sLuminanceWeightG = 0.587f;
	float sLuminanceWeightB = 0.114f;

	unsigned short uiBlueScreenMaskR = 0x0000;
	unsigned short uiBlueScreenMaskG = 0x0000;
	unsigned short uiBlueScreenMaskB = 0xffff;

	unsigned short uiBlueScreenClearR = 0x0000;
	unsigned short uiBlueScreenClearG = 0x0000;
	unsigned short uiBlueScreenClearB = 0x0000;

	float sFP16HDRKey = 4.0f;
	float sFP16HDRShift = 0.0f;
	float sFP16HDRGamma = 2.25f;

	float sUnsharpenRadius = 2.0f;
	float sUnsharpenAmount = 0.5f;
	float sUnsharpenThreshold = 0.0f;

	float sXSharpenStrength = 255.0f;
	float sXSharpenThreshold = 255.0f;

	unsigned int uiVMTParseMode = PARSE_MODE_LOOSE;
}

//
// vlGetVersion()
// Gets the library's version number.
//
unsigned int vlGetVersion()
{
	return VL_VERSION;
}

//
// vlGetVersionString()
// Gets the library's version number string.
//
const char *vlGetVersionString()
{
	return VL_VERSION_STRING;
}

//
// vlGetLastError()
// Gets the last error of a failed function.
//
const char *vlGetLastError()
{
	return LastError.Get();
}

//
// vlInitialize()
// Initializes all resources.
//
bool vlInitialize()
{
	if(bInitialized)
	{
		LastError.Set("VTFLib already initialized.");
		return false;
	}

	bInitialized = true;

	ImageFVector3f = new CImageArray();
	MaterialFVector3f = new CMaterialArray();

	return true;
}

//
// vlShutdown()
// Frees all resources.
//
void vlShutdown()
{
	if(!bInitialized)
		return;

	int i;

	bInitialized = false;

	Image = 0;
	Material = 0;

	for(i = 0; i < ImageFVector3f->Num(); i++)
	{
		delete (*ImageFVector3f)[i];
	}

	delete ImageFVector3f;
	ImageFVector3f = 0;

	for(i = 0; i < MaterialFVector3f->Num(); i++)
	{
		delete (*MaterialFVector3f)[i];
	}

	delete MaterialFVector3f;
	MaterialFVector3f = 0;
}

bool vlGetBoolean(VTFLibOption Option)
{
	return false;
}

void vlSetBoolean(VTFLibOption Option, bool bValue)
{

}

int vlGetInteger(VTFLibOption Option)
{
	switch(Option)
	{
	case VTFLIB_DXT_QUALITY:
		return (int)uiDXTQuality;

	case VTFLIB_BLUESCREEN_MASK_R:
		return (int)uiBlueScreenMaskR;
	case VTFLIB_BLUESCREEN_MASK_G:
		return (int)uiBlueScreenMaskG;
	case VTFLIB_BLUESCREEN_MASK_B:
		return (int)uiBlueScreenMaskB;

	case VTFLIB_BLUESCREEN_CLEAR_R:
		return (int)uiBlueScreenClearR;
	case VTFLIB_BLUESCREEN_CLEAR_G:
		return (int)uiBlueScreenClearG;
	case VTFLIB_BLUESCREEN_CLEAR_B:
		return (int)uiBlueScreenClearB;

	case VTFLIB_VMT_PARSE_MODE:
		return (int)uiVMTParseMode;
	}

	return 0;
}

void vlSetInteger(VTFLibOption Option, int iValue)
{
	switch(Option)
	{
	case VTFLIB_DXT_QUALITY:
		if(iValue < 0 || iValue >= DXT_QUALITY_COUNT)
			return;
		uiDXTQuality = (unsigned int)iValue;
		break;

	case VTFLIB_BLUESCREEN_MASK_R:
		if(iValue < 0)
			iValue = 0;
		else if(iValue > 65535)
			iValue = 65535;
		uiBlueScreenMaskR = (unsigned short)iValue;
		break;
	case VTFLIB_BLUESCREEN_MASK_G:
		if(iValue < 0)
			iValue = 0;
		else if(iValue > 65535)
			iValue = 65535;
		uiBlueScreenMaskG = (unsigned short)iValue;
		break;
	case VTFLIB_BLUESCREEN_MASK_B:
		if(iValue < 0)
			iValue = 0;
		else if(iValue > 65535)
			iValue = 65535;
		uiBlueScreenMaskB = (unsigned short)iValue;
		break;

	case VTFLIB_BLUESCREEN_CLEAR_R:
		if(iValue < 0)
			iValue = 0;
		else if(iValue > 65535)
			iValue = 65535;
		uiBlueScreenClearR = (unsigned short)iValue;
		break;
	case VTFLIB_BLUESCREEN_CLEAR_G:
		if(iValue < 0)
			iValue = 0;
		else if(iValue > 65535)
			iValue = 65535;
		uiBlueScreenClearG = (unsigned short)iValue;
		break;
	case VTFLIB_BLUESCREEN_CLEAR_B:
		if(iValue < 0)
			iValue = 0;
		else if(iValue > 65535)
			iValue = 65535;
		uiBlueScreenClearB = (unsigned short)iValue;
		break;

	case VTFLIB_VMT_PARSE_MODE:
		if(iValue < 0 || iValue >= PARSE_MODE_COUNT)
			return;
		uiVMTParseMode = (unsigned int)iValue;
		break;
	}
}

float vlGetFloat(VTFLibOption Option)
{
	switch(Option)
	{
	case VTFLIB_LUMINANCE_WEIGHT_R:
		return sLuminanceWeightR;
	case VTFLIB_LUMINANCE_WEIGHT_G:
		return sLuminanceWeightG;
	case VTFLIB_LUMINANCE_WEIGHT_B:
		return sLuminanceWeightB;

	case VTFLIB_FP16_HDR_KEY:
		return sFP16HDRKey;

	case VTFLIB_UNSHARPEN_RADIUS:
		return sUnsharpenRadius;
	case VTFLIB_UNSHARPEN_AMOUNT:
		return sUnsharpenAmount;
	case VTFLIB_UNSHARPEN_THRESHOLD:
		return sUnsharpenThreshold;

	case VTFLIB_XSHARPEN_STRENGTH:
		return sXSharpenStrength;
	case VTFLIB_XSHARPEN_THRESHOLD:
		return sXSharpenThreshold;
	}

	return 0.0f;
}

void vlSetFloat(VTFLibOption Option, float sValue)
{
	switch(Option)
	{
	case VTFLIB_LUMINANCE_WEIGHT_R:
		if(sValue < 0.0f)
			sValue = 0.0f;
		sLuminanceWeightR = sValue;
		break;
	case VTFLIB_LUMINANCE_WEIGHT_G:
		if(sValue < 0.0f)
			sValue = 0.0f;
		sLuminanceWeightG = sValue;
		break;
	case VTFLIB_LUMINANCE_WEIGHT_B:
		if(sValue < 0.0f)
			sValue = 0.0f;
		sLuminanceWeightB = sValue;
		break;

	case VTFLIB_FP16_HDR_KEY:
		sFP16HDRKey = sValue;
		break;
	case VTFLIB_FP16_HDR_SHIFT:
		sFP16HDRShift = sValue;
		break;
	case VTFLIB_FP16_HDR_GAMMA:
		sFP16HDRGamma = sValue;
		break;

	case VTFLIB_UNSHARPEN_RADIUS:
		if(sValue <= 0.0f)
			sValue = 2.0f;
		sUnsharpenRadius = sValue;
		break;
	case VTFLIB_UNSHARPEN_AMOUNT:
		if(sValue <= 0.0f)
			sValue = 0.5f;
		sUnsharpenAmount = sValue;
		break;
	case VTFLIB_UNSHARPEN_THRESHOLD:
		if(sValue < 0.0f)
			sValue = 0.0f;
		sUnsharpenThreshold = sValue;
		break;

	case VTFLIB_XSHARPEN_STRENGTH:
		if(sValue < 0.0f)
			sValue = 0.0f;
		if(sValue > 255.0f)
			sValue = 255.0f;
		sXSharpenStrength = sValue;
		break;
	case VTFLIB_XSHARPEN_THRESHOLD:
		if(sValue < 0.0f)
			sValue = 0.0f;
		if(sValue > 255.0f)
			sValue = 255.0f;
		sXSharpenThreshold = sValue;
		break;
	}
}