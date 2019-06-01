/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VMTSingleNode.h"

using namespace VTFLib::Nodes;

CVMTSingleNode::CVMTSingleNode(const char *name) : CVMTValueNode(name)
{
	this->fValue = (float)0.0;
}

CVMTSingleNode::CVMTSingleNode(const char *name, const char *value) : CVMTValueNode(name)
{
	this->SetValue(value);
}

CVMTSingleNode::CVMTSingleNode(const char *name, float value) : CVMTValueNode(name)
{
	this->fValue = value;
}

CVMTSingleNode::CVMTSingleNode(const CVMTSingleNode &SingleNode) : CVMTValueNode(SingleNode.GetName())
{
	this->fValue = SingleNode.fValue;
}

CVMTSingleNode::~CVMTSingleNode()
{

}

void CVMTSingleNode::SetValue(const char *value)
{
	this->fValue = (float)atof(value);
}

void CVMTSingleNode::SetValue(float value)
{
	this->fValue = value;
}

const float CVMTSingleNode::GetValue() const
{
	return this->fValue;
}

VMTNodeType CVMTSingleNode::GetType() const
{
	return NODE_TYPE_SINGLE;
}

CVMTNode *CVMTSingleNode::Clone() const
{
	return new CVMTSingleNode(*this);
}