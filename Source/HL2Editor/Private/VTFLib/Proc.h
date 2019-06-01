/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef PROC_H
#define PROC_H


#include "Error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tagVLProc
{
	PROC_READ_CLOSE = 0,
	PROC_READ_OPEN,
	PROC_READ_READ,
	PROC_READ_SEEK,
	PROC_READ_TELL,
	PROC_READ_SIZE,
	PROC_WRITE_CLOSE,
	PROC_WRITE_OPEN,
	PROC_WRITE_WRITE,
	PROC_WRITE_SEEK,
	PROC_WRITE_SIZE,
	PROC_WRITE_TELL,
	PROC_COUNT
} VLProc;

typedef enum tagVLSeekMode
{
	SEEK_MODE_BEGIN = 0,
	SEEK_MODE_CURRENT,
	SEEK_MODE_END
} VLSeekMode;

typedef void (*PReadCloseProc)(void *);
typedef bool (*PReadOpenProc) (void *);
typedef unsigned int (*PReadReadProc)  (void *, unsigned int, void *);
typedef unsigned int (*PReadSeekProc) (long, VLSeekMode, void *);
typedef unsigned int (*PReadSizeProc) (void *);
typedef unsigned int (*PReadTellProc) (void *);

typedef void (*PWriteCloseProc)(void *);
typedef bool (*PWriteOpenProc) (void *);
typedef unsigned int (*PWriteWriteProc)  (void *, unsigned int, void *);
typedef unsigned int (*PWriteSeekProc) (long, VLSeekMode, void *);
typedef unsigned int (*PWriteSizeProc) (void *);
typedef unsigned int (*PWriteTellProc) (void *);

#ifdef __cplusplus
}
#endif

namespace VTFLib
{
	extern PReadCloseProc pReadCloseProc;
	extern PReadOpenProc pReadOpenProc;
	extern PReadReadProc pReadReadProc;
	extern PReadSeekProc pReadSeekProc;
	extern PReadSizeProc pReadSizeProc;
	extern PReadTellProc pReadTellProc;

	extern PWriteCloseProc pWriteCloseProc;
	extern PWriteOpenProc pWriteOpenProc;
	extern PWriteWriteProc pWriteWriteProc;
	extern PWriteSeekProc pWriteSeekProc;
	extern PWriteSizeProc pWriteSizeProc;
	extern PWriteTellProc pWriteTellProc;
}

#ifdef __cplusplus
extern "C" {
#endif

void vlSetProc(VLProc Proc, void *pProc);
void *vlGetProc(VLProc Proc);

#ifdef __cplusplus
}
#endif

#endif