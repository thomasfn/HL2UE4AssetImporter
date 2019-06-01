/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef VMTFILE_H
#define VMTFILE_H


#include "Readers.h"
#include "Writers.h"
#include "VMTNodes.h"

#ifdef __cplusplus
extern "C" {
#endif

//! VMT parsing mode.
typedef enum tagVMTParseMode
{
	PARSE_MODE_STRICT = 0,
	PARSE_MODE_LOOSE,
	PARSE_MODE_COUNT
} VMTParseMode;

#ifdef __cplusplus
}
#endif

namespace VTFLib
{
	class CVMTFile
	{
	private:
		Nodes::CVMTGroupNode *Root;

	public:
		CVMTFile();
		CVMTFile(const CVMTFile &VMTFile);
		~CVMTFile();

	public:
		bool Create(const char *cRoot);
		void Destroy();

		bool IsLoaded() const;

		bool Load(const void *lpData, unsigned int uiBufferSize);
		bool Load(void *pUserData);

		bool Save(void *lpData, unsigned int uiBufferSize, unsigned int &uiSize) const;
		bool Save(void *pUserData) const;

	private:
		bool Load(IO::Readers::IReader *Reader);
		bool Save(IO::Writers::IWriter *Writer) const;

		//Nodes::CVMTNode *Load(IO::Readers::IReader *Reader, bool bInGroup);

		void Indent(IO::Writers::IWriter *Writer, unsigned int uiLevel) const;
		void Save(IO::Writers::IWriter *Writer, Nodes::CVMTNode *Node, unsigned int uiLevel = 0) const;

	public:
		Nodes::CVMTGroupNode *GetRoot() const;
	};
}

#endif