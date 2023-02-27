/*
 * IHeaderStore.h
 *
 *  Created on: 26 сент. 2016 г.
 *      Author: dan
 */

#ifndef IHEADERSTORE_H_
#define IHEADERSTORE_H_

#include "../Macro.h"

#define D_HEADER_ID		int

namespace Layers
{

// interface IHeaderStore

// Deleting a header is done either using DeleteHeader
// or if FLAG_AUTODELETE is set in the header, while
// the header is removed when the last object is detached from the sector

class Header;

class IHeaderStore
{
public:

	virtual ~IHeaderStore() = 0;

public:

	// New space request
	virtual D_INDEX Request(Header* pHeader) = 0;

	// Header search
	// If index == D_RET_ERROR, then search performed by id
	// Else index must be checked first
	virtual Header* Find(D_HEADER_ID id, D_INDEX index = D_RET_ERROR) = 0;

	// Header deleting
	virtual bool DeleteHeader(D_HEADER_ID id, D_INDEX index = D_RET_ERROR) = 0;
};

} /* namespace DLayers */

#endif /* IHEADERSTORE_H_ */
