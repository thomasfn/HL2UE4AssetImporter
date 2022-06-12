/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VMTWrapper.h"
#include "VTFLib.h"
#include "VMTFile.h"
#include "Containers/Array.h"

using namespace VTFLib;

namespace VTFLib
{
	TArray<int> CurrentIndex;
	Nodes::CVMTGroupNode *CurrentNode = 0;
}

//
// vlMaterialBound()
// Returns true if an material is bound, false otherwise.
//
bool vlMaterialIsBound()
{
	if(!bInitialized)
	{
		LastError.Set("VTFLib not initialized.");
		return false;
	}

	return Material != 0;
}

//
// vlBindMaterial()
// Bind a material to operate on.
// All library routines will use this material.
//
bool vlBindMaterial(unsigned int uiMaterial)
{
	if(!bInitialized)
	{
		LastError.Set("VTFLib not initialized.");
		return false;
	}

	if(uiMaterial >= (unsigned int)MaterialFVector3f->Num() || (*MaterialFVector3f)[uiMaterial] == 0)
	{
		LastError.Set("Invalid material.");
		return false;
	}

	if(Material == (*MaterialFVector3f)[uiMaterial])	// If it is already bound do nothing.
		return true;

	Material = (*MaterialFVector3f)[uiMaterial];

	CurrentIndex.Empty();
	CurrentNode = 0;

	return true;
}

//
// vlCreateMaterial()
// Create an material to work on.
//
bool vlCreateMaterial(unsigned int *uiMaterial)
{
	if(!bInitialized)
	{
		LastError.Set("VTFLib not initialized.");
		return false;
	}

	MaterialFVector3f->Add(new CVMTFile());
	*uiMaterial = (unsigned int)MaterialFVector3f->Num() - 1;

	return true;
}

//
// vlDeleteMaterial()
// Delete a material and all resources associated with it.
//

void vlDeleteMaterial(unsigned int uiMaterial)
{
	if(!bInitialized)
		return;

	if(uiMaterial >= (unsigned int)MaterialFVector3f->Num())
		return;

	if((*MaterialFVector3f)[uiMaterial] == 0)
		return;

	if((*MaterialFVector3f)[uiMaterial] == Material)
	{
		Material = 0;

		CurrentIndex.Empty();
		CurrentNode = 0;
	}

	delete (*MaterialFVector3f)[uiMaterial];
	(*MaterialFVector3f)[uiMaterial] = 0;
}

bool vlMaterialCreate(const char *cRoot)
{
	if(Material == 0)
	{
		LastError.Set("No material bound.");
		return false;
	}

	CurrentIndex.Empty();
	CurrentNode = 0;

	return Material->Create(cRoot);
}

void vlMaterialDestroy()
{
	if(Material == 0)
	{
		return;
	}

	CurrentIndex.Empty();
	CurrentNode = 0;

	Material->Destroy();
}

bool vlMaterialIsLoaded()
{
	if(Material == 0)
	{
		LastError.Set("No material bound.");
		return false;
	}

	return Material->IsLoaded();
}

bool vlMaterialLoadLump(const void *lpData, unsigned int uiBufferSize)
{
	if(Material == 0)
	{
		LastError.Set("No material bound.");
		return false;
	}

	CurrentIndex.Empty();
	CurrentNode = 0;

	return Material->Load(lpData, uiBufferSize);
}

bool vlMaterialLoadProc(void *pUserData)
{
	if(Material == 0)
	{
		LastError.Set("No material bound.");
		return false;
	}

	CurrentIndex.Empty();
	CurrentNode = 0;

	return Material->Load(pUserData);
}

bool vlMaterialSaveLump(void *lpData, unsigned int uiBufferSize, unsigned int *uiSize)
{
	if(Material == 0)
	{
		LastError.Set("No material bound.");
		return false;
	}

	return Material->Save(lpData, uiBufferSize, *uiSize);
}

bool vlMaterialSaveProc(void *pUserData)
{
	if(Material == 0)
	{
		LastError.Set("No material bound.");
		return false;
	}

	return Material->Save(pUserData);
}

//
// vlMaterialGetCurretNode()
// Gets the current node in the transversal.
//
Nodes::CVMTNode *vlMaterialGetCurretNode()
{
	if(Material == 0 || CurrentNode == 0)
		return 0;

	int iIndex = CurrentIndex.Top();

	if(iIndex == -1 || iIndex == (int)CurrentNode->GetNodeCount())
	{
		return CurrentNode;
	}

	return CurrentNode->GetNode((unsigned int)iIndex);
}

//
// vlMaterialGetCurretNode()
// Gets the current node type in the transversal.
//
VMTNodeType vlMaterialGetCurretNodeType()
{
	if(Material == 0 || CurrentNode == 0)
		return NODE_TYPE_COUNT;

	int iIndex = CurrentIndex.Top();

	if(iIndex == -1)
	{
		return NODE_TYPE_GROUP;
	}

	if(iIndex == (int)CurrentNode->GetNodeCount())
	{
		return NODE_TYPE_GROUP_END;
	}

	return CurrentNode->GetNode((unsigned int)iIndex)->GetType();
}

//
// vlMaterialGetFirstNode()
// Moves the current node to the stat of the root node.
//
bool vlMaterialGetFirstNode()
{
	if(Material == 0 || Material->GetRoot() == 0)
		return false;

	CurrentNode = Material->GetRoot();
	CurrentIndex.Push(-1);

	return true;
}

//
// vlMaterialGetLastNode()
// Moves the current node to the end of the root node.
//
bool vlMaterialGetLastNode()
{
	if(Material == 0 || Material->GetRoot() == 0)
		return false;

	CurrentNode = Material->GetRoot();
	CurrentIndex.Push(CurrentNode->GetNodeCount());

	return true;
}

//
// vlMaterialGetNextNode()
// Moves the current node to the next node depth first style.
//
bool vlMaterialGetNextNode()
{
	if(Material == 0 || CurrentNode == 0)
		return false;

	// If we are at the end of the current node, go up a level.
	if(CurrentIndex.Top() == (int)CurrentNode->GetNodeCount())
	{
		if(CurrentNode->GetParent() != 0)
		{
			CurrentNode = CurrentNode->GetParent();
			CurrentIndex.Pop();

			return true;
		}
		else
		{
			return false;
		}
	}

	// Go to the next node in the current node.
	CurrentIndex.Top()++;

	// Check if we are at the end.
	if(CurrentIndex.Top() == (int)CurrentNode->GetNodeCount())
	{
		return true;
	}

	Nodes::CVMTNode *VMTNode = CurrentNode->GetNode((unsigned int)CurrentIndex.Top());

	// If the current node is a group, enter it at the start.
	if(VMTNode->GetType() == NODE_TYPE_GROUP)
	{
		CurrentNode = static_cast<Nodes::CVMTGroupNode *>(VMTNode);
		CurrentIndex.Push(-1);
	}

	return true;
}

//
// vlMaterialGetPreviousNode()
// Moves the current node to the previous node depth first style.  This
// is the reverse of vlMaterialGetNextNode().
//
bool vlMaterialGetPreviousNode()
{
	if(Material == 0 || CurrentNode == 0)
		return false;

	// If we are at the start of the current node, go up a level.
	if(CurrentIndex.Top() == -1)
	{
		if(CurrentNode->GetParent() != 0)
		{
			CurrentNode = CurrentNode->GetParent();
			CurrentIndex.Pop();

			return true;
		}
		else
		{
			return false;
		}
	}

	// Go to the previous node in the current node.
	CurrentIndex.Top()--;

	// Check if we are at the start.
	if(CurrentIndex.Top() == -1)
	{
		return true;
	}

	Nodes::CVMTNode *VMTNode = CurrentNode->GetNode((unsigned int)CurrentIndex.Top());

	// If the current node is a group, enter it at the end.
	if(VMTNode->GetType() == NODE_TYPE_GROUP)
	{
		CurrentNode = static_cast<Nodes::CVMTGroupNode *>(VMTNode);
		CurrentIndex.Push(CurrentNode->GetNodeCount());
	}

	return true;
}

//
// vlMaterialGetParentNode()
// Moves the current node to the current node's parent.
//
bool vlMaterialGetParentNode()
{
	if(Material == 0 || CurrentNode == 0)
		return false;

	// If we are not the root node, go up a level.
	if(CurrentNode->GetParent() != 0)
	{
		CurrentNode = CurrentNode->GetParent();
		CurrentIndex.Pop();

		return true;
	}

	return false;
}

//
// vlMaterialGetParentNode()
// Moves the current node to the specified child node of the current node.
//
bool vlMaterialGetChildNode(const char *cName)
{
	if(Material == 0 || CurrentNode == 0)
		return false;

	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	// Only groups have children.
	if(VMTNode->GetType() != NODE_TYPE_GROUP)
		return false;

	Nodes::CVMTGroupNode *VMTGroupNode = static_cast<Nodes::CVMTGroupNode *>(VMTNode);

	// Search for the specified child.
	for(unsigned int i = 0; i < VMTGroupNode->GetNodeCount(); i++)
	{
		VMTNode = VMTGroupNode->GetNode(i);
		if(_stricmp(VMTNode->GetName(), cName) == 0)
		{
			// If the child is a group, enter it at the start.
			if(VMTNode->GetType() == NODE_TYPE_GROUP)
			{
				CurrentNode = static_cast<Nodes::CVMTGroupNode *>(VMTNode);
				CurrentIndex.Push(-1);
			}
			else
			{
				CurrentIndex.Top() = (int)i;
			}

			return true;
		}
	}

	return false;
}

//
// vlMaterialGetNodeName()
// Gets the current node's name.
//
const char *vlMaterialGetNodeName()
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode == 0)
		return 0;

	return VMTNode->GetName();
}

//
// vlMaterialSetNodeName()
// Sets the current node's name.
//
void vlMaterialSetNodeName(const char *cName)
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode == 0)
		return;

	VMTNode->SetName(cName);
}

//
// GetNodeType()
// Get the type of node.  Returns VL_NODE_UNKOWN at the end of a group.
// For example, subsequent calls to vlGetNextNode() may return the following
// structure.  After the last VL_NODE_UNKOWN, vlGetNextNode() will fail.
//
// NODE_TYPE_GROUP
//   NODE_TYPE_GROUP
//     NODE_TYPE_STRING
//     NODE_TYPE_INTEGER
//   NODE_TYPE_GROUP_END
//   NODE_TYPE_GROUP
//     NODE_TYPE_SINGLE
//     NODE_TYPE_SINGLE
//   NODE_TYPE_GROUP_END
//   NODE_TYPE_STRING
// NODE_TYPE_GROUP_END
//
VMTNodeType vlMaterialGetNodeType()
{
	return vlMaterialGetCurretNodeType();
}

//
// vlMaterialGetNodeString()
// If the current node is a string node, this gets its value.
//
const char *vlMaterialGetNodeString()
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_STRING)
		return 0;

	return static_cast<Nodes::CVMTStringNode *>(VMTNode)->GetValue();
}

//
// vlMaterialSetNodeString()
// If the current node is a string node, this sets its value.
//
void vlMaterialSetNodeString(const char *cValue)
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_STRING)
		return;

	static_cast<Nodes::CVMTStringNode *>(VMTNode)->SetValue(cValue);
}

//
// vlMaterialGetNodeInteger()
// If the current node is a integer node, this gets its value.
//
unsigned int vlMaterialGetNodeInteger()
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_INTEGER)
		return 0;

	return static_cast<Nodes::CVMTIntegerNode *>(VMTNode)->GetValue();
}

//
// vlMaterialSetNodeInteger()
// If the current node is a integer node, this sets its value.
//
void vlMaterialSetNodeInteger(unsigned int iValue)
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_INTEGER)
		return;

	static_cast<Nodes::CVMTIntegerNode *>(VMTNode)->SetValue(iValue);
}

//
// vlMaterialGetNodeSingle()
// If the current node is a single node, this gets its value.
//
float vlMaterialGetNodeSingle()
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_SINGLE)
		return 0.0f;

	return static_cast<Nodes::CVMTSingleNode *>(VMTNode)->GetValue();
}

//
// vlMaterialSetNodeSingle()
// If the current node is a single node, this sets its value.
//
void vlMaterialSetNodeSingle(float sValue)
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_SINGLE)
		return;

	static_cast<Nodes::CVMTSingleNode *>(VMTNode)->SetValue(sValue);
}

//
// vlMaterialAddNodeGroup()
// If the current node is a group node, this adds a group node to the current node.
//
void vlMaterialAddNodeGroup(const char *cName)
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_GROUP)
		return;

	static_cast<Nodes::CVMTGroupNode *>(VMTNode)->AddGroupNode(cName);
}

//
// vlMaterialAddNodeString()
// If the current node is a group node, this adds a string node to the current node.
//
void vlMaterialAddNodeString(const char *cName, const char *cValue)
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_GROUP)
		return;

	static_cast<Nodes::CVMTGroupNode *>(VMTNode)->AddStringNode(cName, cValue);
}

//
// vlMaterialAddNodeInteger()
// If the current node is a group node, this adds a integer node to the current node.
//
void vlMaterialAddNodeInteger(const char *cName, unsigned int iValue)
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_GROUP)
		return;

	static_cast<Nodes::CVMTGroupNode *>(VMTNode)->AddIntegerNode(cName, iValue);
}

//
// vlMaterialAddNodeSingle()
// If the current node is a group node, this adds a single node to the current node.
//
void vlMaterialAddNodeSingle(const char *cName, float sValue)
{
	Nodes::CVMTNode *VMTNode = vlMaterialGetCurretNode();

	if(VMTNode->GetType() != NODE_TYPE_GROUP)
		return;

	static_cast<Nodes::CVMTGroupNode *>(VMTNode)->AddSingleNode(cName, sValue);
}