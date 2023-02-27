/*
 * Location.h
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#ifndef LOCATION_H_
#define LOCATION_H_

#include "../Macro.h"

// The Location class incapsulates sector coordinates
// and implements basic operations

namespace Layers
{

class Location
{
public:
	Location();
	Location(D_OFFSET offset, D_SIZE size);

	~Location();

public:

	// Validates coordinates
	bool Defined()
	{ return offset != D_RET_ERROR && size != D_RET_ERROR;	}

	// Validates offset
	bool OffsetDefined()
	{ return offset != D_RET_ERROR;	}

	// Validates size
	bool SizeDefined()
	{ return size != D_RET_ERROR; }

	// Returns end coordinate of sector
	int RightMargin()
	{ return Defined() ? offset + size : D_RET_ERROR; }

public:

	// Sets location
	void Set(D_OFFSET offset, D_SIZE size);

	// Resets location to undefined
	void Undefine();

	// Moves to new location
	bool Move(D_OFFSET offset);

	// Shifts offset by offsetDelta
	bool Offset(D_OFFSET offsetDelta);

	// Shifts offset by size forward
	bool StepForward();

	// Sets new size
	bool Resize(D_SIZE size);

	// Changes size by sizeDelta
	bool Expand(D_SIZE sizeDelta);

// Overloaded operators
public:

	Location &operator=(const Location& fragment);

	bool operator==(const Location& fragment)
	{ return offset == fragment.offset && size == fragment.size; }

	bool operator!=(const Location& fragment)
	{ return offset != fragment.offset || size != fragment.size; }

public:

	D_OFFSET offset;
	D_SIZE size;
};

} /* namespace DLayers */

#endif /* LOCATION_H_ */
