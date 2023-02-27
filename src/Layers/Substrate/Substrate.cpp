/*
 * Substrate.cpp
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#include <string.h>
#include <alloca.h>

#include "Substrate.h"
#include "../Sector/IHeaderStore.h"

namespace Layers
{

// class Settings
///////////////////////////////////////////////////////////////////////////////////

Settings::Settings()
{
	Set(1, SIZE_UNLIMIT, DEFAULT_FILLER, 0x0000);
}

Settings::Settings(D_SIZE sizeExpand, D_SIZE sizeMax, BYTE emptyByte, WORD flags)
{
	Set(sizeExpand, sizeMax, emptyByte, flags);
}

Settings::~Settings()
{
}

bool Settings::Set(D_SIZE sizeExpand, D_SIZE sizeMax, BYTE emptyByte, WORD flags)
{
	Settings::sizeExpand = sizeExpand;
	Settings::sizeMax = sizeMax;
	Settings::emptyByte = emptyByte;
	Settings::flags = flags;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////
// class Settings

// class State
///////////////////////////////////////////////////////////////////////////////////

State::State()
{
	attachedN = 0;
	flags = 0x0000;
}

State::~State()
{

}

///////////////////////////////////////////////////////////////////////////////////
// class State

// class SavedHeader/Header
///////////////////////////////////////////////////////////////////////////////////

SavedHeader::SavedHeader()
{
	size = sizeof(Header);
	id = D_RET_ERROR;

	Set(Settings::DEFAULT_EXPAND_SIZE,
			Settings::SIZE_UNLIMIT,
			Settings::DEFAULT_FILLER,
			Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);
}

SavedHeader::~SavedHeader()
{

}

bool SavedHeader::Set(SavedHeader* pHeader)
{
	return Set(pHeader->ExpandSize(), pHeader->MaxSize(), pHeader->EmptyByte(), pHeader->Flags());
}

bool SavedHeader::Set(D_SIZE sizeExpand, D_SIZE sizeMax, BYTE emptyByte, WORD flags)
{
	return settings.Set(sizeExpand, sizeMax, emptyByte, flags);
}

Header::Header() : SavedHeader::SavedHeader()
{
}

Header::Header(D_OFFSET offset, D_SIZE size, D_SIZE sizeExpand, D_SIZE sizeMax, BYTE emptyByte, WORD flags)
{
	id = D_RET_ERROR;
	Header::size = sizeof(Header);
	location.Set(offset, size);

	Set(sizeExpand, sizeMax, emptyByte, flags);
}

Header::~Header()
{

}

///////////////////////////////////////////////////////////////////////////////////
// class SavedHeader/Header

// class Substrate
///////////////////////////////////////////////////////////////////////////////////

Substrate::Substrate()
{
	Set(0x0000, D_RET_ERROR);
}

Substrate::Substrate(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

Substrate::~Substrate()
{
}

void Substrate::Set(WORD flags, D_SECTOR_ID id)
{
	m_id = id;
	m_pHeader = nullptr;
	m_pOverLayer = nullptr;
	m_offsetContent = D_RET_ERROR;
	m_offsetRead = D_RET_ERROR;
	m_offsetWrite = D_RET_ERROR;
	m_indexStore = D_RET_ERROR;
	m_pHeaderStore = nullptr;
	m_flags = flags;
}

bool Substrate::Reset()
{
	Set(m_flags, m_id);

	return false;
}

bool Substrate::InitCreate(Header* pHeader)
{
	if (!pHeader)
	{
		return Reset();
	}

	m_pHeader = pHeader;

	// Primary initialization
	if (!t_CorrectHeader(m_pHeader) || !Init())
	{
		return Reset();
	}

	// SavedHeader saving if any
	if (IsSavedHeader() && !UpdateSavedHeader())
	{
		return Reset();
	}

	// If SavedHeader used as primary header then we must place it in memory
	if (CanUseInsideHeader() && !t_WriteFrom(pHeader, 0, pHeader->Size()))
	{
		return false;
	}

	// Content initialization
	if (m_pHeader->NeedFill())
	{
		Location location(0, ContentSize());
		Fill(location);
	}

	// Initializes header after sector creation
	m_pHeader->state.Create();

	// Service pointers initialization
	return InitPointers();
}

bool Substrate::InitAttach(Header* pHeader)
{
	// pHeader == nullptr is unacceptable
	// even if USE_INSIDE_HEADER is set
	// It is due to pHeader->location.offset must be set for t_GetMem()
	if (!pHeader)
	{
		return false;
	}

	m_pHeader = pHeader;

	if (CanUseInsideHeader())
	{
		m_pHeader = static_cast<Header*>(t_GetMem());
	}

	if (!m_pHeader)
	{
		return Reset();
	}

	// Initialization of outer header from saved header
	if (!InitFromSavedHeader())
	{
		return Reset();
	}

	// Common initialization
	if (!Init())
	{
		return Reset();
	}

	// Service pointers initialization
	return InitPointers();
}

bool Substrate::Init()
{
	m_offsetContent = CalcContentOffset(m_pHeader);
	m_offsetRead = m_offsetContent;
	m_offsetWrite = m_offsetContent;
	m_pHeader->state.Attach();

	return true;
}

D_SIZE Substrate::CalcSectorSize(Header* pHeader)
{
	return CalcContentOffset(pHeader) + CorrectContentSize(pHeader->location.size, pHeader);
}

D_OFFSET Substrate::CalcContentOffset(Header* pHeader)
{
	if (FlagUseInsideHeader())
	{
		return pHeader->Size();
	}

	if (IsSavedHeader())
	{
		return t_SavedHeaderSize();
	}

	return 0;
}

D_SIZE Substrate::CorrectContentSize(D_SIZE sizeDemand, Header* pHeader)
{
	D_SIZE sizeCorrect = sizeDemand / pHeader->ExpandSize() * pHeader->ExpandSize();

	if (sizeDemand > sizeCorrect || sizeCorrect == 0)
	{
		sizeCorrect += pHeader->ExpandSize();
	}

	return sizeCorrect;
}

bool Substrate::Delete()
{
	if (Attached())
	{
		m_pHeader->state.Detach();
		m_pHeader->state.Delete();

		if (m_pHeader->NeedClean())
		{
			t_Clean(0, Size());
		}

		if (t_Delete())
		{
			Reset();

			return true;
		}
	}

	return false;
}

bool Substrate::Detach()
{
	if (Attached())
	{
		m_pHeader->state.Detach();

		if (t_Detach())
		{
			Reset();

			return true;
		}
	}

	return false;
}

bool Substrate::SetContent(void* pContent, D_SIZE size)
{
	if (CorrectContentSize(size, m_pHeader) != ContentSize() && !Renew(size))
	{
		return false;
	}

	Location location(0, size);

	return SetFragment(pContent, location);
}

bool Substrate::CopyContent(Substrate* pSector, D_SIZE sizeMaxBuffer)
{
	Location location(0, pSector->ContentSize());

	return Renew(location.size) && CopyFragment(pSector, location, 0, sizeMaxBuffer);
}

bool Substrate::GetContent(void* pContent)
{
	Location location(0, ContentSize());

	return GetFragment(pContent, location);
}

bool Substrate::GetContentOptimal(void** ppContent)
{
	Location location(0, ContentSize());

	return GetFragmentOptimal(ppContent, location);
}

bool Substrate::ClearContent()
{
	if (!Attached())
	{
		return false;
	}

	if (m_pHeader->NeedFill())
	{
		Location location(0, ContentSize());
		Fill(location);
	}

	if (!t_ClearContent())
	{
		return false;
	}

	m_offsetRead = m_offsetContent;
	m_offsetWrite = m_offsetContent;

	return UpdateSavedHeader();
}

bool Substrate::DeleteContent()
{
	return Renew(0) &&
			ClearContent() &&
			t_DeleteContent();
}

bool Substrate::Clean(Location& location)
{
	return Attached() &&
			BlockInside(location) &&
			t_Clean(m_offsetContent + location.offset, location.size);
}

bool Substrate::SetSettings(Settings* pSettings)
{
	return m_pHeader->Set(pSettings->ExpandSize(), pSettings->MaxSize(), pSettings->EmptyByte(), pSettings->Flags()) &&
			t_SettingsChanged() &&
			UpdateSavedHeader();
}

bool Substrate::UpdateSavedHeader()
{
	if (IsSavedHeader())
	{
		D_SIZE sizeSaved = t_SavedHeaderSize();

		if (sizeSaved)
		{
			void* pSavedHeader = ::alloca(sizeSaved);

			return t_InitSavedHeader(pSavedHeader) &&
					t_WriteFrom(pSavedHeader, 0, sizeSaved);
		}
	}

	return true;
}

bool Substrate::ReadSavedHeader(void* pHeader, D_SIZE size)
{
	return t_ReadFrom(pHeader, 0, size);
}

bool Substrate::InitFromSavedHeader()
{
	if (IsSavedHeader())
	{
		D_SIZE sizeSaved = t_SavedHeaderSize();

		if (sizeSaved)
		{
			void* pSavedHeader = ::alloca(sizeSaved);

			return ReadSavedHeader(pSavedHeader, sizeSaved) &&
					t_InitFromSavedHeader(pSavedHeader);
		}
	}

	return true;
}

bool Substrate::SetOffset(D_OFFSET offset)
{
	m_pHeader->location.Move(offset);

	return UpdateSavedHeader();
}

bool Substrate::SetSize(D_SIZE size)
{
	m_pHeader->location.Resize(size);

	return UpdateSavedHeader();
}

bool Substrate::UpdateContentOffset()
{
	D_OFFSET offsetOld = m_offsetContent;
	D_SIZE sizeContent = ContentSize();
	m_offsetContent = CalcContentOffset(m_pHeader);

	if (offsetOld != m_offsetContent)
	{
		if (!t_Copy(offsetOld, m_offsetContent, sizeContent))
		{
			return false;
		}

		if (m_offsetContent < offsetOld && !Renew(sizeContent, true))
		{
			return false;
		}
	}

	return UpdateSavedHeader();
}

bool Substrate::IsSavedHeader()
{
	// The case when USE_INSIDE_HEADER and not MemAvailable() treated as FLAG_SAVED_HEADER
	return !CanUseInsideHeader() && ((m_flags & FLAG_SAVED_HEADER) || FlagUseInsideHeader());
}

bool Substrate::CanUseInsideHeader()
{
	return MemAvailable() && (m_flags & USE_INSIDE_HEADER);
}

bool Substrate::Renew(D_SIZE sizeContent, bool exactBytes)
{
	if (!Attached() || !m_pHeader->Resizable())
	{
		return false;
	}

	if (sizeContent == ContentSize())
	{
		return t_PostRenew(sizeContent, sizeContent);
	}

	// Correction of demanded size
	D_SIZE sizeCorrect = exactBytes ? sizeContent : CorrectContentSize(sizeContent, m_pHeader);

	// Check if size is exceeded
	if (!m_pHeader->UnlimitSize() && sizeCorrect > m_pHeader->MaxSize())
	{
		return false;
	}

	D_SIZE sizeContentOld = ContentSize();

	// Typed initialization before size changing
	if (!t_PreRenew(sizeContentOld, sizeCorrect))
	{
		return false;
	}

	// Clearing empty space if the sector is reduced
	if (m_pHeader->NeedClean() && sizeCorrect < sizeContentOld)
	{
		Location location(sizeCorrect, sizeContentOld - sizeCorrect);
		Clean(location);
	}

	// Typed size changing
	if (!t_Renew(m_offsetContent + sizeCorrect))
	{
		return false;
	}

	// Sets the new size value
	if (!SetSize(m_offsetContent + sizeCorrect))
	{
		return false;
	}

	// Filling a new area if size is increased
	if (sizeCorrect > sizeContentOld && m_pHeader->NeedFill())
	{
		Location location(sizeContentOld, sizeCorrect - sizeContentOld);
		Fill(location);
	}

	// Typed initialization after size changing
	if (!t_PostRenew(sizeContentOld, sizeCorrect))
	{
		return false;
	}

	return true;
}

bool Substrate::Expand(D_SIZE sizeExpand)
{
	return Renew(ContentSize() + sizeExpand);
}

void* Substrate::GetMem()
{
	return MemAvailable() ? t_GetMem() : nullptr;
}

bool Substrate::InitPointers()
{
	if (CanUseInsideHeader())
	{
		m_pHeader = static_cast<Header*>(t_GetMem());
	}

	return true;
}

void Substrate::OffsetHeaderP(D_OFFSET offset)
{
	if (Attached() && CanUseInsideHeader())
	{
		m_pHeader = reinterpret_cast<Header*>(reinterpret_cast<BYTE*>(m_pHeader) + offset);
	}
}

bool Substrate::Readonly()
{
	return m_pHeader->Readonly() || (m_flags & FLAG_READONLY);
}

bool Substrate::ReadFrom(void* pBuffer, Location& location)
{
	return Attached() &&
			BlockInside(location) &&
			t_ReadFrom(pBuffer, m_offsetContent + location.offset, location.size);
}

bool Substrate::WriteFrom(const void* pBuffer, Location& location)
{
	return Attached() &&
			!Readonly() &&
			RequestSpace(location) == location.size &&
			t_WriteFrom(pBuffer, m_offsetContent + location.offset, location.size);
}

D_SIZE Substrate::Read(void* pBuffer, D_SIZE size)
{
	if (Attached())
	{
		Location location(GetReadOffset(), size);
		D_SIZE sizeCorrect = GetSizeInside(location);

		if (sizeCorrect > 0 && t_Read(pBuffer, sizeCorrect) == sizeCorrect)
		{
			return sizeCorrect;
		}
	}

	return D_RET_ERROR;
}

D_OFFSET Substrate::GetReadOffset()
{
	return m_offsetRead - m_offsetContent;
}

bool Substrate::SetReadOffset(D_OFFSET offset)
{
	if (offset < 0)
	{
		return false;
	}

	m_offsetRead = m_offsetContent + offset;

	return true;
}

bool Substrate::Write(const void* pBuffer, D_SIZE size)
{
	Location location(GetWriteOffset(), size);

	return Attached() &&
			!Readonly() &&
			RequestSpace(location) == size &&
			t_Write(pBuffer, size);
}

bool Substrate::WriteEnd(const void* pBuffer, D_SIZE size)
{
	return SetWriteOffset(ContentSize()) &&
			Write(pBuffer, size);
}

D_OFFSET Substrate::GetWriteOffset()
{
	return m_offsetWrite - m_offsetContent;
}

bool Substrate::SetWriteOffset(D_OFFSET offset)
{
	if (offset < 0)
	{
		return false;
	}

	m_offsetWrite = m_offsetContent + offset;

	return true;
}

bool Substrate::GetFragment(void* pFragment, Location& location)
{
	return ReadFrom(pFragment, location);
}

bool Substrate::GetFragmentOptimal(void** ppFragment, Location& location)
{
	if (MemAvailable())
	{
		// In this case pointer is lost, so *ppFragment must be allocated in stack
		*ppFragment = ContentP() + location.offset;

		return true;
	}

	return GetFragment(*ppFragment, location);
}

bool Substrate::SetFragment(void* pFragment, Location& location)
{
	return WriteFrom(pFragment, location);
}

bool Substrate::CopyFragment(Substrate* pSector, Location& locationFrom, D_OFFSET offsetTo, D_SIZE sizeMaxBuffer)
{
	Location locationTo(offsetTo, locationFrom.size);

	D_SIZE sizeRest = RequestSpace(locationTo);

	if (sizeRest != locationFrom.size)
	{
		return false;
	}

	bool ret = false;

	if (MemAvailable())
	{
		// If this sector in memory then perform direct copy to memory
		ret = pSector->t_ReadFrom(ContentP() + offsetTo, pSector->m_offsetContent + locationFrom.offset, locationFrom.size);
	}
	else if (pSector->MemAvailable())
	{
		// If source sector placed in memory then perform reading from memory
		ret = t_WriteFrom(pSector->ContentP() + locationFrom.offset, m_offsetContent + locationTo.offset, locationTo.size);
	}
	else
	{
		// Both of sectors are not in memory - buffer is used

		D_SIZE sizeBuffer = sizeRest > sizeMaxBuffer ? sizeMaxBuffer : sizeRest;
		void* buffer = ::alloca(sizeBuffer);
		D_SIZE sizeToWrite;
		pSector->SetReadOffset(locationFrom.offset);
		SetWriteOffset(offsetTo);

		do
		{
			sizeToWrite = sizeRest > sizeBuffer ? sizeBuffer : sizeRest;
			ret = pSector->t_Read(buffer, sizeToWrite) == sizeToWrite && t_Write(buffer, sizeToWrite);
			sizeRest -= sizeToWrite;
		}
		while(sizeRest && ret);
	}

	return ret;
}

bool Substrate::CompareFragment(void* pFragment, D_SIZE size, Location& locationThis, D_SIZE sizeMaxBuffer)
{
	if (size != locationThis.size)
	{
		return false;
	}

	if (MemAvailable())
	{
		// If this sector in memory then perform memory comparison
		return 0 == ::memcmp(pFragment, ContentP() + locationThis.offset, size);
	}

	// Using of intermediate buffer

	D_SIZE sizeBuffer = locationThis.size > sizeMaxBuffer ? sizeMaxBuffer : locationThis.size;
	void* buffer = ::alloca(sizeBuffer);
	Location location = locationThis;
	D_SIZE sizeRest = locationThis.RightMargin() - location.offset;

	while (sizeRest)
	{
		location.size = sizeRest > sizeBuffer ? sizeBuffer : sizeRest;

		if (!t_ReadFrom(buffer, m_offsetContent + location.offset, location.size) ||
				0 != ::memcmp(buffer, static_cast<BYTE*>(pFragment) + (location.offset - locationThis.offset), location.size))
		{
			return false;
		}

		location.offset += location.size;
		sizeRest = locationThis.RightMargin() - location.offset;
	}

	return true;
}

bool Substrate::CompareFragment(Substrate* pSector, Location& location, Location& locationThis, D_SIZE sizeMaxBuffer)
{
	if (location.size != locationThis.size)
	{
		return false;
	}

	if (pSector->MemAvailable())
	{
		return CompareFragment(pSector->ContentP() + location.offset, location.size, locationThis);
	}

	if (MemAvailable())
	{
		return pSector->CompareFragment(ContentP() + locationThis.offset, locationThis.size, location);
	}

	D_SIZE sizeBuffer = location.size > sizeMaxBuffer ? sizeMaxBuffer : location.size;

	void* buffer1 = ::alloca(sizeBuffer);
	void* buffer2 = ::alloca(sizeBuffer);

	D_OFFSET offset1 = location.offset;
	D_OFFSET offset2 = locationThis.offset;
	D_SIZE size;
	D_SIZE sizeRest = location.RightMargin() - offset1;

	while (sizeRest)
	{
		size = sizeRest > sizeBuffer ? sizeBuffer : sizeRest;

		if (!pSector->t_ReadFrom(buffer1, pSector->m_offsetContent + offset1, size) ||
				!t_ReadFrom(buffer2, m_offsetContent + offset2, size) ||
				0 != ::memcmp(buffer1, buffer2, size))
		{
			return false;
		}

		offset1 += size;
		offset2 += size;
		sizeRest = location.RightMargin() - offset1;
	}

	return true;
}

D_OFFSET Substrate::FindFragment(void* pFragment, D_SIZE size, D_SIZE sizeMaxBuffer)
{
	if (MemAvailable())
	{
		BYTE* pEnd = ContentP() + ContentSize() - size + 1;

		for (BYTE* pMem = ContentP(); pMem < pEnd; pMem++)
		{
			if (0 == ::memcmp(pMem, pFragment, size))
			{
				return (D_OFFSET)(pMem - ContentP());
			}
		}
	}
	else
	{
		Location locationBuffer(ContentSize(), sizeMaxBuffer > ContentSize() ? ContentSize() : sizeMaxBuffer);
		BYTE* buffer = static_cast<BYTE*>(::alloca(locationBuffer.size));
		D_OFFSET offsetEnd = ContentSize() - size;

		for (D_OFFSET offset = 0; offset < offsetEnd; offset++)
		{
			D_OFFSET offsetCompare;
			D_OFFSET offsetEndCompare = offset + size;

			for (offsetCompare = offset; offsetCompare < offsetEndCompare; offsetCompare++)
			{
				if (!PositionInLocation(offsetCompare, locationBuffer))
				{
					// If limits of buffer exceeded then reread buffer from required position
					locationBuffer.Set(offsetCompare, (locationBuffer.size < ContentSize() - offsetCompare) ?
							locationBuffer.size : ContentSize() - offsetCompare);

					if (!t_ReadFrom(buffer, m_offsetContent + locationBuffer.offset, locationBuffer.size))
					{
						return D_RET_ERROR;
					}
				}

				if (static_cast<BYTE*>(pFragment)[offsetCompare - offset] != buffer[offsetCompare - locationBuffer.offset])
				{
					break;
				}
			}

			if (offsetCompare == offsetEndCompare)
			{
				return offset;
			}
		}
	}

	return D_RET_ERROR;
}

bool Substrate::RequestCopy(Location& location, D_OFFSET offsetTo)
{
	Location locationTo(offsetTo, location.size);

	return Attached() &&
			BlockInside(location) &&
			RequestSpace(locationTo) == locationTo.size;
}

bool Substrate::RequestMove(Location& location, D_OFFSET offsetTo)
{
	return RequestCopy(location, offsetTo);
}

bool Substrate::RequestShift(Location& location, D_OFFSET offsetTo)
{
	Location locationTo(offsetTo, location.size);

	return Attached() &&
			BlockInside(location) &&
			BlockInside(locationTo);
}

bool Substrate::RequestExchange(Location& location, D_OFFSET offsetTo)
{
	Location locationTo(offsetTo, location.size);

	return Attached() &&
			BlockInside(location) &&
			BlockInside(locationTo) &&
			!FragmentsOverlapped(location, locationTo);
}

bool Substrate::RequestInsertPlace(Location& location)
{
	return Attached() &&
			BlockInside(location);
}

bool Substrate::RequestInsertFill(Location& location)
{
	return Attached() &&
			BlockInside(location);
}

bool Substrate::RequestRemove(Location& location)
{
	return Attached() &&
			BlockInside(location);
}

bool Substrate::RequestFill(Location& location)
{
	return Attached() &&
			BlockInside(location);
}

bool Substrate::ProcessCopy(Location& location, D_OFFSET offsetTo)
{
	return t_Copy(m_offsetContent + location.offset, m_offsetContent + offsetTo, location.size);
}

bool Substrate::ProcessMove(Location& location, D_OFFSET offsetTo)
{
	if (!t_Copy(m_offsetContent + location.offset, m_offsetContent + offsetTo, location.size))
	{
		return false;
	}

	if (m_pHeader->NeedFill())
	{
		Location locationFree;
		GetFreeAfterMoving(location, offsetTo, locationFree);
		t_Fill(m_pHeader->EmptyByte(), m_offsetContent + locationFree.offset, locationFree.size);
	}

	return true;
}

bool Substrate::ProcessShift(Location& location, D_OFFSET offsetTo)
{
	// This part must be changed to shift the ring buffer through an intermediate buffer

	// As long as everything is simplified
	//
	//		|  Size |    ****    |
	//		|    ****    |  Size |
	//	   From          To

	//		|    Size   |  ****  |
	//		|  ****  |    Size   |
	//	   From      To

	//		|    ****    |  Size |
	//		|  Size |    ****    |
	//	    To          From

	D_OFFSET offset = m_offsetContent + (offsetTo > location.offset ? location.offset : offsetTo);
	D_SIZE size = offsetTo > location.offset ? location.size : location.offset - offsetTo;
	D_OFFSET offsetTarget = m_offsetContent + (offsetTo > location.offset ? offsetTo : offsetTo + location.size);

	void* buffer = ::alloca(size);

	return t_ReadFrom(buffer, offset, size) &&
			t_Copy(offset + size, offset, offsetTarget - offset) &&
			t_WriteFrom(buffer, offsetTarget, size);
}

bool Substrate::ProcessExchange(Location& location, D_OFFSET offsetTo, D_SIZE sizeMaxBuffer)
{
	D_SIZE sizeBuffer = location.size > sizeMaxBuffer ? sizeMaxBuffer : location.size;

	void* buffer = ::alloca(sizeBuffer);

	D_SIZE sizeRest = location.size;
	D_OFFSET offsetFromPos;
	D_OFFSET offsetToPos;
	D_SIZE size;

	do
	{
		size = sizeRest > sizeBuffer ? sizeBuffer : sizeRest;
		offsetFromPos = m_offsetContent + location.RightMargin() - sizeRest;
		offsetToPos = m_offsetContent + offsetTo + location.size - sizeRest;

		if (!t_ReadFrom(buffer, offsetFromPos, size) ||
				!t_Copy(offsetToPos, offsetFromPos, size) ||
				!t_WriteFrom(buffer, offsetToPos, size))
		{
			return false;
		}

		sizeRest -= size;
	}
	while (sizeRest);

	return true;
}

bool Substrate::ProcessInsertPlace(Location& location)
{
	D_OFFSET offsetFrom = m_offsetContent + location.offset;

	if (!t_Copy(offsetFrom, offsetFrom + location.size, ContentSize() - location.RightMargin()))
	{
		return false;
	}

	if (m_pHeader->NeedFill())
	{
		t_Fill(m_pHeader->EmptyByte(), offsetFrom, location.size);
	}

	return true;
}

bool Substrate::ProcessInsertFill(BYTE btFill, Location& location)
{
	D_OFFSET offsetFrom = m_offsetContent + location.offset;

	if (!t_Copy(offsetFrom, offsetFrom + location.size, ContentSize() - location.RightMargin()))
	{
		return false;
	}

	t_Fill(btFill, offsetFrom, location.size);

	return true;
}

bool Substrate::ProcessRemove(Location& location)
{
	D_OFFSET offsetFrom = m_offsetContent + location.offset;

	if (!t_Copy(offsetFrom + location.size, offsetFrom, ContentSize() - location.RightMargin()))
	{
		return false;
	}

	if (m_pHeader->NeedFill())
	{
		t_Fill(m_pHeader->EmptyByte(), Size() - location.size, location.size);
	}

	return true;
}

bool Substrate::ProcessFill(BYTE btFill, Location& location)
{
	return t_Fill(btFill, m_offsetContent + location.offset, location.size);
}

bool Substrate::Copy(Location& location, D_OFFSET offsetTo)
{
	return RequestCopy(location, offsetTo) &&
			ProcessCopy(location, offsetTo);
}

bool Substrate::Move(Location& location, D_OFFSET offsetTo)
{
	return RequestMove(location, offsetTo) &&
			ProcessMove(location, offsetTo);
}

bool Substrate::Shift(Location& location, D_OFFSET offsetTo)
{
	return RequestShift(location, offsetTo) &&
			ProcessShift(location, offsetTo);
}

bool Substrate::Exchange(Location& location, D_OFFSET offsetTo)
{
	return RequestExchange(location, offsetTo) &&
			ProcessExchange(location, offsetTo);
}

bool Substrate::InsertPlace(Location& location)
{
	return RequestInsertPlace(location) &&
			ProcessInsertPlace(location);
}

bool Substrate::InsertFill(BYTE btFill, Location& location)
{
	return RequestInsertFill(location) &&
			ProcessInsertFill(btFill, location);
}

bool Substrate::Remove(Location& location)
{
	return RequestRemove(location) &&
			ProcessRemove(location);
}

bool Substrate::Fill(BYTE btFill, Location& location)
{
	return RequestFill(location) &&
			ProcessFill(btFill, location);
}

bool Substrate::Fill(Location& location)
{
	return Fill(m_pHeader->EmptyByte(), location);
}

bool Substrate::Defrag()
{
	int nFragStat;
	while ((nFragStat = DefragQuantum()) != SECTOR_FRAG_COMPLETED && nFragStat != D_RET_ERROR);

	return nFragStat != D_RET_ERROR;
}

short Substrate::CheckLayoutCascade(bool proceedCheckChain, bool checkUnderLayer, bool checkOverLayer)
{
	// Check positions of overlayers
	if (checkOverLayer && m_pOverLayer)
	{
		if (m_pOverLayer->CheckLayoutCascade(true, false, true) == D_RET_ERROR)
		{
			return D_RET_ERROR;
		}
	}

	// Set Header's pointer if it placed in HeaderStore
	if (CheckHeaderStore() == D_RET_ERROR)
	{
		return D_RET_ERROR;
	}

	return CASCADE_LAYOUT_FORMER;
}

short Substrate::CheckHeaderStore()
{
	// Set Header's pointer if it placed in HeaderStore
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

void Substrate::UpdatePointersCascade(bool proceedChain)
{
	if (MemAvailable())
	{
		InitPointers();

		if (m_pOverLayer)
		{
			m_pOverLayer->UpdatePointersCascade(true);
		}
	}
}

void Substrate::OffsetInsideHeaderCascade(D_OFFSET offset, bool proceedChain)
{
	if (MemAvailable())
	{
		OffsetHeaderP(offset);

		if (m_pOverLayer)
		{
			m_pOverLayer->OffsetInsideHeaderCascade(offset, true);
		}
	}
}

bool Substrate::CollapseCascade(bool proceedChain)
{
	if (Attached())
	{
		return m_pOverLayer ? m_pOverLayer->CollapseCascade(true) : true;
	}

	return false;
}

bool Substrate::CanDefragCascade(bool proceedChain)
{
	return (Attached() && m_pOverLayer) ? m_pOverLayer->CanDefragCascade(true) : false;
}

int Substrate::DefragQuantumCascade(bool proceedChain, bool calcRemain)
{
	// Return value - remain amount of fragmented elements
	// (the number of fragmented elements before the step)
	// SECTOR_FRAG_COMPLETED, if defrag is finished

	if (Attached())
	{
		return m_pOverLayer ?
				m_pOverLayer->DefragQuantumCascade(true, calcRemain) :
				SECTOR_FRAG_COMPLETED;
	}

	return D_RET_ERROR;
}

bool Substrate::StartSessionCascade(bool proceedChain)
{
	if (Attached())
	{
		return m_pOverLayer ? m_pOverLayer->StartSessionCascade(true) : true;
	}

	return false;
}

bool Substrate::CloseSessionCascade(bool proceedChain)
{
	if (Attached())
	{
		return m_pOverLayer ? m_pOverLayer->CloseSessionCascade(true) : true;
	}

	return false;
}

bool Substrate::BlockInside(Location& location)
{
	return ContentSize() >= location.RightMargin();
}

bool Substrate::PositionInLocation(D_OFFSET offset, Location location)
{
	return offset >= location.offset && offset < location.RightMargin();
}

D_SIZE Substrate::GetSizeInside(Location& location)
{
	return BlockInside(location) ?
			location.size :
			(ContentSize() - location.offset > 0 ? ContentSize() - location.offset : D_RET_ERROR);
}

D_SIZE Substrate::RequestSpace(Location& location)
{
	if (BlockInside(location))
	{
		return location.size;
	}

	if (m_pHeader->Resizable())
	{
		return Renew(location.RightMargin()) ? location.size : D_RET_ERROR;
	}

	return GetSizeInside(location);
}

bool Substrate::FragmentsOverlapped(Location& location1, Location& location2)
{
	return location1.offset < location2.RightMargin() &&
			location2.offset < location1.RightMargin();
}

bool Substrate::GetDeletedAfterMoving(Location& location, D_OFFSET offsetTo, Location& locationDeleted)
{
	locationDeleted.Set(offsetTo, location.size);

	if (location.offset > offsetTo)
	{
		if (location.offset - offsetTo < location.size)
		{
			locationDeleted.size = location.offset - offsetTo;
		}
	}
	else
	{
		if (offsetTo - location.offset < location.size)
		{
			locationDeleted.offset = location.RightMargin();
			locationDeleted.size = offsetTo - location.offset;
		}
	}

	return true;
}

bool Substrate::GetFreeAfterMoving(Location& location, D_OFFSET offsetTo, Location& locationFree)
{
	locationFree = location;

	if (location.offset > offsetTo)
	{
		if (location.offset - offsetTo < location.size)
		{
			locationFree.offset = offsetTo + location.size;
			locationFree.size = location.offset - offsetTo;
		}
	}
	else
	{
		if (offsetTo - location.offset < location.size)
		{
			locationFree.size = offsetTo - location.offset;
		}
	}

	return true;
}

Header* Substrate::RequestHeader(IHeaderStore* pHeaderStore, D_SECTOR_ID id, Header* pHeader)
{
	m_indexStore = D_RET_ERROR;

	// Trying to find Header
	Header* pUsedHeader = pHeaderStore->Find(id, m_indexStore);

	// If Header is not found then new Header is requested
	if (!pUsedHeader)
	{
		if (!pHeader)
		{
			return nullptr;
		}

		pHeader->SetID(id);
		m_indexStore = pHeaderStore->Request(pHeader);
		pUsedHeader = pHeaderStore->Find(id, m_indexStore);

		if (!pUsedHeader)
		{
			return nullptr;
		}

		::memcpy(pUsedHeader, pHeader, pHeader->Size());
	}

	m_pHeaderStore = pHeaderStore;

	return pUsedHeader;
}

bool Substrate::n_RequestSectorResize(Substrate* pSector, D_SIZE sizeNew)
{
	if (sizeNew > pSector->m_pHeader->location.size)
	{
		Location location(pSector->m_pHeader->location.offset, sizeNew);

		return RequestSpace(location);
	}

	return true;
}

bool Substrate::t_InitSavedHeader(void* pSaveHeader)
{
	::memcpy(pSaveHeader, m_pHeader, t_SavedHeaderSize());

	return true;
}

bool Substrate::t_InitFromSavedHeader(void* pSaveHeader)
{
	::memcpy(m_pHeader, pSaveHeader, t_SavedHeaderSize());

	return true;
};

} /* namespace DLayers */
