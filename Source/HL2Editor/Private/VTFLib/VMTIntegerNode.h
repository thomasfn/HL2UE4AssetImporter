/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef VMTINTEGERNODE_H
#define VMTINTEGERNODE_H


#include "VMTValueNode.h"

namespace VTFLib
{
	namespace Nodes
	{
		class CVMTIntegerNode : public CVMTValueNode
		{
		private:
			int iValue;

		public:
			CVMTIntegerNode(const char *cName);
			CVMTIntegerNode(const char *cName, const char *cValue);
			CVMTIntegerNode(const char *cName, int iValue);
			CVMTIntegerNode(const CVMTIntegerNode &IntegerNode);
			virtual ~CVMTIntegerNode();

			virtual void SetValue(const char *cValue);

			void SetValue(int iValue);
			const int GetValue() const;

			virtual VMTNodeType GetType() const;
			virtual CVMTNode *Clone() const;
		};
	}
}

#endif