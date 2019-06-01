/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef WRITER_H
#define WRITER_H



namespace VTFLib
{
	namespace IO
	{
		namespace Writers
		{
			class IWriter
			{
			public:
				virtual bool Opened() const = 0;

				virtual bool Open() = 0;
				virtual void Close() = 0;

				virtual unsigned int GetStreamSize() const = 0;
				virtual unsigned int GetStreamPointer() const = 0;

				virtual unsigned int Seek(long lOffset, unsigned int uiMode) = 0;

				virtual bool Write(char cChar) = 0;
				virtual unsigned int Write(void *vData, unsigned int uiBytes) = 0;
			};
		}
	}
}

#endif