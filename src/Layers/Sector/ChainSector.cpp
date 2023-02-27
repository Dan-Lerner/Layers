/*
 * ChainSector.cpp
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#include <alloca.h>

#include "ChainSector.h"
#include "IHeaderStore.h"

namespace Layers
{

ChainSector::ChainSector()
{
	Set(0x0000, D_RET_ERROR);
}

ChainSector::ChainSector(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

ChainSector::~ChainSector()
{
}

void ChainSector::Set(WORD flags, D_SECTOR_ID id)
{
	Sector::Set(flags, id);
	m_pPrevSector = nullptr;
	m_pNextSector = nullptr;
}

bool ChainSector::CreateHead(ChainSector* pChain, Header* pHeader, IHeaderStore* pHeaderStore)
{
	if (!pChain || !pHeader)
	{
		return false;
	}

	// Offset correction
	pHeader->location.Move(0);

	ChainSector* pHead = pChain->ChainHead();

	// Space requesting before first sector
	if (!pHead->RequestSpaceBefore(CalcSectorSize(pHeader)))
	{
		return false;
	}

	// Sector creation
	if (Create(pChain->m_pUnderLayer, pHeader, pHeaderStore))
	{
		BindHead(pChain);

		return true;
	}

	return Reset();
}

bool ChainSector::CreateAfter(ChainSector* pSector, Header* pHeader, IHeaderStore* pHeaderStore)
{
	if (!pSector || !pHeader)
	{
		return false;
	}

	// Offset correction
	pHeader->location.Move(pSector->m_pHeader->location.RightMargin());

	// Space requesting after pPrevSector
	if (!pSector->RequestSpaceAfter(CalcSectorSize(pHeader)))
	{
		return false;
	}

	// Sector creation
	if (Create(pSector->m_pUnderLayer, pHeader, pHeaderStore))
	{
		BindAfter(pSector);

		return true;
	}

	return Reset();
}

bool ChainSector::CreateTail(ChainSector* pChain, Header* pHeader, IHeaderStore* pHeaderStore)
{
	return CreateAfter(pChain->ChainTail(), pHeader, pHeaderStore);
}

bool ChainSector::AttachToChain(ChainSector* pChain, Header* pHeader, D_OFFSET offset, IHeaderStore* pHeaderStore)
{
	if (Attach(pChain->m_pUnderLayer, pHeader, offset, pHeaderStore))
	{
		// Searching for object followed by insertion place

		ChainSector* pPrevSector = pChain->ChainTail();

		while (pPrevSector && m_pHeader->location.offset < pPrevSector->m_pHeader->location.offset)
		{
			pPrevSector = pPrevSector->ChainPrev();
		}

		// Pointers binding
		if (pPrevSector)
		{
			BindAfter(pPrevSector);
		}
		else
		{
			BindHead(pChain);
		}

		return true;
	}

	return Reset();
}

bool ChainSector::Layon(Substrate* pUnderLayer)
{
	return ChainHead()->Sector::Layon(pUnderLayer);
}

bool ChainSector::OrderChain()
{
	ChainSector* pSector = ChainHead();
	ChainSector* pNextSector = pSector->ChainNext();

	while (pSector)
	{
		if (!pSector->m_pHeader)
		{
			// Sector has deleted from another object
			// !!! Works only with HeaderStore yet
			pSector->BindNeighbors();
			pSector = pNextSector;
		}
		else if (pNextSector && pSector->m_pHeader->location.offset > pNextSector->m_pHeader->location.offset)
		{
			// If next sector placed before pSector, then shift pNextSector

			ChainSector* pSectorAfter = pSector->ChainPrev();

			while (pSectorAfter && pSectorAfter->m_pHeader->location.offset > pNextSector->m_pHeader->location.offset)
			{
				pSectorAfter = pSectorAfter->ChainPrev();
			}

			pNextSector->MoveInChain(pSectorAfter);
		}
		else
		{
			pSector = pNextSector;
		}

		pNextSector = pSector ? pSector->ChainNext() : nullptr;
	}

	return true;
}

bool ChainSector::InChain()
{
	return m_pPrevSector != nullptr || m_pNextSector != nullptr;
}

bool ChainSector::InHead()
{
	return m_pPrevSector == nullptr;
}

bool ChainSector::InTail()
{
	return m_pNextSector == nullptr;
}

ChainSector* ChainSector::ChainHead()
{
	ChainSector* pSector = this;

	while (pSector->ChainPrev())
	{
		pSector = pSector->ChainPrev();
	}

	return pSector;
}

ChainSector* ChainSector::ChainTail()
{
	ChainSector* pSector = this;

	while (pSector->ChainNext())
	{
		pSector = pSector->ChainNext();
	}

	return pSector;
}

ChainSector* ChainSector::ChainNext()
{
	return m_pNextSector;
}

ChainSector* ChainSector::ChainPrev()
{
	return m_pPrevSector;
}

bool ChainSector::MoveSector(D_OFFSET offset)
{
	if (!Attached() || m_pHeader->Unmovable())
	{
		return false;
	}

	// New location
	Location location(offset, Size());

	// Space request
	if (offset > m_pHeader->location.offset ?
			RequestSpaceAfter(offset - m_pHeader->location.offset) :
			RequestSpaceBefore(m_pHeader->location.offset - offset))
	{
		// Current location saving
		location = m_pHeader->location;

		// If embedded Header is used, then reconfiguring pointers to headers in the cascade
		OffsetInsideHeader(offset - m_pHeader->location.offset);

		// Move by means of Substrate
		if (t_MoveBinary(offset, &location))
		{
			// Sets new offset
			return SetOffset(offset);
		}
	}

	return false;
}

bool ChainSector::MoveOn(ChainSector* pSector)
{
	// Determine the direction of moving
	bool moveForward = pSector->m_pHeader->location.offset > m_pHeader->location.offset;

	// Sector offset after moving
	D_OFFSET offsetNew = moveForward ?
			pSector->m_pHeader->location.RightMargin() - m_pHeader->location.size :
			pSector->m_pHeader->location.offset;

	if (moveForward)
	{
		// Check if sector moveable
		if (!ChainMovable(this, pSector->ChainNext()))
		{
			return false;
		}

		// Correcting the offsets ​​of sectors that use the embedded Header
		if (!ChainOffsetInsideHeader(ChainNext(), pSector->ChainNext(), -m_pHeader->location.size))
		{
			return false;
		}
	}
	else
	{
		// Check if sector moveable
		if (!ChainMovable(pSector, ChainNext()))
		{
			return false;
		}

		// Correcting the offsets ​​of sectors that use the embedded Header
		if (!ChainOffsetInsideHeader(pSector, this, m_pHeader->location.size))
		{
			return false;
		}
	}

	// Current location saving
	Location location = m_pHeader->location;

	// Correcting the offsets ​​of sectors that use the embedded Header
	OffsetInsideHeader(offsetNew - m_pHeader->location.offset);

	// Sectors moving
	if (t_MoveOnBinary(pSector, &location))
	{
		// Correcting the offsets ​​of the moved sectors

		if (moveForward)
		{
			if (!ChainCorrectOffset(ChainNext(), pSector->ChainNext(), -m_pHeader->location.size))
			{
				return false;
			}
		}
		else
		{
			if (!ChainCorrectOffset(pSector, this, m_pHeader->location.size))
			{
				return false;
			}
		}

		SetOffset(offsetNew);

		// Sets the pointers in chain
		MoveInChain(moveForward ? pSector : pSector->ChainPrev());

		return true;
	}

	return false;
}

bool ChainSector::ChainMovable(ChainSector* pFirst, ChainSector* pLast)
{
	// Check if sectors are moveable

	while (pFirst != pLast)
	{
		if (pFirst->m_pHeader->Unmovable())
		{
			return false;
		}

		pFirst = pFirst->ChainNext();
	}

	return true;
}

bool ChainSector::ChainOffsetInsideHeader(ChainSector* pFirst, ChainSector* pLast, D_OFFSET offsetDelta)
{
	// Correcting the offsets ​​of sectors that use the embedded Header

	while (pFirst != pLast)
	{
		pFirst->OffsetInsideHeader(offsetDelta);
		pFirst = pFirst->ChainNext();
	}

	return true;
}

bool ChainSector::ChainCorrectOffset(ChainSector* pFirst, ChainSector* pLast, D_OFFSET offsetDelta)
{
	// Offsets correction in Headers

	while (pFirst != pLast)
	{
		if (!pFirst->SetOffset(pFirst->m_pHeader->location.offset + offsetDelta))
		{
			return false;
		}

		pFirst = pFirst->ChainNext();
	}

	return true;
}

void ChainSector::InitOffset(ChainSector* pSector, D_OFFSET offset)
{
	// If embedded Header is used, then we move pointer to it avoiding loosing
	if (pSector->CanUseInsideHeader())
	{
		pSector->OffsetInsideHeader(offset);
	}

	// Sets new offset in Header
	pSector->SetOffset(offset);

	// Updating of pointers in cascade
	pSector->UpdatePointers();
}

bool ChainSector::RequestSpaceBefore(D_SIZE size)
{
	if (ChainPrev())
	{
		if (ChainPrev()->m_pHeader->location.RightMargin() <= m_pHeader->location.offset - size)
		{
			// There is enough free space before the sector
			return true;
		}

		if (ChainPrev()->MoveSector(m_pHeader->location.offset - size - ChainPrev()->m_pHeader->location.size))
		{
			// Trying to move previous sectors
			return true;
		}

		// If the search for a space before the sector is unsuccessful, we shift the sectors to the end
		return ChainPrev()->RequestSpaceAfter(size);
	}

	// We request a space before this sector, if sector is the first
	if (m_pHeader->location.offset < size)
	{
		return MoveSector(size);
	}

	// Tere is no one previous sector and there is enough space at the beginning of the chain
	return true;
}

bool ChainSector::RequestSpaceAfter(D_SIZE size)
{
	// Demanding space
	Location location(m_pHeader->location.RightMargin(), size);

	if (ChainNext())
	{
		// We request a place between this and the next sector in the chain
		if (location.RightMargin() > ChainNext()->m_pHeader->location.offset)
		{
			return ChainNext()->MoveSector(location.RightMargin());
		}

		return true;
	}

	// We request a place at the substrate if this sector is the last in the chain
	return m_pUnderLayer->RequestSpace(location) == size;
}

void ChainSector::BindHead(ChainSector* pChain)
{
	// Adds object to the beginning of chain

	ChainSector* pHead = pChain->ChainHead();
	BindNextNeighbor(pHead);
	BindPrevNeighbor(nullptr);

	if (m_pUnderLayer->GetOverLayer() == pHead)
	{
		// If chain is in cascade, we must bind this sector to cascade
		Sector::Layon(m_pUnderLayer);
	}
}

void ChainSector::BindAfter(ChainSector* pSector)
{
	BindNextNeighbor(pSector->ChainNext());
	BindPrevNeighbor(pSector);
}

void ChainSector::BindNeighbors()
{
	if (ChainPrev())
	{
		ChainPrev()->BindNextNeighbor(ChainNext());
	}
	else
	{
		// Sector is first in the chain

		if (ChainNext())
		{
			if (m_pUnderLayer->GetOverLayer() == this)
			{
				// If the chain is in a cascade, we embed the next sector in the cascade
				ChainNext()->Sector::Layon(m_pUnderLayer);
			}

			ChainNext()->BindPrevNeighbor(nullptr);
		}
		else
		{
			// The sector was the only one in the chain
			if (m_pUnderLayer->GetOverLayer() == this)
			{
				// If the sector was binded in the cascade, break the cascade
				m_pUnderLayer->SetOverLayer(nullptr);
			}
		}
	}
}

void ChainSector::BindPrevNeighbor(ChainSector* pPrevSector)
{
	m_pPrevSector = pPrevSector;

	if (pPrevSector)
	{
		pPrevSector->m_pNextSector = this;
	}
}

void ChainSector::BindNextNeighbor(ChainSector* pNextSector)
{
	m_pNextSector = pNextSector;

	if (pNextSector)
	{
		pNextSector->m_pPrevSector = this;
	}
}

void ChainSector::MoveInChain(ChainSector* pSectorAfter)
{
	ChainSector* pHead = ChainHead();

	BindNeighbors();

	if (pSectorAfter)
	{
		BindAfter(pSectorAfter);
	}
	else
	{
		BindHead(pHead);
	}
}

short ChainSector::CheckLayoutCascade(bool proceedChain, bool checkUnderLayer, bool checkOverLayer)
{
	if (proceedChain && !OrderChain())
	{
		return D_RET_ERROR;
	}

	short ret = Sector::CheckLayoutCascade(proceedChain, checkUnderLayer, checkOverLayer);

	if (proceedChain && ChainNext())
	{
		// We check the changes in the position of sectors of the chain
		// but only in the case when this check is initiated from the underlying sector

		if (ChainNext()->CheckLayoutCascade(true, false, true) == D_RET_ERROR)
		{
			return D_RET_ERROR;
		}
	}

	return ret;
}

void ChainSector::UpdatePointersCascade(bool proceedChain)
{
	if (MemAvailable())
	{
		InitPointers();

		// Forwarding the operation to Overlayer
		if (m_pOverLayer)
		{
			static_cast<ChainSector*>(m_pOverLayer)->UpdatePointersCascade(true);
		}

		// Forwarding the operation to chain
		if (proceedChain && ChainNext())
		{
			ChainNext()->UpdatePointersCascade(true);
		}
	}
}

void ChainSector::OffsetInsideHeaderCascade(D_OFFSET offset, bool proceedChain)
{
	if (MemAvailable())
	{
		OffsetHeaderP(offset);

		// Forwarding the operation to Overlayer
		if (m_pOverLayer)
		{
			static_cast<ChainSector*>(m_pOverLayer)->OffsetInsideHeaderCascade(offset, true);
		}

		// Forwarding the operation to chain
		if (proceedChain && ChainNext())
		{
			ChainNext()->OffsetInsideHeaderCascade(offset, true);
		}
	}
}

bool ChainSector::CollapseCascade(bool proceedChain)
{
	if (!Attached())
	{
		return false;
	}

	// Forwarding the operation to Overlayer
	if (m_pOverLayer && !static_cast<ChainSector*>(m_pOverLayer)->CollapseCascade(true))
	{
		return false;
	}

	// Minimize the sector and move it close to the previous sector
	if (!t_CollapseLayer())
	{
		return false;
	}

	// Forwarding the operation to chain
	if (proceedChain)
	{
		// Minimize the Sector or Underlayer if the sector is last
		// May be it's not so correct that the substrate minimizes itself in t_CollapseLayer()
		return ChainNext() ?
				ChainNext()->CollapseCascade(true) :
				m_pUnderLayer->Renew(m_pHeader->location.RightMargin());
	}

	return true;
}

bool ChainSector::CanDefragCascade(bool proceedChain)
{
	// We allow defragmentation for the cascade if at least one sector is subject to defragmentation

	if (m_pHeader->CanDefrag())
	{
		return true;
	}

	if (m_pOverLayer && static_cast<ChainSector*>(m_pOverLayer)->CanDefragCascade(true))
	{
		return true;
	}

	if (proceedChain && ChainNext() && ChainNext()->CanDefragCascade(true))
	{
		return true;
	}

	return false;
}

int ChainSector::DefragQuantumCascade(bool proceedChain, bool calcRemain)
{
	if (!Attached())
	{
		return D_RET_ERROR;
	}

	int ret = SECTOR_FRAG_COMPLETED;

	// Forwarding the operation to Overlayer
	if (m_pOverLayer)
	{
		ret = static_cast<ChainSector*>(m_pOverLayer)->DefragQuantumCascade(true, calcRemain);

		if (ret == D_RET_ERROR)
		{
			return D_RET_ERROR;
		}
	}

	// If the defragmentation step has already been taken,
	// then simply add the number of remaining steps in this sector
	// Otherwise, we take a defragmentation step in this sector
	if (m_pHeader->CanDefrag())
	{
		int sectorRet = t_DefragQuantum(calcRemain || ret != SECTOR_FRAG_COMPLETED);

		if (sectorRet == D_RET_ERROR)
		{
			return D_RET_ERROR;
		}

		ret += sectorRet;
	}

	// Forwarding the operation to chain
	// then simply add the number of remaining steps in this sector
	// Otherwise, we take a defragmentation step in chain
	if (proceedChain && ChainNext())
	{
		int chainRet = ChainNext()->DefragQuantumCascade(true, calcRemain || ret != SECTOR_FRAG_COMPLETED);

		if (chainRet == D_RET_ERROR)
		{
			return D_RET_ERROR;
		}

		ret += chainRet;
	}

	return ret;
}

bool ChainSector::StartSessionCascade(bool proceedChain)
{
	// If the previous session was terminated incorrectly (was not saved),
	// re-initialize the session parameters
	if (m_pHeader->SessionOpened() && !t_InitSession())
	{
		return false;
	}

	m_pHeader->state.OpenSession();

	// SavedHeader updating
	if (!UpdateSavedHeader())
	{
		return false;
	}

	// Forwarding the operation to Overlayer
	if (m_pOverLayer && !static_cast<ChainSector*>(m_pOverLayer)->StartSessionCascade(true))
	{
		return false;
	}

	// Forwarding the operation to chain
	if (proceedChain && ChainNext() && ChainNext()->StartSessionCascade(true))
	{
		return false;
	}

	return true;
}

bool ChainSector::CloseSessionCascade(bool proceedChain)
{
	m_pHeader->state.CloseSession();

	// SavedHeader updating
	if (!UpdateSavedHeader())
	{
		return false;
	}

	// Forwarding the operation to Overlayer
	if (m_pOverLayer && !static_cast<ChainSector*>(m_pOverLayer)->CloseSessionCascade(true))
	{
		return false;
	}

	// Forwarding the operation to chain
	if (proceedChain && ChainNext() && ChainNext()->CloseSessionCascade(true))
	{
		return false;
	}

	return true;
}

bool ChainSector::t_MoveBinary(D_OFFSET offset, Location* pLocation)
{
	return m_pUnderLayer->Move(*pLocation, offset);
}

bool ChainSector::t_MoveOnBinary(ChainSector* pSector, Location* pLocation)
{
	// Determine the direction of movement
	bool moveForward = pSector->m_pHeader->location.offset > pLocation->offset;

	// Sector offset after moving
	D_OFFSET offsetNew = moveForward ?
			pSector->m_pHeader->location.RightMargin() -
			pLocation->size : pSector->m_pHeader->location.offset;

	// Binary shifting
	return m_pUnderLayer->Shift(*pLocation, offsetNew);
}

bool ChainSector::t_CollapseLayer()
{
	// Move the sector close to the previous one
	if (!m_pHeader->Unmovable())
	{
		D_OFFSET offsetPrevMargin = ChainPrev() ? ChainPrev()->m_pHeader->location.RightMargin() : 0;

		if (offsetPrevMargin < m_pHeader->location.offset && !MoveSector(offsetPrevMargin))
		{
			return false;
		}
	}

	return true;
}

bool ChainSector::t_PreRenew(D_SIZE sizeOld, D_SIZE sizeNew)
{
	if (!Sector::t_PreRenew(sizeOld, sizeNew))
	{
		return false;
	}

	if (sizeNew > ContentSize())
	{
		// If increasing of size is required, then space is requested
		return RequestSpaceAfter(sizeNew - ContentSize());
	}

	// If size reduction is required, then simply return true
	return true;
}

bool ChainSector::t_Detach()
{
	BindNeighbors();

	return true;
}

} /* namespace DLayers */
