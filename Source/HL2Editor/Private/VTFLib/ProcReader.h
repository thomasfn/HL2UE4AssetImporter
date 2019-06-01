/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef PROCREADER_H
#define PROCREADER_H


#include "Reader.h"

namespace VTFLib
{
	namespace IO
	{
		namespace Readers
		{
			class CProcReader : public IReader
			{
			private:
				bool bOpened;
				void *pUserData;

			public:
				CProcReader(void *pUserData);
				virtual ~CProcReader();

			public:
				virtual bool Opened() const;

				virtual bool Open();
				virtual void Close();

				virtual unsigned int GetStreamSize() const;
				virtual unsigned int GetStreamPointer() const;

				virtual unsigned int Seek(long lOffset, unsigned int uiMode);

				virtual bool Read(char &cChar);
				virtual unsigned int Read(void *vData, unsigned int uiBytes);
			};
		}
	}
}

#endif