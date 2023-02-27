/*
* * Sector.cpp
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#include <alloca.h>

#include "Sector.h"
#include "IHeaderStore.h"

namespace Layers
{

Sector::Sector()
{
	Set(0x0000, D_RET_ERROR);
}

Sector::Sector(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

Sector::~Sector()
{
	// TODO Auto-generated destructor stub
}

void Sector::Set(WORD flags, D_SECTOR_ID id)
{
	Substrate::Set(flags, id);

	m_pUnderLayer = nullptr;
}

bool Sector::MemAvailable()
{
	return Attached() ? m_pUnderLayer->MemAvailable() : false;
}

BYTE* Sector::Sector::ContentP()
{
	return MemAvailable() ? static_cast<BYTE*>(t_GetMem()) + m_offsetContent : nullptr;
}

bool Sector::Create(Substrate* pUnderLayer, Header* pHeader, IHeaderStore* pHeaderStore)
{
	// Used Header
	Header* pUsedHeader = pHeaderStore ?
			RequestHeader(pHeaderStore, m_id, pHeader) :
			pHeader;

	if (!pUsedHeader)
	{
		return false;
	}

	// Total sector size calculation
	pUsedHeader->location.Resize(CalcSectorSize(pUsedHeader));

	// Requests the place in substrate
	if (!pUnderLayer->RequestSpace(pUsedHeader->location))
	{
		return false;
	}

	m_pHeaderStore = pHeaderStore;
	m_pUnderLayer = pUnderLayer;

	// Object initialization
	return InitCreate(pUsedHeader);
}

bool Sector::Attach(Substrate* pUnderLayer, Header* pHeader, D_OFFSET offset, IHeaderStore* pHeaderStore)
{
	Header header;
	Header* pUsedHeader;

	// Substrate
	m_pUnderLayer = pUnderLayer;

	if (CanUseInsideHeader())
	{
		// We substitute a header with a specific offset
		// for the case when the header is stored in sector memory
		// and used for work (USE_INSIDE_HEADER)

		header.location.offset = offset;

		pUsedHeader = &header;
	}
	else if (pHeaderStore)
	{
		// Trying to find an existing header in HeaderStore or to create a new one
		pUsedHeader = RequestHeader(pHeaderStore, m_id, pHeader);

		if (!pUsedHeader && IsSavedHeader())
		{
			// If not found in pHeaderStore, pHeader is not set and there is a SavedHeader
			// we read from saved

			// !!! Works only if the whole header is saved

			Location location(offset, t_SavedHeaderSize());

			if (!pUnderLayer->ReadFrom(&header, location))
			{
				return Reset();
			}

			// Memory allocation, knowing the real size of the Header
			pUsedHeader = static_cast<Header*>(::alloca(header.Size()));

			location.Resize(header.Size());

			// SavedHeader reading
			if (!pUnderLayer->ReadFrom(pUsedHeader, location))
			{
				return Reset();
			}

			pUsedHeader = RequestHeader(pHeaderStore, m_id, pUsedHeader);
		}
	}
	else
	{
		pUsedHeader = pHeader;
	}

	if (!pUsedHeader || !InitAttach(pUsedHeader))
	{
		return Reset();
	}

	// Checking if the sector is in the substrate space
	// and perform the final initialization
	if (m_pUnderLayer->BlockInside(m_pHeader->location) && t_InitAttach())
	{
		return true;
	}

	return Reset();
}

bool Sector::Layon(Substrate* pUnderLayer)
{
	if (Attached())
	{
		pUnderLayer->SetOverLayer(this);

		return true;
	}

	return false;
}

bool Sector::MoveSector(D_OFFSET offset)
{
	if (Attached() && !m_pHeader->Unmovable())
	{
		// New location
		Location location(offset, m_pHeader->location.size);

		// Requests the place in substrate
		if (m_pUnderLayer->RequestSpace(location))
		{
			// Current location saving
			location = m_pHeader->location;

			// If we use a header in sector memory, we reconfigure the pointer to the header,
			// so as not to lose it after moving
			OffsetInsideHeader(offset - m_pHeader->location.offset);

			// Sector moving by means of substrate
			if (m_pUnderLayer->Move(location, offset))
			{
				// Sets new offset
				return SetOffset(offset);
			}
		}
	}

	return false;
}

short Sector::CheckLayoutCascade(bool proceedChain, bool checkUnderLayer, bool checkOverLayer)
{
	short ret = CASCADE_LAYOUT_FORMER;

	if (checkUnderLayer)
	{
		ret = (static_cast<Sector*>(m_pUnderLayer))->CheckLayoutCascade(false, true, false);
	}

	if (Substrate::CheckLayoutCascade(false, false, checkOverLayer) == D_RET_ERROR)
	{
		return D_RET_ERROR;
	}

	return ret;
}

short Sector::CheckHeaderStore()
{
	// Sets the header pointer if stored in the HeaderStore
	if (m_pHeaderStore)
	{
		m_pHeader = m_pHeaderStore->Find(m_id, m_indexStore);

		if (!m_pHeader)
		{
			return D_RET_ERROR;
		}
	}

	return D_RET_OK;
}

void* Sector::t_GetMem()
{
	return m_pUnderLayer->ContentP() + m_pHeader->location.offset;
}

bool Sector::t_PreRenew(D_SIZE sizeOld, D_SIZE sizeNew)
{
	return m_pUnderLayer->n_RequestSectorResize(this, m_offsetContent + sizeNew);
}

bool Sector::t_Renew(D_SIZE size)
{
	return true;
}

bool Sector::t_PostRenew(D_SIZE sizeOld, D_SIZE sizeNew)
{
	return m_pUnderLayer->n_SectorResized(this);
}

bool Sector::t_Detach()
{
	if (m_pUnderLayer->GetOverLayer() == this)
	{
		// Detach from the substrate if a cascade is created
		m_pUnderLayer->SetOverLayer(nullptr);
	}

	return true;
}

bool Sector::t_Delete()
{
	// Delete the header if stored in the HeaderStore
	if (m_pHeaderStore && !m_pHeaderStore->DeleteHeader(m_id, m_indexStore))
	{
		return false;
	}

	return t_Detach();
}

bool Sector::t_Clean(D_OFFSET offsetFrom, D_SIZE size)
{
	return t_Fill(m_pUnderLayer->GetHeader()->NeedFill() ?
			m_pUnderLayer->GetHeader()->EmptyByte() :
			Settings::DEFAULT_FILLER, offsetFrom, size);
}

bool Sector::t_ReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size)
{
	return m_pUnderLayer->t_ReadFrom(pBuffer, m_pUnderLayer->GetContentOffset() +
			m_pHeader->location.offset + offset, size);
}

D_SIZE Sector::t_Read(void* pBuffer, D_SIZE size)
{
	m_pUnderLayer->SetReadOffset(m_pHeader->location.offset + m_offsetRead);

	D_SIZE sizeReaded = m_pUnderLayer->t_Read(pBuffer, size);

	if (sizeReaded != D_RET_ERROR)
	{
		m_offsetRead += sizeReaded;

		return sizeReaded;
	}

	return D_RET_ERROR;
}

bool Sector::t_WriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size)
{
	return m_pUnderLayer->t_WriteFrom(pBuffer, m_pUnderLayer->GetContentOffset() +
			m_pHeader->location.offset + offset, size);
}

bool Sector::t_Write(const void* pBuffer, D_SIZE size)
{
	m_pUnderLayer->SetWriteOffset(m_pHeader->location.offset + m_offsetWrite);

	if (m_pUnderLayer->t_Write(pBuffer, size))
	{
		m_offsetWrite += size;

		return true;
	}

	return false;
}

bool Sector::t_Copy(D_OFFSET offsetFrom, D_OFFSET offsetTo, D_SIZE size)
{
	return m_pUnderLayer->t_Copy(m_pUnderLayer->GetContentOffset() + m_pHeader->location.offset + offsetFrom,
			m_pUnderLayer->GetContentOffset() + m_pHeader->location.offset + offsetTo, size);
}

bool Sector::t_Fill(BYTE fill, D_OFFSET offset, D_SIZE size)
{
	return m_pUnderLayer->t_Fill(fill, m_pUnderLayer->GetContentOffset() +
			m_pHeader->location.offset + offset, size);
}

} /* namespace DLayers */
