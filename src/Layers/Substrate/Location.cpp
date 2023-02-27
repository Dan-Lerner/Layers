/*
 * Location.cpp
 *
 *  Created on: 17 сент. 2016 г.
 *      Author: dan
 */

#include "Location.h"

namespace Layers
{

Location::Location()
{
	Undefine();
}

Location::Location(D_OFFSET offset, D_SIZE size)
{
	Location::offset = offset;
	Location::size = size;
}

Location::~Location()
{
	Set(offset, size);
}

void Location::Set(D_OFFSET offset, D_SIZE size)
{
	Location::offset = offset;
	Location::size = size;
}

void Location::Undefine()
{
	Set(D_RET_ERROR, D_RET_ERROR);
}

bool Location::Move(D_OFFSET offset)
{
	if (offset < 0)
	{
		return false;
	}

	Location::offset = offset;

	return true;
}

bool Location::Offset(D_OFFSET offsetDelta)
{
	if (Location::offset + offsetDelta < 0)
	{
		return false;
	}

	Location::offset += offsetDelta;

	return true;
}

bool Location::StepForward()
{
	Location::offset += size;

	return true;
}

bool Location::Resize(D_SIZE size)
{
	if (size < 0)
	{
		return false;
	}

	Location::size = size;

	return true;
}

bool Location::Expand(D_SIZE sizeDelta)
{
	if (Location::size + sizeDelta < 0)
	{
		return false;
	}

	Location::size += sizeDelta;

	return true;
}

Location &Location::operator=(const Location& fragment)
{
	if (this == &fragment)
	{
		return *this;
	}

	offset = fragment.offset;
	size = fragment.size;

 	return *this;
}

} /* namespace DLayers */
