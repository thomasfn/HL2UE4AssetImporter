/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "ProcWriter.h"
#include "Proc.h"
#include "VTFLib.h"


using namespace VTFLib;
using namespace VTFLib::IO::Writers;

CProcWriter::CProcWriter(void *pUserData)
{
	this->bOpened = false;
	this->pUserData = pUserData;
}

CProcWriter::~CProcWriter()
{
	this->Close();
}

bool CProcWriter::Opened() const
{
	return this->bOpened;
}

bool CProcWriter::Open()
{
	this->Close();

	if(pWriteOpenProc == 0)
	{
		LastError.Set("pWriteOpenProc not set.");
		return false;
	}

	if(this->bOpened)
	{
		LastError.Set("Writer already open.");
		return false;
	}

	if(!pWriteOpenProc(this->pUserData))
	{
		LastError.Set("Error opening file.");
		return false;
	}

	this->bOpened = true;

	return true;
}

void CProcWriter::Close()
{
	if(pWriteCloseProc == 0)
	{
		return;
	}

	if(this->bOpened)
	{
		pWriteCloseProc(this->pUserData);
		this->bOpened = false;
	}
}

unsigned int CProcWriter::GetStreamSize() const
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(pWriteSizeProc == 0)
	{
		LastError.Set("pWriteTellProc not set.");
		return 0xffffffff;
	}

	return pWriteSizeProc(this->pUserData);
}

unsigned int CProcWriter::GetStreamPointer() const
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(pWriteTellProc == 0)
	{
		LastError.Set("pWriteTellProc not set.");
		return 0;
	}

	return pWriteTellProc(this->pUserData);
}

unsigned int CProcWriter::Seek(long lOffset, unsigned int uiMode)
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(pWriteSeekProc == 0)
	{
		LastError.Set("pWriteSeekProc not set.");
		return 0;
	}

	return pWriteSeekProc(lOffset, (VLSeekMode)uiMode, this->pUserData);
}

bool CProcWriter::Write(char cChar)
{
	if(!this->bOpened)
	{
		return false;
	}

	if(pWriteWriteProc == 0)
	{
		LastError.Set("pWriteWriteProc not set.");
		return false;
	}

	unsigned int uiBytesWritten = pWriteWriteProc(&cChar, 1, this->pUserData);

	if(uiBytesWritten == 0)
	{
		LastError.Set("pWriteWriteProc() failed.");
	}

	return uiBytesWritten == 1;
}

unsigned int CProcWriter::Write(void *vData, unsigned int uiBytes)
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(pWriteWriteProc == 0)
	{
		LastError.Set("pWriteWriteProc not set.");
		return 0;
	}

	unsigned int uiBytesWritten = pWriteWriteProc(vData, uiBytes, this->pUserData);

	if(uiBytesWritten == 0)
	{
		LastError.Set("pWriteWriteProc() failed.");
	}

	return uiBytesWritten;
}