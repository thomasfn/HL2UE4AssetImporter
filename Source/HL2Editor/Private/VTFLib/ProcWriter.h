/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef PROCWRITER_H
#define PROCWRITER_H


#include "Writer.h"

namespace VTFLib
{
	namespace IO
	{
		namespace Writers
		{
			class CProcWriter : public IWriter
			{
			private:
				bool bOpened;
				void *pUserData;

			public:
				CProcWriter(void *pUserData);
				virtual ~CProcWriter();

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