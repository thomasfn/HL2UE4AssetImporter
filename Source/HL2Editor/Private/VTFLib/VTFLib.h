/*
 * VTFLib
 * Copyright (C) 2005-2011 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

// ============================================================
// NOTE: This file is commented for compatibility with Doxygen.
// ============================================================
/*!
	\file VTFLib.h
	\brief VTFLib main header.
*/

/*!
	\mainpage VTFLib - Copyright &copy; 2005-2007 Neil Jedrzejewski & Ryan Gregg

	\section intro Introduction

	\subsection intro_sec What is VTFLib?
	VTFLib is an open source programming library which allows developers to
	add support for VMT and VTF files used by the material system of Valve
	Software's Source game engine. It functions independent of Steam allowing
	third party apps to use these file types without the need for Steam to be running.

	VTFLib offers a simply API which allows you to read or write VMT and VTF files
	through a few simple functions. It also takes care of a lot of formatting and
	validation of the files.

	\subsection whatcando_sec What can VTFLib do?
	VTFLib comprises of two modules, VMTFile and VTFFile.

	VMTFile allows you to read and write VMT files using a hierarchical node
	based system similar to how one might navigate an XML file. The system uses
	a series of VMTNode objects which have a name and data type. These can be set,
	read or parented as needed by your application.

	You can load an existing VMT file, navigate and modify it or create a new file
	from scratch. VMTFile will handle indentation and formatting of the file output.

	The VTFFile module allows you to read and write binary image data in VTF files.
	Internally, the module uses RGBA8888 (32-bit) data for images and requires either
	your own functions or a 3rd party library to read and write more common image formats.
	We use and have tested it with DevIL.

	It supports single frame, multiple frame and cube map files and can read and write
	compressed and uncompressed formats and allows access to individual frames, faces
	and MIP levels. It contains functions to automatically create MIP levels for you using
	a number of different filter types, can automatically generate sphere maps and even create
	Normal and DuDv maps from RGB and greyscale source images.

	\section started Getting Started with VTFLib

	\subsection settingup_sec Setting up your project for VTFLib
	If you have used third party libraries in your projects before, you probably know
	how to do the following, but if not here is a brief explanation of how to use VTFLib
	with your project. The following example is for Visual Studio C++ .NET 2003 but setting
	up for Visual C++ 6.0 is pretty much the same. We are assuming you have downloaded the
	binary distribution or have downloaded and compiled the source distribution of VTFLib
	to your liking.

	Within your projects setting, make sure under Linker Options, that you include the
	VTFLib.lib file and include the path to it in the library search paths. Alternatively
	you can use a pragma directive. Next, add VTFLib.h in your source files where you will
	be wanting to access VTFLib functions.

	All being well you should be up and ready to use VTFLib. Don't forget to make sure you
	place VTFLib.dll in a location where your application can find it.

	\section workingwith Working with textures using VTFFile

	\subsection vtfformat The VTF file format
	The VTF file format is not particularly complicated. Specific details can be found within
	the VTFLib source code however for convenience here is a short overview.

	VTF files are essentially bitmap files containing the data which makes up an image. The format
	is not dissimilar to Microsoft's DDS format, just organised a little different and with a
	different header.

	The files header is 64 bytes and contains a magic-number to identify the file as being VTF
	format, information on the width and height of the image and flags to indicate certain properties.
	It also contains information on the format of the bitmap data for the main image, low-res
	version, MIP levels, reflectivity and bump scale.

	Following the header is the data which makes up the low-res version of the image. This low-res
	image is used to get colour values when you hit a surface covered in the image with a weapon.

	After the low-res data comes the main data compressed in the chosen format. The data is stored
	smallest MIP level first interpolated with data for any additional faces (in the case of cube
	maps) or frames for animated textures.
*/


#ifndef VTFLIB_H
#define VTFLIB_H


#include "Error.h"
#include "VTFFile.h"
#include "VMTFile.h"

#include "Containers/Array.h"

namespace VTFLib
{
	typedef TArray<VTFLib::CVTFFile *> CImageArray;
	typedef TArray<VTFLib::CVMTFile *> CMaterialArray;

	extern bool bInitialized;
	extern Diagnostics::CError LastError;

	extern CVTFFile *Image;
	extern CImageArray *ImageFVector;

	extern CVMTFile *Material;
	extern CMaterialArray *MaterialFVector;

	extern unsigned int uiDXTQuality;

	extern float sLuminanceWeightR;
	extern float sLuminanceWeightG;
	extern float sLuminanceWeightB;

	extern unsigned short uiBlueScreenMaskR;
	extern unsigned short uiBlueScreenMaskG;
	extern unsigned short uiBlueScreenMaskB;

	extern unsigned short uiBlueScreenClearR;
	extern unsigned short uiBlueScreenClearG;
	extern unsigned short uiBlueScreenClearB;

	extern float sFP16HDRKey;
	extern float sFP16HDRShift;
	extern float sFP16HDRGamma;

	extern float sUnsharpenRadius;
	extern float sUnsharpenAmount;
	extern float sUnsharpenThreshold;

	extern float sXSharpenStrength;
	extern float sXSharpenThreshold;

	extern unsigned int uiVMTParseMode;
}

#define VL_VERSION			132			//!< VTFLib version as integer
#define VL_VERSION_STRING	"1.3.2"		//!< VTFLib version as string

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tagVTFLibOption
{
	VTFLIB_DXT_QUALITY,

	VTFLIB_LUMINANCE_WEIGHT_R,
	VTFLIB_LUMINANCE_WEIGHT_G,
	VTFLIB_LUMINANCE_WEIGHT_B,

	VTFLIB_BLUESCREEN_MASK_R,
	VTFLIB_BLUESCREEN_MASK_G,
	VTFLIB_BLUESCREEN_MASK_B,

	VTFLIB_BLUESCREEN_CLEAR_R,
	VTFLIB_BLUESCREEN_CLEAR_G,
	VTFLIB_BLUESCREEN_CLEAR_B,

	VTFLIB_FP16_HDR_KEY,
	VTFLIB_FP16_HDR_SHIFT,
	VTFLIB_FP16_HDR_GAMMA,

	VTFLIB_UNSHARPEN_RADIUS,
	VTFLIB_UNSHARPEN_AMOUNT,
	VTFLIB_UNSHARPEN_THRESHOLD,

	VTFLIB_XSHARPEN_STRENGTH,
	VTFLIB_XSHARPEN_THRESHOLD,

	VTFLIB_VMT_PARSE_MODE
} VTFLibOption;

//! Return the VTFLib version as an integer.
unsigned int vlGetVersion();

//! Return the VTFLib version as a string.
const char *vlGetVersionString();

//! Return the last error message as a string.
const char *vlGetLastError();

//! Initialisation function
bool vlInitialize();

//! Shutdown function
void vlShutdown();

//! Return the specified option.
bool vlGetBoolean(VTFLibOption Option);
//! Set the specified option.
void vlSetBoolean(VTFLibOption Option, bool bValue);

//! Return the specified option.
int vlGetInteger(VTFLibOption Option);
//! Set the specified option.
void vlSetInteger(VTFLibOption Option, int iValue);

//! Return the specified option.
float vlGetFloat(VTFLibOption Option);
//! Set the specified option.
void vlSetFloat(VTFLibOption Option, float sValue);

#ifdef __cplusplus
}
#endif

#endif