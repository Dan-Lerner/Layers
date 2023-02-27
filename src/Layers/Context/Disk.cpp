/*
 * Disk.cpp
 *
 *  Created on: 17 дек. 2016 г.
 *      Author: dan
 */

#include <fcntl.h>
#include <alloca.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "Disk.h"

namespace Layers
{

namespace Contexts
{

Disk::Disk()
{
	// TODO Auto-generated constructor stub

	Set(0, D_RET_ERROR);
}

Disk::Disk(WORD wdFlags, D_SECTOR_ID id)
{
	Set(wdFlags, id);
}

Disk::~Disk()
{
	// TODO Auto-generated destructor stub

	Detach();
}

void Disk::Set(WORD flags, D_SECTOR_ID id)
{
	m_file = D_RET_ERROR;
	m_offsetCurrent = D_RET_ERROR;
	Context::Set(flags, id);
}

bool Disk::Reset()
{
	m_file = D_RET_ERROR;
	m_offsetCurrent = D_RET_ERROR;

	return Context::Reset();
}

bool Disk::Attached()
{
	return m_file != D_RET_ERROR;
}

D_SIZE Disk::ContextSize()
{
	if (Attached())
	{
		struct stat stat;

		return ::fstat(m_file, &stat) != D_RET_ERROR ? stat.st_size : D_RET_ERROR;
	}

	return D_RET_ERROR;
}

bool Disk::Rename(const char* strNewName)
{
	if (Detach())
	{
		if (::rename(m_strKey, strNewName) != D_RET_ERROR)
		{
			return Attach(strNewName, m_pHeader, m_pHeaderStore, false);
		}

		Attach(m_strKey, m_pHeader, m_pHeaderStore, false);
	}

	return false;
}

bool Disk::t_New(const char* strKey, Header* pHeader, WORD access, bool lock)
{
	return Open(strKey, pHeader->location.size, pHeader->Readonly(), true, access);
}

bool Disk::t_Attach(const char* strKey, Header* pHeader, bool lock)
{
	return Open(strKey, 0, pHeader->Readonly(), false, 0);
}

bool Disk::Open(const char* strKey, D_SIZE size, bool readonly, bool recreate, unsigned short access)
{
	::strcpy(m_strKey, strKey);

	m_file = ::open(strKey, (readonly ? O_RDONLY : O_RDWR) | (recreate ? (O_CREAT | O_TRUNC) : 0), access);

	if (m_file != D_RET_ERROR)
	{
		if (recreate && !t_Renew(size))
		{
			Delete();

			return false;
		}

		m_offsetCurrent = 0;

		return true;
	}

	return false;
}

bool Disk::t_Renew(D_SIZE size)
{
	BYTE byte = 0x00;

	if (m_pHeader)
	{
		return size > Size() ?
				SysWriteFrom(&byte, size - 1, 1) == 1 :
				::ftruncate(m_file, size) != D_RET_ERROR;
	}

	return SysWriteFrom(&byte, size - 1, 1) == 1;
}

bool Disk::t_Detach()
{
	if (::close(m_file) != D_RET_ERROR)
	{
		Reset();

		return true;
	}

	return false;
}

bool Disk::t_Delete()
{
	return Detach() &&
			::unlink(m_strKey) != D_RET_ERROR;
}

bool Disk::t_ReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size)
{
	return Attached() ? SysReadFrom(pBuffer, offset, size) == size : false;
}

bool Disk::t_WriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size)
{
	return Attached() ? SysWriteFrom(pBuffer, offset, size) == size : false;
}

D_SIZE Disk::t_Read(void* pBuffer, D_SIZE size)
{
	if (m_offsetCurrent == m_offsetRead || SysSetOffset(m_offsetRead))
	{
		D_SIZE sizeRead = SysRead(pBuffer, size);

		if (sizeRead != D_RET_ERROR)
		{
			m_offsetCurrent += sizeRead;
			m_offsetRead += sizeRead;

			return sizeRead;
		}
	}

	return D_RET_ERROR;
}

bool Disk::t_Write(const void* pBuffer, D_SIZE size)
{
	if (m_offsetCurrent == m_offsetWrite || SysSetOffset(m_offsetWrite))
	{
		if (SysWrite(pBuffer, size) == size)
		{
			m_offsetCurrent += size;
			m_offsetWrite += size;

			return true;
		}
	}

	return false;
}

bool Disk::t_Copy(D_OFFSET offsetFrom, D_OFFSET offsetTo, D_SIZE size)
{
	D_SIZE sizeBuffer = size > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : size;
	BYTE* pBuffer = static_cast<BYTE*>(::alloca(sizeBuffer));
	D_SIZE sizeRest = size;
	D_OFFSET offsetRead = offsetFrom > offsetTo ? offsetFrom : offsetFrom + size - sizeBuffer;
	D_OFFSET offsetWrite = offsetFrom > offsetTo ? offsetTo : offsetTo + size - sizeBuffer;
	D_SIZE sizeRead;

	do
	{
		sizeRead = SysReadFrom(pBuffer, offsetRead, sizeBuffer);

		if (sizeRead == D_RET_ERROR)
		{
			return false;
		}

		sizeRead = SysWriteFrom(pBuffer, offsetWrite, sizeRead);

		if (sizeRead == D_RET_ERROR)
		{
			return false;
		}

		sizeRest -= sizeBuffer;
		sizeBuffer = sizeRest > sizeBuffer ? sizeBuffer : sizeRest;

		if (offsetFrom > offsetTo)
		{
			offsetRead += sizeRead;
			offsetWrite += sizeRead;
		}
		else
		{
			offsetRead -= sizeBuffer;
			offsetWrite -= sizeBuffer;
		}
	}
	while (sizeRest);

	return true;
}

bool Disk::t_Fill(BYTE fill, D_OFFSET offset, D_SIZE size)
{
	D_SIZE sizeBuffer = size > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : size;
	BYTE* pBuffer = static_cast<BYTE*>(::alloca(sizeBuffer));
	::memset(pBuffer, fill, sizeBuffer);
	D_SIZE sizeRest = size;
	D_OFFSET offsetWriteMem = m_offsetWrite;
	m_offsetWrite = offset;
	D_SIZE sizeWrite;

	do
	{
		sizeWrite = sizeRest > sizeBuffer ? sizeBuffer : sizeRest;

		if (!t_Write(pBuffer, sizeWrite))
		{
			m_offsetWrite = offsetWriteMem;

			return false;
		}

		sizeRest -= sizeWrite;
	}
	while (sizeRest);

	m_offsetWrite = offsetWriteMem;

	return true;
}

int Disk::SysReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size)
{
	int ret = ::pread(m_file, pBuffer, size, offset);

	while (ret != size && ret != 0)
	{
		// Add handling when less than size bytes are returned

	}

	return ret;
}

int Disk::SysRead(void* pBuffer, D_SIZE size)
{
	int ret = ::read(m_file, pBuffer, size);

	while (ret != size && ret != 0)
	{
		// Add handling when less than size bytes are returned

	}

	return ret;
}

int Disk::SysWriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size)
{
	int ret = ::pwrite(m_file, pBuffer, size, offset);

	while (ret != size)
	{
		// Add handling when less than size bytes are returned

	}

	return ret;
}

int Disk::SysWrite(const void* pBuffer, D_SIZE size)
{
	int ret = ::write(m_file, pBuffer, size);

	while (ret != size)
	{
		// Add handling when less than size bytes are returned

	}

	return ret;
}

inline int Disk::SysGetOffset()
{
	return SysSeek(0, SEEK_CUR);
}

inline bool Disk::SysSetOffset(D_OFFSET offset)
{
	return SysSeek(m_offsetCurrent = offset, SEEK_SET) != D_RET_ERROR;
}

int Disk::SysSeek(D_OFFSET offset, int whence)
{
	return ::lseek(m_file, offset, whence);
}

} /* namespace Context */

} /* namespace DLayers */
