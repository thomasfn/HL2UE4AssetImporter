/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "MemoryReader.h"
#include "VTFLib.h"

using namespace VTFLib;
using namespace VTFLib::IO::Readers;

CMemoryReader::CMemoryReader(const void *vData, unsigned int uiBufferSize)
{
	this->bOpened = false;

	this->vData = vData;
	this->uiBufferSize = uiBufferSize;
}

CMemoryReader::~CMemoryReader()
{

}

bool CMemoryReader::Opened() const
{
	return this->bOpened;
}

bool CMemoryReader::Open()
{
	if(vData == 0)
	{
		LastError.Set("Memory stream is null.");
		return false;
	}

	this->uiPointer = 0;

	this->bOpened = true;

	return true;
}

void CMemoryReader::Close()
{
	this->bOpened = false;
}

unsigned int CMemoryReader::GetStreamSize() const
{
	if(!this->bOpened)
	{
		return 0;
	}

	return this->uiBufferSize;
}

unsigned int CMemoryReader::GetStreamPointer() const
{
	if(!this->bOpened)
	{
		return 0;
	}

	return this->uiPointer;
}

unsigned int CMemoryReader::Seek(long lOffset, unsigned int uiMode)
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
			this->uiPointer = this->uiBufferSize;
			break;
	}

	long lPointer = (long)this->uiPointer + lOffset;

	if(lPointer < 0)
	{
		lPointer = 0;
	}

	if(lPointer > (long)this->uiBufferSize)
	{
		lPointer = (long)this->uiBufferSize;
	}

	this->uiPointer = (unsigned int)lPointer;

	return this->uiPointer;
}

bool CMemoryReader::Read(char &cChar)
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
		cChar = *((char *)this->vData + this->uiPointer++);

		return true;
	}
}

unsigned int CMemoryReader::Read(void *data, unsigned int uiBytes)
{
	if(!this->bOpened)
	{
		return 0;
	}

	if(this->uiPointer == this->uiBufferSize)
	{
		return 0;
	}
	else if(this->uiPointer + uiBytes > this->uiBufferSize) // This right?
	{
		uiBytes = this->uiBufferSize - this->uiPointer;

		FMemory::Memcpy(data, (unsigned char *)vData + this->uiPointer, uiBytes);

		this->uiPointer = this->uiBufferSize;

		LastError.Set("End of memory stream.");

		return uiBytes;
	}
	else
	{
		FMemory::Memcpy(data, (unsigned char *)vData + this->uiPointer, uiBytes);

		this->uiPointer += uiBytes;

		return uiBytes;
	}
}