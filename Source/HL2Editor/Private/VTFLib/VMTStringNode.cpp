/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VMTStringNode.h"

using namespace VTFLib::Nodes;

CVMTStringNode::CVMTStringNode(const char *name) : CVMTValueNode(name)
{
	this->cValue = new char[1];
	*this->cValue = '\0';
}

CVMTStringNode::CVMTStringNode(const char *name, const char *value) : CVMTValueNode(name)
{
	this->cValue = new char[strlen(value) + 1];
	strcpy_s(this->cValue, strlen(value) + 1, value);
}

CVMTStringNode::CVMTStringNode(const CVMTStringNode &StringNode) : CVMTValueNode(StringNode.GetName())
{
	this->cValue = new char[strlen(StringNode.cValue) + 1];
	strcpy_s(this->cValue, strlen(StringNode.cValue) + 1, StringNode.cValue);
}

CVMTStringNode::~CVMTStringNode()
{
	delete this->cValue;
}

void CVMTStringNode::SetValue(const char *value)
{
	delete this->cValue;
	this->cValue = new char[strlen(value) + 1];
	strcpy_s(this->cValue, strlen(value) + 1, value);
}

const char *CVMTStringNode::GetValue() const
{
	return this->cValue;
}

VMTNodeType CVMTStringNode::GetType() const
{
	return NODE_TYPE_STRING;
}

CVMTNode *CVMTStringNode::Clone() const
{
	return new CVMTStringNode(*this);
}