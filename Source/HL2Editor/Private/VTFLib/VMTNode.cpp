/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VMTNode.h"

using namespace VTFLib::Nodes;

CVMTNode::CVMTNode(const char *name)
{
	this->cName = new char[strlen(name) + 1];
	strcpy_s(this->cName, strlen(name) + 1, name);

	this->Parent = 0;
}

CVMTNode::~CVMTNode()
{
	delete []this->cName;
}

void CVMTNode::SetName(const char *name)
{
	delete []this->cName;
	this->cName = new char[strlen(name) + 1];
	strcpy_s(this->cName, strlen(name) + 1, name);
}

const char *CVMTNode::GetName() const
{
	return this->cName;
}

CVMTGroupNode *CVMTNode::GetParent()
{
	return this->Parent;
}