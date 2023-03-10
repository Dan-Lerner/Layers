/*
 * SectorsTypes.h
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#ifndef SECTORSTYPES_H_
#define SECTORSTYPES_H_

namespace DLayers
{

// IDs for sectors types

enum
{
	SECTOR_SUBSTRATE = 1,

	SECTOR,
	SECTOR_CHAIN,
	SECTOR_INDEXED,

	SECTOR_FIXED,
	SECTOR_INDEXER,
	SECTOR_INDEXER_STORE,
	SECTOR_RECORD_INDEXER,

	SECTOR_RECORD,

	SECTOR_ICONTEXT,

	SECTOR_CONTEXT_DISK,

	SECTOR_MEM_CONTEXT,
	SECTOR_MEM_CONTEXT_ATTACHABLE,
	SECTOR_MEM_CONTEXT_HEAP,
	SECTOR_MEM_CONTEXT_SHARED,
	SECTOR_MEM_CONTEXT_SYSTEMV,
	SECTOR_MEM_CONTEXT_DISKMAP,

	SECTOR_ELEMENT,
	SECTOR_CELL,
	SECTOR_EXTERN_PROPERTIES,
	SECTOR_VALUE,
	SECTOR_ISTRING,
	SECTOR_STRING,
	SECTOR_USTRING,
};

} /* namespace DLayers */

#endif /* SECTORSTYPES_H_ */
