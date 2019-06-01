/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef VMTSINGLENODE_H
#define VMTSINGLENODE_H


#include "VMTValueNode.h"

namespace VTFLib
{
	namespace Nodes
	{
		class CVMTSingleNode : public CVMTValueNode
		{
		private:
			float fValue;

		public:
			CVMTSingleNode(const char *cName);
			CVMTSingleNode(const char *cName, const char *cValue);
			CVMTSingleNode(const char *cName, float fValue);
			CVMTSingleNode(const CVMTSingleNode &SingleNode);
			virtual ~CVMTSingleNode();

			virtual void SetValue(const char *cValue);

			void SetValue(float fValue);
			const float GetValue() const;

			virtual VMTNodeType GetType() const;
			virtual CVMTNode *Clone() const;
		};
	}
}

#endif