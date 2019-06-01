/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef VTFWRAPPER_H
#define VTFWRAPPER_H



#ifdef __cplusplus
extern "C" {
#endif

//
// Memory managment routines.
//

bool vlMaterialIsBound();
bool vlBindMaterial(unsigned int uiMaterial);

bool vlCreateMaterial(unsigned int *uiMaterial);
void vlDeleteMaterial(unsigned int uiMaterial);

//
// Library routines.  (Basically class wrappers.)
//

bool vlMaterialCreate(const char *cRoot);
void vlMaterialDestroy();

bool vlMaterialIsLoaded();

bool vlMaterialLoad(const char *cFileName);
bool vlMaterialLoadLump(const void *lpData, unsigned int uiBufferSize);
bool vlMaterialLoadProc(void *pUserData);

bool vlMaterialSave(const char *cFileName);
bool vlMaterialSaveLump(void *lpData, unsigned int uiBufferSize, unsigned int *uiSize);
bool vlMaterialSaveProc(void *pUserData);

//
// Node routines.
//

bool vlMaterialGetFirstNode();
bool vlMaterialGetLastNode();
bool vlMaterialGetNextNode();
bool vlMaterialGetPreviousNode();

bool vlMaterialGetParentNode();
bool vlMaterialGetChildNode(const char *cName);

const char *vlMaterialGetNodeName();
void vlMaterialSetNodeName(const char *cName);

VMTNodeType vlMaterialGetNodeType();

const char *vlMaterialGetNodeString();
void vlMaterialSetNodeString(const char *cValue);

unsigned int vlMaterialGetNodeInteger();
void vlMaterialSetNodeInteger(unsigned int iValue);

float vlMaterialGetNodeSingle();
void vlMaterialSetNodeSingle(float sValue);

void vlMaterialAddNodeGroup(const char *cName);
void vlMaterialAddNodeString(const char *cName, const char *cValue);
void vlMaterialAddNodeInteger(const char *cName, unsigned int iValue);
void vlMaterialAddNodeSingle(const char *cName, float sValue);

#ifdef __cplusplus
}
#endif

#endif