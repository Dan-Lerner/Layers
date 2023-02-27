/*
 * Sector.h
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#ifndef SECTOR_H_
#define SECTOR_H_

#include "../Substrate/Substrate.h"

namespace Layers
{

// class Sector
// The sector placed on a context or other sector and implements binary operations on the content

class Sector : public Substrate
{
public:

	Sector();
	Sector(WORD fLags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~Sector();

protected:

	// Fields initialization
	virtual void Set(WORD flags, D_SECTOR_ID id);

public:

	// Sector type
	virtual short Type() { return SECTOR; };

	// true, if sector attached
	virtual bool Attached() { return m_pUnderLayer; };

// Direct memory operations are unavailable
public:

	// Must return true, if sector in memory
	virtual bool MemAvailable();

	virtual BYTE* ContentP();

// Creation

// Settings must be initialized in Header
// and Location too
// In location.size you must set CONTENT size
// (it is recalculated to the whole sector size during initialization)
public:

	virtual bool Create(Substrate* pUnderLayer, Header* pHeader, IHeaderStore* pHeaderStore = nullptr);

// Attaching to existing context

// The following options for Setting parameters are possible

// 1. Header placed not in the sector (FLAG_SAVED_HEADER and USE_INSIDE_HEADER are not set)
// pHeader must be fully initialized (e.g. manually before attaching the first object
// or it is not the first object that is attached to the sector and pHeader is already
// initialized and used by other objects working with this sector

// size - must be set to whole sector size including Header

// 2. Header saved in sector (FLAG_SAVED_HEADER)
// pHeader.offset must be set
// pHeader is initialized inside the method using saved header. If the header is not saved
// completely, then non-persistent fields must be initialized manually in pHeader before
// attaching the first object

// 3. Header placed in sector in memory (USE_INSIDE_HEADER),
// offset must be set
// pHeader and pHeaderStore are ignored

// 4. If Header saved in the sector and HeaderStore is used,
// then we may not to set the pHeader (pHeader = nullptr), but offset must be set
// The object reads the saved header, requests a place in the HeaderStore and initializes it
// !!! This option only works if the whole header is saved
// !!! If the header is partially saved, the result is undefined

// 5. If the HeaderStore already has a Header, then only strKey and pHeaderStore can be set
public:

	bool Attach(Substrate* pUnderLayer, Header* pHeader, D_OFFSET offset = D_RET_ERROR, IHeaderStore* pHeaderStore = nullptr);

// Combining layers into a cascade

// This method combines the sector with the substrate, forming a cascade
// We can apply cascade methods to cascades
public:

	virtual bool Layon(Substrate* pUnderLayer);

// Sector routines
public:

	// Moving the sector to the offset position on the substrate
	virtual bool MoveSector(D_OFFSET offset);

// Cascade methods
protected:

	// Sector location changing check
	virtual short CheckLayoutCascade(bool proceedCheckChain = false, bool checkUnderLayer = true, bool checkOverLayer = true);

	short CheckHeaderStore();

// Typed-methods (t_) Sector
protected:

	// It called after object attaching
	virtual bool t_InitAttach() { return true; };

// Implementation of (t_) Substrate
protected:

	virtual void* t_GetMem();
	virtual bool t_PreRenew(D_SIZE sizeOld, D_SIZE sizeNew);
	virtual bool t_Renew(D_SIZE size);
	virtual bool t_PostRenew(D_SIZE sizeOld, D_SIZE sizeNew);
	virtual bool t_Detach();
	virtual bool t_Delete();
	virtual bool t_Clean(D_OFFSET offsetFrom, D_SIZE size);
	virtual bool t_ReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size);
	virtual bool t_WriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size);
	virtual D_SIZE t_Read(void* pBuffer, D_SIZE size);
	virtual bool t_Write(const void* pBuffer, D_SIZE size);
	virtual bool t_Copy(D_OFFSET offsetFrom, D_OFFSET offsetTo, D_SIZE size);
	virtual bool t_Fill(BYTE btFill, D_OFFSET offset, D_SIZE size);

protected:

	// The sector on which this sector is placed (Substrate)
	Substrate* m_pUnderLayer;
};

} /* namespace DLayers */

#endif /* SECTOR_H_ */
