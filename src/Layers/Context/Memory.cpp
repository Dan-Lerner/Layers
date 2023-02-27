/*
 * Memory.cpp
 *
 *  Created on: 17 дек. 2016 г.
 *      Author: dan
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include "Memory.h"

namespace Layers
{

namespace Contexts
{

// class Memory
///////////////////////////////////////////////////////////////////////////////////

// Terrible
const void* Memory::POSIX_ERROR = reinterpret_cast<void*>(D_RET_ERROR);

Memory::Memory()
{
	Set(0, D_RET_ERROR);
}

Memory::Memory(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

Memory::~Memory()
{

}

void Memory::Set(WORD flags, D_SECTOR_ID id)
{
	m_pMem = nullptr;
	m_sizeAttached = D_RET_ERROR;
	Context::Set(flags, id);
}

bool Memory::Reset()
{
	m_pMem = nullptr;
	m_sizeAttached = D_RET_ERROR;
	return Context::Reset();
}

bool Memory::Attached()
{
	return m_pMem != nullptr;
}

short Memory::Reattach(D_SIZE size)
{
	return t_Reattach(size);
}

short Memory::CheckLayoutCascade(bool proceedCheckChain, bool checkUnderLayer, bool checkOverLayer)
{
	short ret = CASCADE_LAYOUT_FORMER;

	// Check size (ie possible and address changing too) in memory
	if (m_sizeAttached != Size())
	{
		ret = Reattach(Size());

		if (ret == D_RET_ERROR)
		{
			return D_RET_ERROR;
		}
	}

	if (Substrate::CheckLayoutCascade(proceedCheckChain, checkUnderLayer, checkOverLayer) == D_RET_ERROR)
	{
		return D_RET_ERROR;
	}

	return ret;
}

BYTE* Memory::ContentP()
{
	return Attached() ? static_cast<BYTE*>(t_GetMem()) + m_offsetContent : nullptr;
}

void* Memory::t_GetMem()
{
	return m_pMem;
}

bool Memory::t_ReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size)
{
	::memcpy(pBuffer, static_cast<BYTE*>(m_pMem) + offset, size);

	return true;
}

bool Memory::t_WriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size)
{
	::memcpy(static_cast<BYTE*>(m_pMem) + offset, pBuffer, size);

	return true;
}

D_SIZE Memory::t_Read(void* pBuffer, D_SIZE size)
{
	::memcpy(pBuffer, static_cast<BYTE*>(m_pMem) + m_offsetRead, size);
	m_offsetRead += size;

	return size;
}

bool Memory::t_Write(const void* pBuffer, D_SIZE size)
{
	t_WriteFrom(pBuffer, m_offsetWrite, size);
	m_offsetWrite += size;

	return true;
}

bool Memory::t_Copy(D_OFFSET offsetFrom, D_OFFSET offsetTo, D_SIZE size)
{
	::memmove(static_cast<BYTE*>(m_pMem) + offsetTo, static_cast<BYTE*>(m_pMem) + offsetFrom, size);

	return true;
}

bool Memory::t_Fill(BYTE btFill, D_OFFSET offsetFrom, D_SIZE size)
{
	::memset(static_cast<BYTE*>(m_pMem) + offsetFrom, btFill, size);

	return true;
}

// class Attachable
///////////////////////////////////////////////////////////////////////////////////

Attachable::Attachable()
{
	// TODO Auto-generated constructor stub

	Set(0, D_RET_ERROR);
}

Attachable::Attachable(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

Attachable::~Attachable()
{
	// TODO Auto-generated destructor stub
}

void Attachable::Set(WORD flags, D_SECTOR_ID id)
{
	Memory::Set(flags, id);
}

bool Attachable::Reset()
{
	return Memory::Reset();
}

bool Attachable::Attach(void* pMem, D_SIZE size, Header* pParentHeader)
{
	m_header.location.Set(0, size);

	// Embedded Header initialization
	if (pParentHeader)
	{
		m_header.Set(pParentHeader);
	}

	// Attaching
	if (!Context::Attach(static_cast<const char*>(pMem), &m_header, 0))
	{
		return Reset();
	}

	m_pMem = pMem;

	return true;
}

bool Attachable::t_Attach(const char* strKey, Header* pHeader, bool lock)
{
	m_sizeAttached = pHeader->location.size;

	return true;
}

// class Heap
///////////////////////////////////////////////////////////////////////////////////

Heap::Heap()
{
	Set(0, D_RET_ERROR);
}

Heap::Heap(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

Heap::~Heap()
{
	if (m_pMem)
	{
//		Delete();
	}
}

void Heap::Set(WORD flags, D_SECTOR_ID id)
{
	Memory::Set(flags, id);
}

bool Heap::Reset()
{
	return Memory::Reset();
}

bool Heap::Attach(void* pMem, Header* pHeader)
{
	// Attaching
	if (!Context::Attach(static_cast<const char*>(pMem), pHeader))
	{
		return Reset();
	}

	m_pMem = pMem;

	return true;
}

bool Heap::t_New(const char* strKey, Header* pHeader, WORD access, bool lock)
{
	m_pMem = ::calloc(1, pHeader->location.size);
	m_sizeAttached = pHeader->location.size;

	return m_pMem != nullptr;
}

bool Heap::t_Renew(D_SIZE size)
{
	void* pNew = ::realloc(m_pMem, size);

	if (pNew)
	{
		// Header pointer shift if USE_INSIDE_HEADER
		if (m_pMem != pNew)
		{
			OffsetInsideHeader(static_cast<BYTE*>(pNew) - static_cast<BYTE*>(m_pMem));
		}

		m_pMem = pNew;
		m_sizeAttached = size;

		return true;
	}

	return false;
}

bool Heap::t_Attach(const char* strKey, Header* pHeader, bool lock)
{
	m_sizeAttached = pHeader->location.size;

	return true;
}

bool Heap::t_Delete()
{
	::free(m_pMem);

	return true;
}

// class Shared
///////////////////////////////////////////////////////////////////////////////////

Shared::Shared()
{
	Set(0, D_RET_ERROR);
}

Shared::Shared(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

Shared::~Shared()
{
	Detach();
}

void Shared::Set(WORD flags, D_SECTOR_ID id)
{
	m_MemId = D_RET_ERROR;
	Memory::Set(flags, id);
}

bool Shared::Reset()
{
	m_MemId = D_RET_ERROR;

	return Memory::Reset();
}

D_SIZE Shared::ContextSize()
{
	if (m_MemId != D_RET_ERROR)
	{
		struct stat stat;

		return (::fstat(m_MemId, &stat) != D_RET_ERROR) ? stat.st_size : D_RET_ERROR;
	}

	return D_RET_ERROR;
}

bool Shared::t_New(const char* strKey, Header* pHeader, WORD access, bool lock)
{
	return Open(strKey, pHeader->location.size, pHeader->Readonly(), true, access, lock);
}

bool Shared::t_Attach(const char* strKey, Header* pHeader, bool lock)
{
	return Open(strKey, 0, pHeader->Readonly(), false, 0, lock);
}

bool Shared::Open(const char* strKey, D_SIZE size, bool readonly, bool recreate, WORD access, bool lock)
{
	m_MemId = ::shm_open(strKey, (readonly ? O_RDONLY : O_RDWR) | (recreate ? (O_CREAT | O_TRUNC) : 0), access);

	if (m_MemId != D_RET_ERROR)
	{
		if (recreate)
		{
			if (::ftruncate(m_MemId, size) == D_RET_ERROR)
			{
				return Reset();
			}

			m_sizeAttached = size;
		}
		else
		{
			m_sizeAttached = ContextSize();
		}

		if (m_sizeAttached != D_RET_ERROR)
		{
			m_pMem = ::mmap(nullptr, m_sizeAttached, PROT_READ | (readonly ? 0 : PROT_WRITE), MAP_SHARED, m_MemId, 0);

			if (m_pMem != POSIX_ERROR)
			{
				if (!lock || ::mlock(m_pMem, m_sizeAttached) != D_RET_ERROR)
				{
					::strcpy(m_strKey, strKey);

					return true;
				}

				t_Detach();
			}

			if (recreate)
			{
				::shm_unlink(strKey);
			}
		}
	}

	return Reset();
}

bool Shared::t_Renew(D_SIZE sizeNew)
{
	if (::ftruncate(m_MemId, sizeNew) != D_RET_ERROR)
	{
		return Reattach(sizeNew) != D_RET_ERROR;
	}

	return false;
}

bool Shared::t_Detach()
{
	return ::munmap(m_pMem, m_sizeAttached) != D_RET_ERROR;
}

bool Shared::t_Delete()
{
	return t_Detach() && ::shm_unlink(m_strKey) != D_RET_ERROR;
}

short Shared::t_Reattach(D_SIZE size)
{
	// 	Not sure if mremap will always work on resize, maybe Unmap + Map is more correct

	// Experiments show that everything works even if Map hasn't changed, do not remap to a different size,
	// although it may be so until the sector size is exceeded
	// It is not clear whether it will work if the physical address of the memory block changes to m_MemId
	// Everything rests on the assumption that Map will always allocated to m_MemId when resizing

	void* pMap = m_pMem;

//	m_pMem = static_cast<BYTE*>::mremap(m_pMem, m_sizeAttached, size, m_pHeader->Unmovable() ? 0 : MREMAP_MAYMOVE));

	///////////////////////////////////////////////////////////////////////////
	// For experiment, we are testing the address change during remapping

	m_pMem = ::mmap(nullptr, size, PROT_READ | (m_pHeader->Readonly() ? 0 : PROT_WRITE), MAP_SHARED, m_MemId, 0);

	::munmap(pMap, m_sizeAttached);
	//////////////////////////////////////////////////////////////////////////

	if (m_pMem == POSIX_ERROR)
	{
		Reset();

		return D_RET_ERROR;
	}

	// Offset header addresses if USE_INSIDE_HEADER
	if (m_pMem != pMap)
	{
		OffsetInsideHeader(static_cast<BYTE*>(m_pMem) - static_cast<BYTE*>(pMap));
	}

	m_sizeAttached = size;

	return (m_pMem != pMap) ? CASCADE_LAYOUT_CHANGED : CASCADE_LAYOUT_FORMER;
}

// class SystemV
///////////////////////////////////////////////////////////////////////////////////

SystemV::SystemV()
{
	Set(0, D_RET_ERROR);
}

SystemV::SystemV(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

SystemV::~SystemV()
{
	Detach();
}

void SystemV::Set(WORD flags, D_SECTOR_ID id)
{
	m_keyMem = D_RET_ERROR;
	m_MemID = D_RET_ERROR;
	Memory::Set(flags, id);
}

bool SystemV::Reset()
{
	m_keyMem = D_RET_ERROR;
	m_MemID = D_RET_ERROR;

	return Memory::Reset();
}

D_SIZE SystemV::ContextSize()
{
	return Size();

//	if (Attached())
//	{
//		// !!! Must be tested
//		struct stat stat;
//
//		return (::fstat(m_nMemID, &stat) != D_RET_ERROR) ? stat.st_size : D_RET_ERROR;
//	}
//
//	return D_RET_ERROR;
}

bool SystemV::t_New(const char* strKey, Header* pHeader, WORD access, bool lock)
{
	m_keyMem = ::atoi(strKey);

	// Memory segment creating
	m_MemID = ::shmget(m_keyMem, pHeader->location.size, IPC_CREAT | IPC_EXCL | access);

	if (m_MemID == D_RET_ERROR)
	{
		return false;
	}

	// Attaching to created segment
	m_pMem = static_cast<BYTE*>(::shmat(m_MemID, nullptr, pHeader->Readonly() ? SHM_RDONLY : 0));

	if (m_pMem == nullptr)
	{
		// Deleting on error
		::shmctl(m_MemID, IPC_RMID, 0);

		return Reset();
	}

	return true;
}

bool SystemV::t_Attach(const char* strKey, Header* pHeader, bool lock)
{
	m_keyMem = ::atoi(strKey);

	// Gets segment ID
	m_MemID = ::shmget(m_keyMem, 0, 0);

	if (m_MemID == D_RET_ERROR)
	{
		return false;
	}

	// Attaching to segment
	m_pMem = ::shmat(m_MemID, nullptr, pHeader->Readonly() ? SHM_RDONLY : 0);

	if (m_pMem == nullptr)
	{
		return Reset();
	}

	return true;
}

bool SystemV::t_Renew(D_SIZE sizeNew)
{
	return false;
}

bool SystemV::t_Detach()
{
	return ::shmdt(m_pMem) != D_RET_ERROR;
}

bool SystemV::t_Delete()
{
	return t_Detach() && ::shmctl(m_MemID, IPC_RMID, 0) != D_RET_ERROR;
}

short SystemV::t_Reattach(D_SIZE size)
{
	return CASCADE_LAYOUT_FORMER;
}

// class DiskMap
///////////////////////////////////////////////////////////////////////////////////

DiskMap::DiskMap()
{
	Set(0, D_RET_ERROR);
}

DiskMap::DiskMap(WORD flags, D_SECTOR_ID id)
{
	Set(flags, id);
}

DiskMap::~DiskMap()
{
	Detach();
}

void DiskMap::Set(WORD flags, D_SECTOR_ID id)
{
	Memory::Set(flags, id);
}

bool DiskMap::Reset()
{
	return Memory::Reset();
}

D_SIZE DiskMap::ContextSize()
{
	return m_contextDisk.ContextSize();
}

bool DiskMap::t_New(const char* strKey, Header* pHeader, WORD access, bool lock)
{
	// At this point, pHeader->location.size has been recalculated in Context::Create to full size
	// To create a file, recalculate back to the content size
	pHeader->location.size -= CalcContentOffset(pHeader);

	// Flags synchronizing
	m_contextDisk.SetFlags(m_flags);

	// File creating
	if (pHeader->location.size > 0 && m_contextDisk.Create(strKey, pHeader, access, nullptr, lock))
	{
		// Map file to memory
		if (Map(pHeader->location.size, pHeader->Readonly(), lock))
		{
			m_sizeAttached = pHeader->location.size;

			return true;
		}

		m_contextDisk.Delete();

		Reset();
	}

	return false;
}

bool DiskMap::t_Attach(const char* strKey, Header* pHeader, bool lock)
{
	// File object attaching
	if (m_contextDisk.Attach(strKey, pHeader, nullptr, lock))
	{
		m_sizeAttached = m_contextDisk.ContextSize();

		// Map file to memory
		if (m_sizeAttached > 0 && Map(m_sizeAttached, pHeader->Readonly(), lock))
		{
			return true;
		}

		m_contextDisk.Detach();

		Reset();
	}

	return false;
}

bool DiskMap::Map(D_SIZE size, bool readonly, bool lock)
{
	// Map file to memory
	m_pMem = ::mmap(nullptr, size, PROT_READ | (readonly ? 0 : PROT_WRITE), MAP_SHARED, m_contextDisk.m_file, 0);

	if (m_pMem != POSIX_ERROR)
	{
		// Lock the memory (we prohibit its movement from RAM)
		if (!lock || ::mlock(m_pMem, size) != D_RET_ERROR)
		{
			return true;
		}

		::munmap(m_pMem, size);
	}

	return Reset();
}

bool DiskMap::t_Renew(D_SIZE sizeNew)
{
	return m_contextDisk.Renew(sizeNew - m_offsetContent, true) &&
			(Memory::Reattach(sizeNew) != D_RET_ERROR);
}

bool DiskMap::t_Detach()
{
	if (::munmap(m_pMem, m_sizeAttached) != D_RET_ERROR)
	{
		return m_contextDisk.Detach();
	}

	return false;
}

bool DiskMap::t_Delete()
{
	if (::munmap(m_pMem, m_sizeAttached) != D_RET_ERROR)
	{
		return m_contextDisk.Delete();
	}

	return false;
}

short DiskMap::t_Reattach(D_SIZE size)
{
	void* pMap = m_pMem;

	m_pMem = ::mremap(m_pMem, m_sizeAttached, size, m_pHeader->Unmovable() ? 0 : MREMAP_MAYMOVE);

	if (m_pMem == POSIX_ERROR)
	{
		m_contextDisk.Detach();

		Reset();

		return D_RET_ERROR;
	}

	// 	Offset header addresses if USE_INSIDE_HEADER
	if (m_pMem != pMap)
	{
		OffsetInsideHeader(static_cast<BYTE*>(m_pMem) - static_cast<BYTE*>(pMap));
	}

	m_sizeAttached = size;

	return m_pMem != pMap ? CASCADE_LAYOUT_CHANGED : CASCADE_LAYOUT_FORMER;
}

} /* namespace Context */

} /* namespace DLayers */
