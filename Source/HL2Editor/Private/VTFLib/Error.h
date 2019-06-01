/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

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
	\file Error.h
	\brief Error handling class header.
*/

#ifndef ERROR_H
#define ERROR_H

namespace VTFLib
{
	namespace Diagnostics
	{
		//! VTFLib Error handling class
		/*!
			The Error handling class allows you to aceess a text description 
			for the last error encountered.
		*/
		class CError
		{
		private:
			char *cErrorMessage;

		public:
			CError();
			~CError();

			//! Clear the error message buffer.
			void Clear();

			//! Get the error message text.
			const char *Get() const;

			//! Set the error message buffer.
			void SetFormatted(const char *cFormat, ...);
			void Set(const char *cErrorMessage, bool bSystemError = false);
		};
	}
}

#endif