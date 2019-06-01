/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef READER_H
#define READER_H

#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2

namespace VTFLib
{
	namespace IO
	{
		namespace Readers
		{
			class IReader
			{
			public:
				virtual bool Opened() const = 0;

				virtual bool Open() = 0;
				virtual void Close() = 0;

				virtual unsigned int GetStreamSize() const = 0;
				virtual unsigned int GetStreamPointer() const = 0;

				virtual unsigned int Seek(long lOffset, unsigned int uiMode) = 0;

				virtual bool Read(char &cChar) = 0;
				virtual unsigned int Read(void *vData, unsigned int uiBytes) = 0;
			};
		}
	}
}

#endif