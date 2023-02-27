/*
 * Memory.h
 *
 *  Created on: 17 дек. 2016 г.
 *      Author: dan
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "Context.h"
#include "Disk.h"

namespace Layers
{

namespace Contexts
{

// class Memory
// Base class for all inmemory contexts

class Memory : public Context
{
protected:

	// For POSIX api
	static const void* POSIX_ERROR;

public:

	Memory();
	Memory(WORD flags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~Memory();

protected:

	// Fields initialization
	void Set(WORD wdFlags = 0, D_SECTOR_ID id = D_RET_ERROR);

	// Fields reset
	bool Reset();

public:

	// Sector type
	virtual short Type() { return MEM_CONTEXT; };

	// true, if sector attached
	virtual bool Attached();

// Direct access to memory
public:

	// Must return true, if sector in memory
	virtual bool MemAvailable() { return true; };

	virtual BYTE* ContentP();

protected:

	// Reattaching on size changing
	short Reattach(D_SIZE size);

protected:

	virtual short CheckLayoutCascade(bool proceedCheckChain = false, bool checkUnderLayer = true, bool checkOverLayer = true);

// Implementation of (t_) Substrate
protected:

	virtual void* t_GetMem();
	virtual bool t_ReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size);
	virtual bool t_WriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size);
	virtual D_SIZE t_Read(void* pBuffer, D_SIZE size);
	virtual bool t_Write(const void* pBuffer, D_SIZE size);
	virtual bool t_Copy(D_OFFSET offsetFrom, D_OFFSET offsetTo, D_SIZE size);
	virtual bool t_Fill(BYTE btFill, D_OFFSET offsetFrom, D_SIZE size);

// Typed-methods (t_) Memory
protected:

	// Must perform reattaching to system resource on size changing
	virtual short t_Reattach(D_SIZE size) = 0;

protected:

	// Pointer to the first byte of sector
	void* m_pMem;

	// Size, saved in object
	// It compares to m_pHeader->Size(). if different then context has been changed by another object
	D_SIZE m_sizeAttached;
};

// class Attachable
// Context, attached to allocated memory

// Can't be used in multithreading

class Attachable : public Memory
{
public:

	Attachable();
	Attachable(WORD wdFlags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~Attachable();

protected:

	// Fields initialization
	void Set(WORD wdFlags = 0, D_SECTOR_ID id = D_RET_ERROR);

	// Resets fields
	bool Reset();

public:

	// Sector type
	virtual short GetType() { return MEM_CONTEXT_ATTACHABLE; };

	virtual D_SIZE ContextSize() { return m_header.location.size; };

	// Attaching to memory
	// pParentHeader - Header, from where the properties of the object's embedded header are copied
	bool Attach(void* pMem, D_SIZE size, Header* pParentHeader = nullptr);

// Implementation of (t_) Context
protected:

	virtual bool t_New(const char* strKey, Header* pHeader, WORD access, bool lock) { return false; };
	virtual bool t_Attach(const char* strKey, Header* pHeader, bool lock);

// Implementation of (t_) Substrate
protected:

	virtual bool t_Renew(D_SIZE size) { return false; };
	virtual bool t_Detach() { return false; };
	virtual bool t_Delete() { return false; };

// Implementation of (t_) Memory
protected:

	virtual short t_Reattach(D_SIZE size) { return CASCADE_LAYOUT_FORMER; };

private:

	// Embedded Header
	Header m_header;
};

// class Heap
// Context for heap memory

// Can't be used in multithreading

class Heap : public Memory
{
public:

	Heap();
	Heap(WORD wdFlags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~Heap();

protected:

	// Fields initialization
	void Set(WORD wdFlags = 0, D_SECTOR_ID id = D_RET_ERROR);

	// Resets fields
	bool Reset();

public:

	// Sector type
	virtual short GetType() { return MEM_CONTEXT_HEAP; };

	virtual D_SIZE ContextSize() { return m_pHeader->location.size; };

	// Attaching to memory
	bool Attach(void* pMem, Header* pHeader);

	// Implementation of (t_) Context
// strKey, access, lock are not used
protected:

	virtual bool t_New(const char* strKey, Header* pHeader, WORD access, bool lock);

	// To attach to allocated memory block, a pointer to the allocated memory is passed through strKey
	// location.size must be set in header
	virtual bool t_Attach(const char* strKey, Header* pHeader, bool lock);

// Implementation of of (t_) Substrate
protected:

	virtual bool t_Renew(D_SIZE size);
	virtual bool t_Detach() { return true; };
	virtual bool t_Delete();

// Implementation of (t_) Memory
protected:

	virtual short t_Reattach(D_SIZE size) { return CASCADE_LAYOUT_FORMER; };
};

// class Shared
// Context for shared Posix memory

// Can be used in multithreading

class Shared : public Memory
{
public:

	Shared();
	Shared(WORD flags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~Shared();

protected:

	// Fields initialization
	void Set(WORD flags = 0, D_SECTOR_ID id = D_RET_ERROR);

	// Resets fields
	bool Reset();

public:

	// Sector type
	virtual short GetType() { return MEM_CONTEXT_SHARED; };

	virtual D_SIZE ContextSize();

// Implementation of (t_) Context

// strKey - name of memory file without path
protected:

	virtual bool t_New(const char* strKey, Header* pHeader, WORD access, bool lock);
	virtual bool t_Attach(const char* strKey, Header* pHeader, bool lock);
	bool Open(const char* strKey, D_SIZE size, bool readonly, bool recreate, WORD access, bool lock);

// Implementation of of (t_) Substrate
protected:

	virtual bool t_Renew(D_SIZE sizeNew);
	virtual bool t_Detach();
	virtual bool t_Delete();

// Implementation of (t_) Memory
protected:

	virtual short t_Reattach(D_SIZE size);

protected:

	// Shared memory ID
	int m_MemId;
};

// class SystemV
// Context for SystemV shared memory

// This context can't be resized
// Renew doesn't work
// location.size must be set n Header on attaching

class SystemV : public Memory
{
public:

	SystemV();
	SystemV(WORD flags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~SystemV();

protected:

	// Fields initialization
	void Set(WORD flags = 0, D_SECTOR_ID id = D_RET_ERROR);

	// Resets fields
	bool Reset();

public:

	// Sector type
	virtual short GetType() { return MEM_CONTEXT_SYSTEMV; };

	virtual D_SIZE ContextSize();

// Implementation of (t_) Context
// strKey - memory ID generated by ::GenKey()
// and transformed to string
protected:

	virtual bool t_New(const char* strKey, Header* pHeader, WORD access, bool lock);
	virtual bool t_Attach(const char* strKey, Header* pHeader, bool lock);

// Implementation of of (t_) Substrate
protected:

	virtual bool t_Renew(D_SIZE sizeNew);
	virtual bool t_Detach();
	virtual bool t_Delete();

// Implementation of (t_) Memory
protected:

	virtual short t_Reattach(D_SIZE size);

private:

	// System memory key
	key_t m_keyMem;

	// System memory ID
	int m_MemID;
};

// class DiskMap
// Context for disk to memory map

// All mapping operations are performed by means of system

class DiskMap : public Memory
{
public:

	DiskMap();
	DiskMap(WORD flags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~DiskMap();

protected:

	// Fields initialization
	void Set(WORD flags = 0, D_SECTOR_ID id = D_RET_ERROR);

	// Resets fields
	bool Reset();

public:

	// Sector type
	virtual short GetType() { return MEM_CONTEXT_DISKMAP; };

	virtual D_SIZE ContextSize();

// Implementation of (t_) Context
// strKey - name of file according to the filesystem
protected:

	virtual bool t_New(const char* strKey, Header* pHeader, WORD access, bool lock);
	virtual bool t_Attach(const char* strKey, Header* pHeader, bool lock);
	bool Map(D_SIZE size, bool readonly, bool lock);

// Implementation of of (t_) Substrate
protected:

	virtual bool t_Renew(D_SIZE sizeNew);
	virtual bool t_Detach();
	virtual bool t_Delete();

	// Implementation of (t_) Memory
protected:

	virtual short t_Reattach(D_SIZE size);

private:

	// Embedded disk context
	Disk m_contextDisk;
};

} /* namespace Context */

} /* namespace DLayers */

#endif /* MEMORY_H_ */
