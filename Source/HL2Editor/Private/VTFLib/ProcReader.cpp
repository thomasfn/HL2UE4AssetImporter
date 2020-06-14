/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "ProcReader.h"
#include "Proc.h"
#include "VTFLib.h"

using namespace VTFLib;
using namespace VTFLib::IO::Readers;

CProcReader::CProcReader(void *pUserData)
{
	this->bOpened = false;
	this->pUserData = pUserData;
}

CProcReader::~CProcReader()
{
	this->Close();
}

bool CProcReader::Opened() const
{
	return this->bOpened;
}

bool CProcReader::Open()
{
	this->Close();

	if(pReadOpenProc == 0)
	{
		LastError.Set("pReadOpenProc not set.");
		return false;
	}

	if(this->bOpened)
	{
		LastError.Set("Reader already open.");
		return false;
	}

	if(!pReadOpenProc(this->pUserData))
	{
		LastError.Set("Error opening file.");
		return false;
	}

	this->bOpened = true;

	return true;
}

void CProcReader::Close()
{
	if(pReadCloseProc == 0)
	{
		return;
	}

	if(this->bOpened)
	{
		pReadCloseProc(this->pUserData);
		this->bOpened = false;
	}
}

unsigned int CProcReader::GetStreamSize() const
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(pReadSizeProc == 0)
	{
		LastError.Set("pReadSizeProc not set.");
		return 0xffffffff;
	}

	return pReadSizeProc(this->pUserData);
}

unsigned int CProcReader::GetStreamPointer() const
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(pReadTellProc == 0)
	{
		LastError.Set("pReadTellProc not set.");
		return 0;
	}

	return pReadTellProc(this->pUserData);
}

unsigned int CProcReader::Seek(long lOffset, unsigned int uiMode)
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(pReadSeekProc == 0)
	{
		LastError.Set("pReadSeekProc not set.");
		return 0;
	}

	return pReadSeekProc(lOffset, (VLSeekMode)uiMode, this->pUserData);
}

bool CProcReader::Read(char &cChar)
{
	if(!this->bOpened)
	{
		return false;
	}

	if(pReadReadProc == 0)
	{
		LastError.Set("pReadReadProc not set.");
		return false;
	}

	unsigned int uiBytesRead = pReadReadProc(&cChar, 1, this->pUserData);

	if(uiBytesRead == 0)
	{
		LastError.Set("pReadReadProc() failed.");
	}

	return uiBytesRead == 1;
}

unsigned int CProcReader::Read(void *vData, unsigned int uiBytes)
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(pReadReadProc == 0)
	{
		LastError.Set("pReadReadProc not set.");
		return 0;
	}

	unsigned int uiBytesRead = pReadReadProc(vData, uiBytes, this->pUserData);

	if(uiBytesRead == 0)
	{
		LastError.Set("pReadReadProc() failed.");
	}

	return uiBytesRead;
}