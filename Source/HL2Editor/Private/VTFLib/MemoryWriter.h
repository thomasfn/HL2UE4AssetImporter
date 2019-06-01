/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef MEMORYWRITER_H
#define MEMORYWRITER_H


#include "Writer.h"

namespace VTFLib
{
	namespace IO
	{
		namespace Writers
		{
			class CMemoryWriter : public IWriter
			{
			private:
				bool bOpened;

				void *vData;
				unsigned int uiBufferSize;

				unsigned int uiPointer;
				unsigned int uiLength;

			public:
				CMemoryWriter(void *vData, unsigned int uiBufferSize);
				virtual ~CMemoryWriter();

			public:
				virtual bool Opened() const;

				virtual bool Open();
				virtual void Close();

				virtual unsigned int GetStreamSize() const;
				virtual unsigned int GetStreamPointer() const;

				virtual unsigned int Seek(long lOffset, unsigned int uiMode);

				virtual bool Write(char cChar);
				virtual unsigned int Write(void *vData, unsigned int uiBytes);
			};
		}
	}
}

#endif