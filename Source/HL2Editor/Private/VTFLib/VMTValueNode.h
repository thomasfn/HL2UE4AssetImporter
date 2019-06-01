/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef VMTVALUENODE_H
#define VMTVALUENODE_H


#include "VMTNode.h"

namespace VTFLib
{
	namespace Nodes
	{
		class CVMTValueNode : public CVMTNode
		{
		public:
			CVMTValueNode(const char *cName);
			virtual ~CVMTValueNode();

			virtual void SetValue(const char *cValue) = 0;
		};
	}
}

#endif