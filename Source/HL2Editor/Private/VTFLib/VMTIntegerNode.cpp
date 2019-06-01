/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VMTIntegerNode.h"

using namespace VTFLib::Nodes;

CVMTIntegerNode::CVMTIntegerNode(const char *name) : CVMTValueNode(name)
{
	this->iValue = 0;
}

CVMTIntegerNode::CVMTIntegerNode(const char *name, const char *value) : CVMTValueNode(name)
{
	this->SetValue(value);
}

CVMTIntegerNode::CVMTIntegerNode(const char *name, int value) : CVMTValueNode(name)
{
	this->iValue = value;
}

CVMTIntegerNode::CVMTIntegerNode(const CVMTIntegerNode &IntegerNode) : CVMTValueNode(IntegerNode.GetName())
{
	this->iValue = IntegerNode.iValue;
}

CVMTIntegerNode::~CVMTIntegerNode()
{

}

void CVMTIntegerNode::SetValue(const char *value)
{
	this->iValue = atoi(value);
}

void CVMTIntegerNode::SetValue(int value)
{
	this->iValue = value;
}

const int CVMTIntegerNode::GetValue() const
{
	return this->iValue;
}

VMTNodeType CVMTIntegerNode::GetType() const
{
	return NODE_TYPE_INTEGER;
}

CVMTNode *CVMTIntegerNode::Clone() const
{
	return new CVMTIntegerNode(*this);
}