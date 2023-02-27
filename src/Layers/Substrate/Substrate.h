/*
 * Substrate.h
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#ifndef SUBSTRATE_H_
#define SUBSTRATE_H_

#include "../Macro.h"
#include "Location.h"

namespace Layers
{

// Settings for sector
class Settings
{

// Constants and flags
public:

	// Default for expandSize
	static const D_SIZE DEFAULT_EXPAND_SIZE	= 1;

	// Default for maxSize (sector size is not limited while expanding)
	static const D_SIZE SIZE_UNLIMIT		= -1;

	// Default free space byte-filler for emptyByte
	static const BYTE DEFAULT_FILLER		= 0xFF;

	// Flags for "flags"
	static const WORD FLAG_SECTOR_READONLY	= 0x0001;	// Sector is read only
	static const WORD FLAG_RESIZABLE		= 0x0002;	// Sector can change size
	static const WORD FLAG_UNMOVABLE		= 0x0004;	// Sector can't be moved to another location
	static const WORD FLAG_FILL_EMPTY		= 0x0008;	// Free space must be filled by btEmptyByte
	static const WORD FLAG_CLEAN			= 0x0010;	// Free space must be additionally cleaned
	static const WORD FLAG_DEFRAG			= 0x0020;	// Sector available for defragmentaton
	static const WORD FLAG_AUTODELETE		= 0x0040;	// Header for the sector available for autoremoving from HeaderStore when state.shAttachedN == 0

public:

	Settings();
	Settings(D_SIZE sizeExpand, D_SIZE sizeMax, BYTE emptyByte, WORD flags);
	~Settings();

// Adjusting settings (setters)
public:

	bool Set(D_SIZE sizeExpand, D_SIZE sizeMax, BYTE emptyByte, WORD flags);
	inline void SetEmptyByte(BYTE emptyByte)		{ Settings::emptyByte = emptyByte;		};
	inline void SetSizeExpand(D_SIZE sizeExpand)	{ Settings::sizeExpand = sizeExpand;	};
	inline void SetFlags(WORD flags)				{ flags |= flags;						};
	inline void ResetFlags(WORD flags)				{ flags &= ~flags;						};
	inline void SetReadonly()						{ SetFlags(FLAG_SECTOR_READONLY);		};

// Getters
public:

	D_SIZE ExpandSize()	{ return sizeExpand; 	};
	D_SIZE MaxSize()	{ return sizeMax; 		};
	BYTE EmptyByte()	{ return emptyByte;	};
	WORD Flags()		{ return flags;		};

	inline bool Readonly()		{ return flags & FLAG_SECTOR_READONLY;	};
	inline bool Resizable()		{ return flags & FLAG_RESIZABLE;		};
	inline bool Unmovable()		{ return flags & FLAG_UNMOVABLE;		};
	inline bool NeedFill()		{ return flags & FLAG_FILL_EMPTY;		};
	inline bool NeedClean()		{ return flags & FLAG_CLEAN;			};
	inline bool CanDefrag()		{ return flags & FLAG_DEFRAG;			};
	inline bool Autodelete()	{ return flags & FLAG_AUTODELETE;		};

protected:

	D_SIZE sizeExpand;	// The sector size is always a multiple of this value.
	D_SIZE sizeMax;		// Max sector size. If SIZE_UNLIMIT is set the size is unlimited
	BYTE emptyByte;		// Free space byte-filler
	BYTE reserved[5];	// Just for data alignment
	WORD flags;			// flags
};

// class State
// Incapsulates the current state of the sector
class State
{

// Constants and flags
protected:

	// Values for "flags"
	static const WORD FLAG_SESSION_OPENED	= 0x0001;	// Session is opened
	static const WORD FLAG_DELETED			= 0x0002;	// Header has been deleted by one of attached objects

public:

	State();
	~State();

public:

	// Amount of attached objects
	inline short AttachedN() 		{ return attachedN; };

	// Attaching a new object
	inline void Attach() 			{ attachedN++; };

	// Detaching an object
	inline void Detach() 			{ attachedN--; };

	// Session opening
	inline void OpenSession()		{ flags |= FLAG_SESSION_OPENED; };

	// Session closing
	inline void CloseSession()		{ flags &= ~FLAG_SESSION_OPENED; };

	// Creating. Just mark as non-deleted
	inline void Create()			{ flags &= ~FLAG_DELETED; };

	// Deleting. Just mark as deleted
	inline void Delete()			{ flags |= FLAG_DELETED; };

	// Getting all flags
	inline WORD Flags()				{ return flags; };

	// true if session opened
	inline bool SessionOpened()		{ return flags & FLAG_SESSION_OPENED; };

	// true if deleted
	inline bool Deleted()			{ return flags & FLAG_DELETED; };

protected:

	short attachedN;	// Amount of attached objects
	WORD flags;			// flags
};

// Header, saved between sessions
class SavedHeader
{
public:

	SavedHeader();
	~SavedHeader();

public:

	// Gets header size in bytes
	D_SIZE Size() { return size; };

	// Copies the header settings from another header
	bool Set(SavedHeader* pHeader);

	// Sets the header settings
	bool Set(D_SIZE sizeExpand = Settings::DEFAULT_EXPAND_SIZE,
			D_SIZE sizeMax = Settings::SIZE_UNLIMIT,
			BYTE emptyByte = Settings::DEFAULT_FILLER,
			WORD flags = Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	// Sets the sector ID
	void SetID(D_SECTOR_ID id) { SavedHeader::id = id; };

// Getters
public:

	D_SIZE ExpandSize()	{ return settings.ExpandSize();	};
	D_SIZE MaxSize()	{ return settings.MaxSize();	};
	bool UnlimitSize()	{ return settings.MaxSize() == Settings::SIZE_UNLIMIT;	};
	BYTE EmptyByte()	{ return settings.EmptyByte();	};

	WORD Flags()		{ return settings.Flags(); };
	bool Readonly()		{ return settings.Readonly();	};
	bool Resizable()	{ return settings.Resizable();	};
	bool Unmovable()	{ return settings.Unmovable();	};
	bool NeedFill()		{ return settings.NeedFill();	};
	bool NeedClean()	{ return settings.NeedClean();	};
	bool CanDefrag()	{ return settings.CanDefrag();	};

public:

	Location location;	// Sector location on substrate
	D_SIZE size;		// Header size
	D_SECTOR_ID id;		// Sector ID
	Settings settings;	// Sector settings
};

// Header, shared to all attached objects
class Header : public SavedHeader
{
public:

	// !!! "size" must be set in child classes

	Header();

	Header(D_OFFSET offset,										// Position on underlying substrate
			D_SIZE size,										// Size of sector (without header size)
			D_SIZE sizeExpand = Settings::DEFAULT_EXPAND_SIZE,	// Sector size is always a multiple of this value.
			D_SIZE sizeMax = Settings::SIZE_UNLIMIT,			// Max size of sector
			BYTE emptyByte = Settings::DEFAULT_FILLER,			// Filler for free space
			WORD flags = Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	~Header();

public:

	bool SessionOpened()	{ return state.SessionOpened(); };

public:
	// Current state of sector
	State state;
};

class IHeaderStore;

// class Substrate -
// Abstract class with basic functionality for all type of sectors
class Substrate
{

// Constant and flags
public:

	// Default buffer size for operations with buffer (copying, moving etc)
	// Buffer is allocated in stack while operation is performing
	static const WORD MAX_BUFFER_SIZE			= 512;

	// This value is returned by Defrag after defragmentation is complete
	static const INT SECTOR_FRAG_COMPLETED		= 0;

	// Values for CheckLayout

	// No one of sectors in cascade have changed their locations
	static const short CASCADE_LAYOUT_FORMER	= 0;

	// Some sectors in cascade have changed their locations
	static const short CASCADE_LAYOUT_CHANGED	= 1;

	// Flags for Substrate::m_flags
	static const WORD FLAG_READONLY				= 0x0001;	// For this attached object sector is readonly
	static const WORD FLAG_SAVED_HEADER			= 0x0002;	// Header must be saved at beginning of sector
	static const WORD USE_INSIDE_HEADER			= 0x0004;	// Use Header, placed in sector if any

// Sectors types
public:
	enum
	{
		SUBSTRATE = 1,

		SECTOR, CHAIN, INDEXED,

		FIXED, INDEXER, INDEXER_STORE, RECORD_INDEXER,

		RECORD,

		ICONTEXT,

		CONTEXT_DISK,

		MEM_CONTEXT, MEM_CONTEXT_ATTACHABLE, MEM_CONTEXT_HEAP,
		MEM_CONTEXT_SHARED, MEM_CONTEXT_SYSTEMV, MEM_CONTEXT_DISKMAP,

		ELEMENT, CELL, EXTERN_PROPERTIES, VALUE,
		ISTRING, STRING, USTRING,

		SURVEYOR
	};

public:

	Substrate();
	Substrate(WORD fLags, D_SECTOR_ID id = D_RET_ERROR);
	virtual ~Substrate();

protected:

	// Initialization
	virtual void Set(WORD wdFlags, D_SECTOR_ID id);

	// Resets sector data
	virtual bool Reset();

public:

	// Flags initialization
	virtual void SetFlags(WORD flags) { m_flags = flags; };

	// Sets sector as readonly
	void SetReadonty()	{ m_pHeader->settings.SetReadonly();		};

public:

	// Gets the sector type
	virtual short Type() { return SUBSTRATE; };

	// true, if object attached
	virtual bool Attached() = 0;

// Initialization stuff
protected:

	// Initialization on creation
	// Space for sector must be already allocated
	bool InitCreate(Header* pHeader);

	// Initialization on attach
	bool InitAttach(Header* pHeader);

	// Common initialization
	bool Init();

	// Calculates the sector size using settings
	virtual D_SIZE CalcSectorSize(Header* pHeader);

	// Calculates content offset
	virtual D_OFFSET CalcContentOffset(Header* pHeader);

	// Corrects demanding sector size corresponding to Settings::expandSize
	D_SIZE CorrectContentSize(D_SIZE sizeDemand, Header* pHeader);

// Deleting, detaching
public:

	// Deletes the sector
	virtual bool Delete();

	// Detaches the object from the sector
	virtual bool Detach();

// Content management
public:

	// Writes new content as binary
	virtual bool SetContent(void* pContent, D_SIZE size);

	// Copying of content from another sector
	virtual bool CopyContent(Substrate* pSector, D_SIZE sizeMaxBuffer = MAX_BUFFER_SIZE);

	// Gets content binary
	virtual bool GetContent(void* pContent);

	// Gets content binary
	// If MemAvailable() == true, then *ppContent just redirected to native memory block
	// *ppContent must be allocated in the stack
	virtual bool GetContentOptimal(void** ppContent);

	// Clears the content not changing sector size
	virtual bool ClearContent();

	// Deletes the content with size reducing to minimal value
	virtual bool DeleteContent();

	// Cleans the free space after sector deleting or reducing
	virtual bool Clean(Location& location);

// Getters
public:
	// Pointer to Header
	Header* GetHeader() { return m_pHeader; }

	// First Sector in overlaying Layer
	Substrate* GetOverLayer() { return m_pOverLayer; }

	// Content offset from beginning of sector
	D_OFFSET GetContentOffset() { return m_offsetContent; }

// Setters
public:

	// First Sector in overlaying Layer
	void SetOverLayer(Substrate* pOverLayer) { m_pOverLayer = pOverLayer; }

protected:

	// Settings adjustment
	bool SetSettings(Settings* setings);

	// Saving data from m_pHeader to Saved Header
	bool UpdateSavedHeader();

	// Reads data from SavedHeader to pHeader
	bool ReadSavedHeader(void* pHeader, D_SIZE size);

	// Initializes sector from SavedHeader
	bool InitFromSavedHeader();

	// Sets new offset value in Header
	virtual bool SetOffset(D_OFFSET offset);

	// Sets new size value in Header
	virtual bool SetSize(D_SIZE size);

protected:

	// This method is called after SavedHeader size has changed
	// It moves content and updates SavedHeader
	// !!! New size of SavedHeader must be already provided in t_SavedHeaderSize
	bool UpdateContentOffset();

	// true, if Header saved in sector
	bool IsSavedHeader();

	// true, if Header placed in sector are used (flag is set)
	bool FlagUseInsideHeader() { return m_flags & USE_INSIDE_HEADER; };

	// true, if Header placed in sector can be used
	bool CanUseInsideHeader();

// Size stuff
public:

	// Sector size in bytes including header
	inline D_SIZE Size() { return Attached() ? m_pHeader->location.size : D_RET_ERROR; };

	// Content size in bytes
	inline D_SIZE ContentSize() { return Attached() ? m_pHeader->location.size - m_offsetContent : D_RET_ERROR; };

	// Size changing

	// Sets new sector size equal to sizeContent.
	// If exactBytes != true, size will be multiple of Settings::expandSize
	virtual bool Renew(D_SIZE sizeContent, bool exactBytes = false);

	// Increases size on sizeExpand (negative value can be used)
	// Size will be adjusted to a multiple of Settings::expandSize
	bool Expand(D_SIZE sizeExpand = 1);

// Direct access to memory
public:

	// It must return true if sector placed in memory
	virtual bool MemAvailable() = 0;

	// Pointer to first byte of sector memory (if MemAvailable == true)
	void* GetMem();

	// Initializes pointers
	bool InitPointers();

	// Initialization of pointer to Header inside sector after moving
	void OffsetHeaderP(D_OFFSET offset);

	// Pointer to content
	virtual BYTE* ContentP() = 0;

// Rudimentary operations - content reading/writing
public:

	// true, if sector is readonly
	bool Readonly();

	// Reads data from location to pBuffer
	bool ReadFrom(void* pBuffer, Location& location);

	// Writes from pBuffer to location
	bool WriteFrom(const void* pBuffer, Location& location);

	// Sequential reading
	D_SIZE Read(void* pBuffer, D_SIZE size);

	// Current reader pointer
	D_OFFSET GetReadOffset();

	// Sets the reader pointer
	bool SetReadOffset(D_OFFSET offset);

	// Sequential writing
	bool Write(const void* pBuffer, D_SIZE size);

	// Writing to the end of sector
	bool WriteEnd(const void* pBuffer, D_SIZE size);

	// Current writer pointer
	D_OFFSET GetWriteOffset();

	// Sets the writer pointer
	bool SetWriteOffset(D_OFFSET offset);

// Data fragments operations
public:

	// Reading data from location to pFragment
	bool GetFragment(void* pFragment, Location& location);

	// Getting data by optimal way
	// If MemAvailable() == true, then *ppFragment will be set directly to sector memory
	// *ppFragment must be allocated in stack
	bool GetFragmentOptimal(void** ppFragment, Location& location);

	// Data writing from pFragment to location
	bool SetFragment(void* pFragment, Location& location);

	// Data copying from another sector to this sector
	bool CopyFragment(Substrate* pSector, Location& locationFrom, D_OFFSET offsetTo, D_SIZE sizeMaxBuffer = MAX_BUFFER_SIZE);

	// Compares data in pFragment with data placed in locationThis
	bool CompareFragment(void* pFragment, D_SIZE size, Location& locationThis, D_SIZE sizeMaxBuffer = MAX_BUFFER_SIZE);

	// Compares data from two sectors - pSector and this
	bool CompareFragment(Substrate* pSector, Location& location, Location& locationThis, D_SIZE sizeMaxBuffer = MAX_BUFFER_SIZE);

// Finding
public:

	// Binary data fragment finding
	D_OFFSET FindFragment(void* pFragment, D_SIZE size, D_SIZE sizeMaxBuffer = MAX_BUFFER_SIZE);

// Binary data blocks operations
public:

	// Request for performing data block copying
	bool RequestCopy(Location& location, D_OFFSET offsetTo);

	// Request for performing data block moving
	bool RequestMove(Location& location, D_OFFSET offsetTo);

	// Request for performing data block shifting
	bool RequestShift(Location& location, D_OFFSET offsetTo);

	// Request for performing data blocks exchanging
	bool RequestExchange(Location& location, D_OFFSET offsetTo);

	// Request for performing data block insertion
	bool RequestInsertPlace(Location& location);

	// Request for performing data block insertion and filling
	bool RequestInsertFill(Location& location);

	// Request for performing data block deletion
	bool RequestRemove(Location& location);

	// Request for performing data block filling
	bool RequestFill(Location& location);


	// Performing data block copying
	bool ProcessCopy(Location& location, D_OFFSET offsetTo);

	// Performing data block moving
	bool ProcessMove(Location& location, D_OFFSET offsetTo);

	// Performing data block shifting
	bool ProcessShift(Location& location, D_OFFSET offsetTo);

	// Performing data blocks exchanging
	bool ProcessExchange(Location& location, D_OFFSET offsetTo, D_SIZE sizeMaxBuffer = MAX_BUFFER_SIZE);

	// Performing data block insertion
	bool ProcessInsertPlace(Location& location);

	// Performing data block insertion and filling
	bool ProcessInsertFill(BYTE btFill, Location& location);

	// Performing data block deletion
	bool ProcessRemove(Location& location);

	// Performing data block filling
	bool ProcessFill(BYTE btFill, Location& location);


	// Request and performing data block copying
	bool Copy(Location& location, D_OFFSET offsetTo);

	// Request and performing data block moving
	bool Move(Location& location, D_OFFSET offsetTo);

	// Request and performing data block shifting
	bool Shift(Location& location, D_OFFSET offsetTo);

	// Request and performing data blocks exchanging
	bool Exchange(Location& location, D_OFFSET offsetTo);

	// Request and performing data block insertion
	bool InsertPlace(Location& location);

	// Request and performing data block insertion and filling
	bool InsertFill(BYTE btFill, Location& location);

	// Request and performing data block deletion
	bool Remove(Location& location);

	// Request and performing data block filling
	bool Fill(BYTE btFill, Location& location);

	// Request and performing data block filling wit default filler
	bool Fill(Location& location);

// Cascade operations
public:

	// This method must be called in the case when
	// there are more than one object attached to sector
	// It must be called before current object starts working with the sector
	// after other object has finished its work

	// This method checks possible changing in sectors and synchronizes
	// changed data if any

	// All sectors in cascade are checked starting the most lower sector (it may change location and address)
	// Then all sectors are checked along the cascades and chains

	// Pointer to Header is checked also

	// Returning value
	// D_RET_ERROR				- error
	// CASCADE_LAYOUT_FORMER	- sector location hasn't changed
	// CASCADE_LAYOUT_CHANGED	- sector location has changed
	short CheckLayout() { return CheckLayoutCascade(); };

	// Cascade initialization of header pointers on location changing
	virtual void OffsetInsideHeader(D_OFFSET offset) { return OffsetInsideHeaderCascade(offset); } ;

	// Cascade updating of objects pointers
	virtual void UpdatePointers() { return UpdatePointersCascade(); };

	// Collapses cascade
	// Must be used with care, cause it may take a considerable amount of time
	bool Collapse() { return CollapseCascade(); };

	// True if defragmentation is available
	bool CanDefrag() { return CanDefragCascade(); };

	// Step by step defragmentation

	// Return value - remain amount of fragmented elements
	// (the number of fragmented elements before the step)
	// SECTOR_FRAG_COMPLETED, if defrag is finished

	// For progress calculation you need to store the first return value
	// and then calculate the progress at each step

	// If steps of defrag performs between sector operations
	// then amount of fragmented elements may change
	int DefragQuantum() { return DefragQuantumCascade(); };

	// Cascade defrag in one call
	// Must be used carefully, cause it may take a considerable amount of timeемя
	bool Defrag();

	// Opens the session
	bool StartSession() { return StartSessionCascade(); };

	// Closes the session
	bool CloseSession() { return CloseSessionCascade(); };

protected:

	// Checks the sectors locations in cascade
	virtual short CheckLayoutCascade(bool proceedCheckChain = false, bool checkUnderLayer = true, bool checkOverLayer = true);

	short CheckHeaderStore();

	// Cascade pointers update
	// !!! it can be not actual
	virtual void UpdatePointersCascade(bool proceedChain = false);

	// Cascade initialization of header pointers inside the sectors on sector moving
	virtual void OffsetInsideHeaderCascade(D_OFFSET offset, bool proceedChain = false);

	// Collapses the sector to minimal size
	virtual bool CollapseCascade(bool proceedChain = false);

	// Cascade request for defrag availability
	virtual bool CanDefragCascade(bool proceedChain = false);

	// One step of defrag performing
	virtual int DefragQuantumCascade(bool proceedChain = false, bool calcRemain = false);

	// Session opening - loads state
	virtual bool StartSessionCascade(bool proceedChain = false);

	// Session closing - saves state
	virtual bool CloseSessionCascade(bool proceedChain = false);

// Service
public:

	// true, if fragment placed within sector
	bool BlockInside(Location& location);

	// true, if offset placed within location
	bool PositionInLocation(D_OFFSET offset, Location location);

	// Calculates space available for requested fragment
	D_SIZE GetSizeInside(Location& location);

	// Requests space in sector. If needed and possible the sector expands
	D_SIZE RequestSpace(Location& location);

	// true, if fragments overlaps
	bool FragmentsOverlapped(Location& location1, Location& location2);

	// Calculates location of deleted space after moving
	bool GetDeletedAfterMoving(Location& location, D_OFFSET offsetTo, Location& locationDeleted);

	// Calculates location of free space after moving
	bool GetFreeAfterMoving(Location& location, D_OFFSET offsetTo, Location& locationFree);

// HeaderStore operations
protected:

	// Header request in HeaderStore
	// If Header with id not found, then new Header is created in HeaderStore
	// Else pointer to existing one is returned
	Header* RequestHeader(IHeaderStore* pHeaderStore, D_SECTOR_ID id, Header* pHeader);

// notification (n_), these are called from overlying sectors
public:

	// Overlying pSector demands sizeNew
	// sizeNew - new entire sector size
	virtual bool n_RequestSectorResize(Substrate* pSector, D_SIZE sizeNew);

	virtual bool n_SectorResized(Substrate* pSector) { return true; };

// Typed-methods (t_) Substrate
public:

	// If MemAvailable == true then returns pointer to the first byte of sector
	virtual void* t_GetMem() = 0;

	// In this method Header is corrected if necessary
	virtual bool t_CorrectHeader(Header* pHeader) { return true; };

	// These three realizes operations with SavedHeader placed in sector space
	// By default if FLAG_SAVED_HEADER is set then whole header is saved
	// Due to these it is possible in child classes to save header as you need

	// This method must initialize pSaveHeader
	// pSaveHeader - buffer for initialization with size defined in t_SavedHeaderSize
	virtual bool t_InitSavedHeader(void* pSaveHeader);

	// This method must initialize sector from pSaveHeader
	// pSaveHeader - saved SavedHeader
	virtual bool t_InitFromSavedHeader(void* pSaveHeader);

	// This method must return the size of SavedHeader
	// This size must be defined before creation sector or attaching to one
	virtual D_SIZE t_SavedHeaderSize() { return sizeof(SavedHeader); };

	// This method is called after settings changing
	virtual bool t_SettingsChanged() { return true; };

	// This method is called just before size changing
	// sizeOld, sizeNew - sizes of content
	virtual bool t_PreRenew(D_SIZE sizeOld, D_SIZE sizeNew) { return true; };

	// This method must perform resizing of sector
	// size - sector size including Header
	virtual bool t_Renew(D_SIZE size) = 0;

	// This method is called just after size changing
	// sizeOld, sizeNew - sizes of content
	virtual bool t_PostRenew(D_SIZE sizeOld, D_SIZE sizeNew) { return true; };

	// Must perform physical cleaning of data
	virtual bool t_ClearContent() { return true; };

	// Must perform content deletion with reducing sector size to minimal value
	virtual bool t_DeleteContent() { return true; };

	// Must perform object detaching
	virtual bool t_Detach() = 0;

	// Must perform data deletion
	virtual bool t_Delete() = 0;

	// Must perform physical cleaning of data in free space
	virtual bool t_Clean(D_OFFSET offset, D_SIZE size) = 0;

	// Must perform data reading from offset position with size bytes length
	// Return value - amount of bytes actually read or D_RET_ERROR
	virtual bool t_ReadFrom(void* pBuffer, D_OFFSET offset, D_SIZE size) = 0;

	// Must perform data writing to offset position with size bytes length
	// Return value - amount of bytes actually wrote or D_RET_ERROR
	virtual bool t_WriteFrom(const void* pBuffer, D_OFFSET offset, D_SIZE size) = 0;

	// Must perform sequential data reading from current position with size bytes length
	// Return value - amount of bytes actually read or D_RET_ERROR
	virtual D_SIZE t_Read(void* pBuffer, D_SIZE size) = 0;

	// Must perform sequential data writing to current position with size bytes length
	// Return value - amount of bytes actually wrote or D_RET_ERROR
	virtual bool t_Write(const void* pBuffer, D_SIZE size) = 0;

	// Must perform copying ob binary data block
	virtual bool t_Copy(D_OFFSET offsetFrom, D_OFFSET offsetTo, D_SIZE size) = 0;

	// Must perform data block filling with btFill
	virtual bool t_Fill(BYTE btFill, D_OFFSET offset, D_SIZE size) = 0;

protected:

	// D_SECTOR_ID id - ID of Sector
	// The ID for different sectors must be unique in the scope of all interacting processes
	// For objects from different processes working with the same sector, the ID must be the same
	// If the objects do not interact with service objects such as the HeaderStore, the ID can be omitted
	D_SECTOR_ID m_id;

	// Pointer to Header
	Header* m_pHeader;

	// First Sector in overlaying Layer
	Substrate* m_pOverLayer;

	// Content offset from beginning of sector
	D_OFFSET m_offsetContent;

	// Current read position for sequential reading
	D_OFFSET m_offsetRead;

	// Current write position for sequential writing
	D_OFFSET m_offsetWrite;

	// Pointer to HeaderStore
	IHeaderStore* m_pHeaderStore;

	// Sector index in HeaderStore
	D_INDEX m_indexStore;

	// flags
	WORD m_flags;
};

} /* namespace DLayers */

#endif /* ISUBSTRATE_H_ */
