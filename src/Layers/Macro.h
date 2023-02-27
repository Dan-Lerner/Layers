/*
 * Macro.h
 *
 *  Created on: 12 июля 2016 г.
 *      Author: dan
 */

#ifndef MACRO_H_
#define MACRO_H_

#pragma once
#pragma pack(1)


// For returning values
#define D_RET_OK		0
#define D_RET_ERROR		(int)-1

// Base types
//------------
//

typedef char				CHAR;
typedef wchar_t				UCHAR;
typedef unsigned char		BYTE;
typedef short				SHORT;
typedef unsigned short		WORD;
typedef int					INT;
typedef unsigned int		UINT;
typedef long long	 		LONG;
typedef unsigned long long 	ULONG;
typedef float			 	FLOAT;
typedef double			 	DOUBLE;

typedef int		D_SECTOR_ID;

typedef int		D_OFFSET;
typedef int		D_SIZE;

typedef int		D_INDEX;
typedef int		D_INDEX_COUNT;

typedef short	D_COLUMN_ID;
typedef short	D_COLUMN_INDEX;
typedef short	D_COLUMNS_COUNT;

typedef short	D_COLUMN_SET_ID;
typedef short	D_COLUMN_SET_INDEX;
typedef short	D_COLUMN_SETS_COUNT;

#endif /* MACRO_H_ */
