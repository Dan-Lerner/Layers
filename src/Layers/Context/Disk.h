/*
 * Disk.h
 *
 *  Created on: 17 дек. 2016 г.
 *      Author: dan
 */

#ifndef DISC_H_
#define DISC_H_

#include "Context.h"

namespace Layers
{

namespace Contexts
{

// class Memory
// Context for filesystem

class Disk : public Context
{
public:

	Disk();
	Disk(WORD wdFLags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~Disk();

protected:

	// Fields initialization
	void Set(WORD wdFlags = 0, D_SECTOR_ID id = D_RET_ERROR);

	// Fields reset
	bool Reset();

public:

	// Sector type
	virtual short Type() { return CONTEXT_DISK; };

	// true, if sector attached
	virtual bool Attached();

	// Memory is not available
	virtual bool MemAvailable() { return false; };

	virtual D_SIZE ContextSize();

public:

	// Renames the file
	bool Rename(const char* strNewName);

// Direct memory operations are unavailable
public:

	virtual BYTE* ContentP() { return nullptr; };

// Implementation of (t_) Context
// strKey - name of file according to the filesystem
protected:

	virtual bool t_New(const char* strKey, Header* pHeader, WORD access, bool lock);
	virtual bool t_Attach(const char* strKey, Header* pHeader, bool lock);
	bool Open(const char* strKey, D_SIZE size, bool readonly, bool recreate, unsigned short access);

// Implementation of of (t_) Substrate
protected:

	virtual void* t_GetMem() { return nullptr; };
	virtual bool t_Renew(D_SIZE size);
	virtual bool t_Detach();
	virtual bool t_Delete();
	virtual bool t_ReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size);
	virtual bool t_WriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size);
	virtual D_SIZE t_Read(void* pBuffer, D_SIZE size);
	virtual bool t_Write(const void* pBuffer, D_SIZE size);
	virtual bool t_Copy(D_OFFSET offsetFrom, D_OFFSET offsetTo, D_SIZE size);
	virtual bool t_Fill(BYTE fill, D_OFFSET offsetFrom, D_SIZE size);

// System routines
protected:

	int SysReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size);
	int SysRead(void* pBuffer, D_SIZE size);
	int SysWriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size);
	int SysWrite(const void* pBuffer, D_SIZE size);
	int SysGetOffset();
	bool SysSetOffset(int pos);
	int SysSeek(int offset, int whence);

public:

	// System file ID
	int m_file;

	// Read/write offset position
	D_OFFSET m_offsetCurrent;
};

} /* namespace Context */

} /* namespace DLayers */

#endif /* DISC_H_ */
