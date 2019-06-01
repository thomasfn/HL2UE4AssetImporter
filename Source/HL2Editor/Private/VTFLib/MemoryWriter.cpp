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
#include "MemoryWriter.h"

using namespace VTFLib;
using namespace VTFLib::IO::Writers;

CMemoryWriter::CMemoryWriter(void *vData, unsigned int uiBufferSize)
{
	this->bOpened = false;

	this->vData = vData;
	this->uiBufferSize = uiBufferSize;

	this->uiPointer = 0;
	this->uiLength = 0;
}

CMemoryWriter::~CMemoryWriter()
{

}

bool CMemoryWriter::Opened() const
{
	return this->bOpened;
}

bool CMemoryWriter::Open()
{
	if(vData == 0)
	{
		LastError.Set("Memory stream is null.");
		return false;
	}

	this->uiPointer = 0;
	this->uiLength = 0;

	this->bOpened = true;

	return true;
}

void CMemoryWriter::Close()
{
	this->bOpened = false;
}

unsigned int CMemoryWriter::GetStreamSize() const
{
	/*if(!this->bOpened)
	{
		return 0;
	}*/

	return this->uiLength;
}

unsigned int CMemoryWriter::GetStreamPointer() const
{
	if(!this->bOpened)
	{
		return 0;
	}

	return this->uiPointer;
}

unsigned int CMemoryWriter::Seek(long lOffset, unsigned int uiMode)
{
	if(!this->bOpened)
	{
		return 0;
	}

	switch(uiMode)
	{
		case FILE_BEGIN:
			this->uiPointer = 0;
			break;
		case FILE_CURRENT:

			break;
		case FILE_END:
			this->uiPointer = this->uiLength;
			break;
	}

	long lPointer = (long)this->uiPointer + lOffset;

	if(lPointer < 0)
	{
		lPointer = 0;
	}

	if(lPointer > (long)this->uiLength)
	{
		lPointer = (long)this->uiLength;
	}

	this->uiPointer = (unsigned int)lPointer;

	return this->uiPointer;
}

bool CMemoryWriter::Write(char cChar)
{
	if(!this->bOpened)
	{
		return false;
	}

	if(this->uiPointer == this->uiBufferSize)
	{
		LastError.Set("End of memory stream.");

		return false;
	}
	else
	{
		*((char *)this->vData + this->uiPointer++) = cChar;

		this->uiLength++;

		return true;
	}
}

unsigned int CMemoryWriter::Write(void *data, unsigned int uiBytes)
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(this->uiPointer == this->uiBufferSize)
	{
		return 0;
	}
	else if(this->uiPointer + uiBytes > this->uiBufferSize)
	{
		uiBytes = this->uiBufferSize - this->uiPointer;

		FMemory::Memcpy((unsigned char *)vData + this->uiPointer, data, uiBytes);

		this->uiLength += uiBytes;

		this->uiPointer = this->uiBufferSize;

		LastError.Set("End of memory stream.");

		return uiBytes;
	}
	else
	{
		FMemory::Memcpy((unsigned char *)vData + this->uiPointer, data, uiBytes);

		this->uiLength += uiBytes;

		this->uiPointer += uiBytes;

		return uiBytes;
	}
}