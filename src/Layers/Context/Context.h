/*
 * Context.h
 *
 *  Created on: 17 июля 2016 г.
 *      Author: dan
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>

#include "../Substrate/Substrate.h"

namespace Layers
{

namespace Contexts
{

// Class Context - Base class for all contexts

// !!! In Contexts sectors SaveHeader must be saved entirely

class Context: public Substrate
{

// Constants and flags
public:

	// Permission flags
	static const WORD U_R			= S_IRUSR;
	static const WORD U_W			= S_IWUSR;
	static const WORD U_X			= S_IXUSR;
	static const WORD G_R			= S_IRGRP;
	static const WORD G_W			= S_IWGRP;
	static const WORD G_X			= S_IXGRP;
	static const WORD O_R			= S_IXGRP;
	static const WORD O_W			= S_IWOTH;
	static const WORD O_X			= S_IXOTH;

	static const WORD DEF_ACCESS	= U_R | U_W;

public:

	Context();
	Context(WORD flags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~Context();

protected:

	// Initialization of fields
	void Set(WORD flags = 0, D_SECTOR_ID id = D_RET_ERROR);

	// Resets all fields
	bool Reset();

public:

	// Sector type
	virtual short Type() { return ICONTEXT; };

	// Context size. Must be implemented by means of system
	virtual D_SIZE ContextSize() { return D_RET_ERROR; };

// New context creation

// strKey - string ID of system resource (filename, ID in shared memory etc)
// Settings must be initialized in Header
// and Location too
// In location.size you must set CONTENT size
// (it is recalculated to the whole sector size during initialization)
// access - access flags
// lock - if true, then memory is locked (does't move from RAM to disk)
public:

	bool Create(const char* strKey, Header* pHeader, WORD access = DEF_ACCESS, IHeaderStore* pHeaderStore = nullptr,
			bool lock = false);

// Attaching to existing context
// strKey - string ID of system resource (filename, ID in shared memory etc)
// lock - if true, then memory is locked (does't move from RAM to disk)

// The following options for setting parameters are possible

// 1. Header placed not in the sector (FLAG_SAVED_HEADER and USE_INSIDE_HEADER are not set)
// pHeader must be fully initialized (e.g. manually before attaching the first object
// or it is not the first object that is attached to the sector and pHeader is already
// initialized and used by other objects working with this sector

// 2. Header saved in sector (FLAG_SAVED_HEADER)
// pHeader is initialized inside the method using saved header. If the header is not saved
// completely, then non-persistent fields must be initialized manually in pHeader before
// attaching the first object

// 3. Header placed in sector in memory (USE_INSIDE_HEADER),
// pHeader and pHeaderStore are ignored

// 4. If Header saved in the sector and HeaderStore is used,
// then we may not to set the pHeader (pHeader = nullptr),
// The object reads the saved header, requests a place in the HeaderStore and initializes it

// 5. If the HeaderStore already has a Header, then only strKey and pHeaderStore can be set

public:

	bool Attach(const char* strKey, Header* pHeader, IHeaderStore* pHeaderStore = nullptr, bool lock = false);

protected:

	// Header reading before object attaching
	bool ReadHeader(const char* strKey, Header* pHeader);

// Typed-metods (t_), these are called for operations specific in child classes
protected:

	// Must perform new context creation
	virtual bool t_New(const char* strKey, Header* pHeader, WORD access, bool lock) = 0;

	// Must perform object attaching to context
	virtual bool t_Attach(const char* strKey, Header* pHeader, bool lock) = 0;

// Implementations
protected:

	// Должна реализовывать очистку места, освобождаемого сектором
	virtual bool t_Clean(D_OFFSET offset, D_SIZE size);

protected:

	// Buffer to store context key name
	char m_strKey[PATH_MAX];
};

} /* namespace Context */

} /* namespace DLayers */

#endif /* ICONTEXT_H_ */
