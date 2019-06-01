/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef VMTGROUPNODE_H
#define VMTGROUPNODE_H


#include "VMTNode.h"

#include "VMTStringNode.h"
#include "VMTIntegerNode.h"
#include "VMTSingleNode.h"

#include <list>

namespace VTFLib
{
	class CVMTFile;

	namespace Nodes
	{
		class CVMTGroupNode : public CVMTNode
		{
		//private:
		//	friend class CVMTFile;	// For direct node addition.

		private:
			typedef std::list<CVMTNode *> CVMTNodeList;

		private:
			CVMTNodeList *VMTNodeList;

		public:
			CVMTGroupNode(const char *cName);
			CVMTGroupNode(const CVMTGroupNode &GroupNode);
			virtual ~CVMTGroupNode();

			virtual VMTNodeType GetType() const;
			virtual CVMTNode *Clone() const;

		public:
			unsigned int GetNodeCount() const;

			CVMTNode *AddNode(CVMTNode *VMTNode);
			CVMTGroupNode *AddGroupNode(const char *cName);
			CVMTStringNode *AddStringNode(const char *cName, const char *cValue);
			CVMTIntegerNode *AddIntegerNode(const char *cName, int iValue);
			CVMTSingleNode *AddSingleNode(const char *cName, float fValue);

			void RemoveNode(CVMTNode *VMTNode);
			void RemoveAllNodes();

			CVMTNode *GetNode(unsigned int uiIndex) const;
			CVMTNode *GetNode(const char *cName) const;		
		};
	}
}

#endif