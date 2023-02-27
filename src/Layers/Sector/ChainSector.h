/*
 * ChainSector.h
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#ifndef CHAINSECTOR_H_
#define CHAINSECTOR_H_

#include "Sector.h"

namespace Layers
{

// ChainSector objects can be combined in a Layer
// The layer is a chain of connected sectors laying on a common substrate
// For ChainSector cascade operations are available

class ChainSector : public Sector
{
public:

	ChainSector();
	ChainSector(WORD flags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~ChainSector();

protected:

	// Fields initialization
	virtual void Set(WORD flags, D_SECTOR_ID id);

public:

	// Sector type
	virtual short Type() { return CHAIN; };

// Creation

// These methods creates the chain of connected sectors
// pHeader->location.offset is ignored
// Sectors is placed to the beginning of the substrate or to previous sector
// The first sector of chain is created by Sector::Create
// You can attach chain to cascade using Layon method

public:

	// Creating a new sector at the beginning of chain
	virtual bool CreateHead(ChainSector* pChain, Header* pHeader, IHeaderStore* pHeaderStore = nullptr);

	// Creating a new sector at the position after pSector
	virtual bool CreateAfter(ChainSector* pSector, Header* pHeader, IHeaderStore* pHeaderStore = nullptr);

	// Creating a new sector at the end of chain
	virtual bool CreateTail(ChainSector* pChain, Header* pHeader, IHeaderStore* pHeaderStore = nullptr);

// Attaching

// pChain - any sector from the chain
// The attached sector automatically takes place in the objects chain according to the sector physical location
// The first sector in the chain is attached using Sector::Attach

public:

	// Attaching to the chain
	virtual bool AttachToChain(ChainSector* pChain, Header* pHeader, D_OFFSET offset = D_RET_ERROR, IHeaderStore* pHeaderStore = nullptr);

// 	Combining layers into a cascade
public:

	virtual bool Layon(Substrate* pUnderLayer);

// Ordering chain objects according to physical location of sectors
public:

	bool OrderChain();

// Chain navigation
public:

	// true, if object is in chain
	bool InChain();

	// true, if object is at the beginning of the chain
	bool InHead();

	// true, if object is at the end of the chain
	bool InTail();

	// First object in chain
	ChainSector* ChainHead();

	// Last object in chain
	ChainSector* ChainTail();

	// Next object in chain
	ChainSector* ChainNext();

	// Previous object in chain
	ChainSector* ChainPrev();

// Sector operations
// !!! Look at Movies.txt
public:

	// Moving the sector to the specified offset on the substrate
	virtual bool MoveSector(D_OFFSET offset);

	// Moving a sector in the layer chain to the location of pSector
	virtual bool MoveOn(ChainSector* pSector);

// Chain operations
protected:

	// true if subchain pFirst-pLast is movable
	bool ChainMovable(ChainSector* pFirst, ChainSector* pLast);

	// Offsets correction for subchain with embedded headers
	bool ChainOffsetInsideHeader(ChainSector* pFirst, ChainSector* pLast, D_OFFSET offsetDelta);

	// Offsets correction for Headers of subchain
	bool ChainCorrectOffset(ChainSector* pFirst, ChainSector* pLast, D_OFFSET offsetDelta);

protected:

	// Sector initialization after shifting
	void InitOffset(ChainSector* pSector, D_OFFSET offset);

// Space request within the chain
protected:

	// Space request of size bytes after sector
	virtual bool RequestSpaceAfter(D_SIZE size);

	// Space request of size bytes before sector
	virtual bool RequestSpaceBefore(D_SIZE size);

// Pointers binding
protected:

	// Binding to the beginning of the chain
	void BindHead(ChainSector* pChain);

	// Binding after pSector
	void BindAfter(ChainSector* pSector);

	// Binding of previous and next objects
	void BindNeighbors();

	// Binding with previous next object
	void BindPrevNeighbor(ChainSector* pPrevSector);

	// Binding with next object
	void BindNextNeighbor(ChainSector* pNextSector);

	// Binding to the place after pSectorAfter
	void MoveInChain(ChainSector* pSectorAfter);

// Cascade operations
protected:

	// Checks the sectors locations in cascade and layer
	virtual short CheckLayoutCascade(bool proceedCheckChain = false, bool checkUnderLayer = true, bool checkOverLayer = true);

	// Cascade pointers update
	virtual void UpdatePointersCascade(bool proceedChain = false);

	// Cascade initialization of header pointers inside the sectors on sector moving
	virtual void OffsetInsideHeaderCascade(D_OFFSET offset, bool proceedChain = false);

	// Collapses the sector to minimal size
	virtual bool CollapseCascade(bool proceedChain = false);

	// Cascade request for defrag availability
	virtual bool CanDefragCascade(bool proceedChain = false);

	// One step of defrag performing

	// Return value - the number of the defragmented element in the countdown(from N to 1)
	// (or the number of fragmented elements before the step))
	// SECTOR_FRAG_COMPLETED, if defragmentation not required
	virtual int DefragQuantumCascade(bool proceedChain = false, bool calcRemain = false);

	// Session opening - loads state
	virtual bool StartSessionCascade(bool proceedChain = false);

	// Session closing - saves state
	virtual bool CloseSessionCascade(bool proceedChain = false);

// Typed-функции (t_) ChainSector
protected:

	// Binary shifting to location
	virtual bool t_MoveBinary(D_OFFSET offset, Location* pLocation);

	// Binary shifting to the place of pSector in chain
	virtual bool t_MoveOnBinary(ChainSector* pSector, Location* pLocation);

	// This function cuts the right border of the layer if there is free space at the end
	virtual bool t_CollapseLayer();

	// One defragmentaton step is performed in sector (one element is defragmented)

	// Return value - the number of the defragmented element in the countdown (from N to 1)
	// (or the number of fragmented elements before the step))
	// SECTOR_FRAG_COMPLETED, if defragmentation not required

	// If calcRemain, then just returns the number of remaining operations without defragmentation
	virtual int t_DefragQuantum(bool calcRemain) { return SECTOR_FRAG_COMPLETED; };

	// Session initialization - determining the current state if loading of state is not possible
	virtual bool t_InitSession() { return true; };

// Implementation of (t_) Substrate
protected:

	virtual bool t_PreRenew(D_SIZE sizeOld, D_SIZE sizeNew);
	virtual bool t_Detach();

protected:

	// Previous sector in chain
	ChainSector* m_pPrevSector;

	ChainSector* m_pNextSector;
};

} /* namespace DLayers */

#endif /* CHAINSECTOR_H_ */
