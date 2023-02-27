/*
 * Context.cpp
 *
 *  Created on: 17 июля 2016 г.
 *      Author: dan
 */

#include <string.h>

#include "Context.h"
#include "../Sector/IHeaderStore.h"

namespace Layers
{

namespace Contexts
{

// class Context
///////////////////////////////////////////////////////////////////////////////////

Context::Context()
{
	// TODO Auto-generated constructor stub

	Set(0, D_RET_ERROR);
}

Context::Context(WORD flags, D_SECTOR_ID id)
{
	// TODO Auto-generated constructor stub

	Set(flags, id);
}

Context::~Context()
{
	// TODO Auto-generated destructor stub
}

void Context::Set(WORD flags, D_SECTOR_ID id)
{
	Substrate::Set(flags, id);
}

bool Context::Reset()
{
	return Substrate::Reset();
}

bool Context::Create(const char* strKey, Header* pHeader, WORD access, IHeaderStore* pHeaderStore, bool lock)
{
	if (Attached())
	{
		return false;
	}

	// Used Header
	Header* pUsedHeader = pHeaderStore ?
			(Header*)RequestHeader(pHeaderStore, m_id, pHeader) :
			pHeader;

	if (!pUsedHeader)
	{
		return false;
	}

	// Converting content size to sector size
	pUsedHeader->location.Set(0, CalcSectorSize(pUsedHeader));

	if (t_New(strKey, pUsedHeader, access, lock) && InitCreate(pUsedHeader))
	{
		m_pHeaderStore = pHeaderStore;

		return true;
	}

	Reset();

	return false;
}

bool Context::Attach(const char* strKey, Header* pHeader, IHeaderStore* pHeaderStore, bool lock)
{
	Header header;
	Header* pUsedHeader;

	// For a low-level opening of a context, pUsedHeader must be fully initialized

	if (CanUseInsideHeader())
	{
		// If USE_INSIDE_HEADER, read header from context

		pUsedHeader = &header;

		if (!ReadHeader(strKey, pUsedHeader))
		{
			return false;
		}
	}
	else if (pHeaderStore)
	{
		// Trying to find an existing header in HeaderStore or create a new one based on pHeader

		pUsedHeader = static_cast<Header*>(RequestHeader(pHeaderStore, m_id, pHeader));

		if (!pUsedHeader && IsSavedHeader())
		{
			// If not found in pHeaderStore, pHeader is not set and there is a saved header, read from saved
			if (!ReadHeader(strKey, &header))
			{
				return false;
			}

			pUsedHeader = static_cast<Header*>(RequestHeader(pHeaderStore, m_id, &header));
		}
	}
	else
	{
		pUsedHeader = pHeader;

		// If there is a saved header, read its contents from the context
		if (IsSavedHeader() && !ReadHeader(strKey, pUsedHeader))
		{
			return false;
		}
	}

	// Attaching
	if (pUsedHeader && !Attached() && t_Attach(strKey, pUsedHeader, lock) && InitAttach(pUsedHeader))
	{
		m_pHeader->location.Set(0, ContextSize());

		return true;
	}

	Reset();

	return false;
}

bool Context::ReadHeader(const char* strKey, Header* pHeader)
{
	// Temporarily attach an object to the header and read it

	pHeader->location.Set(0, t_SavedHeaderSize());

	if (!t_Attach(strKey, pHeader, false))
	{
		return false;
	}

	if (!t_ReadFrom(pHeader, 0, t_SavedHeaderSize()))
	{
		return false;
	}

	if (!t_Detach())
	{
		return Reset();
	}

	Reset();

	return true;
}

bool Context::t_Clean(D_OFFSET offset, D_SIZE size)
{
	return t_Fill(0x00, offset, size) &&
			t_Fill(0xFF, offset, size);
}

///////////////////////////////////////////////////////////////////////////////////
// class Context

} /* namespace Context */

} /* namespace DLayers */

