/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "Error.h"
#include <cstdarg>

using namespace VTFLib::Diagnostics;

CError::CError()
{
	this->cErrorMessage = 0;
}

CError::~CError()
{
	delete []this->cErrorMessage;
}

void CError::Clear()
{
	delete []this->cErrorMessage;
	this->cErrorMessage = 0;
}

const char *CError::Get() const
{
	return this->cErrorMessage != 0 ? this->cErrorMessage : "";
}

void CError::SetFormatted(const char *cFormat, ...)
{
	char cBuffer[2048];

	va_list ArgumentList;
	va_start(ArgumentList, cFormat);
	vsprintf_s(cBuffer, sizeof(cBuffer), cFormat, ArgumentList);
	va_end(ArgumentList);

	this->Set(cBuffer, false);
}

void CError::Set(const char *errMessage, bool systemError)
{
	char buffer[2048];
	if(systemError)
	{
		sprintf_s(buffer, sizeof(buffer), "System Error:\n%s", errMessage);
		
	}
	else
	{
		sprintf_s(buffer, sizeof(buffer), "Error:\n%s", errMessage);
	}
	
	cErrorMessage = new char[strlen(buffer) + 1];
	strcpy_s(cErrorMessage, sizeof(cErrorMessage), buffer);
}