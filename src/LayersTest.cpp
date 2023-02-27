//============================================================================
// Name        : LayersTest.cpp
// Author      : Dan Lerner
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <limits.h>
#include <sys/shm.h>
#include <math.h>

#include "Layers/Context/Disk.h"
#include "Layers/Context/Memory.h"
#include "Layers/Sector/ChainSector.h"

int main()
{
	// This is the test of "Layers" classes functionality
	// "Layers" is a simple filesystem-like tools for RAM/ROM management
	// used rather for embedded projects

	// I use C-like syntax in this file for make it easier for testing observations

	// Contexts test
	///////////////////////////////////////////////////////////////

	using namespace Layers;
	using namespace Layers::Contexts;

	//	Attachable context test
	//--------------------------

	UINT uint = 10;

	Attachable contextAttach;
	if (!contextAttach.Attach(&uint, sizeof(UINT)))
	{ return 1; }

	// Observe the uint variable

	if (!contextAttach.ClearContent())
	{ return 1; }

	UINT uint1 = 0xABCD;

	if (!contextAttach.SetContent(&uint1, sizeof(UINT)))
	{ return 1; }

	//	Heap context test
	//--------------------

	Header headerHeap(0, 4, 1, 256, 0xDD, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	Heap contextHeap;
	if (!contextHeap.Create(nullptr, &headerHeap))
	{ return 1; }

	UINT uint3 = 0x01234567;

	if (!contextHeap.SetContent(&uint3, sizeof(UINT)))
	{ return 1; }

	// It is possible to work in one process with several objects with one heap memory

	Heap contextHeap1;
	if (!contextHeap1.Attach(contextHeap.GetMem(), &headerHeap))
	{ return 1; }

	if (!contextHeap1.Renew(sizeof(ULONG)))
	{ return 1; }

	ULONG ulong1 = *((ULONG*)contextHeap1.ContentP());

	ULONG ulong2 = 0x0123456789ABCDEF;

	if (!contextHeap1.SetContent(&ulong2, sizeof(ULONG)))
	{ return 1; }

	ulong1 = *((ULONG*)contextHeap1.ContentP());

	if (!contextHeap1.Delete())
	{ return 1; }

	//	SystemV - shared memory test
	//-------------------------------

	// SystemV can't be resized

	Header headerSysV(0, 8, 1, 256, 0xDD, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	SystemV contextSysV1;
	SystemV contextSysV2;

	// SystemV system name generation
	key_t m_keyMem;
	m_keyMem = ftok(".", 5);
	char strSysVName[32];
	sprintf(strSysVName, "%d", m_keyMem);

	if (contextSysV1.Attach(strSysVName, &headerSysV))
	{
		// Deletes if not deleted
		if (!contextSysV1.Delete())
		{ return 1; }
	}

	if (!contextSysV1.Create(strSysVName, &headerSysV))
	{ return 1; }

	ulong1 = 0x0123456789ABCDEF;

	if (!contextSysV1.SetContent(&ulong1, sizeof(ULONG)))
	{ return 1; }
	if (!contextSysV1.GetContent(&ulong2))
	{ return 1; }
	if (!contextSysV2.Attach(strSysVName, &headerSysV))
	{ return 1; }

	ULONG ulong3 = 0;

	if (!contextSysV2.GetContent(&ulong3))
	{ return 1; }
	if (!contextSysV2.Delete())
	{ return 1; }

	// Disk, Shared, DiskMap contexts test
	//-------------------------------------

	// Observe changes by means of mc hex viewer for instance

	Header header1(0, 32, 16, 256, 0xDD, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	// Disks contexts
//	Disk context1(Substrate::FLAG_SAVED_HEADER);
//	Disk context2(Substrate::FLAG_SAVED_HEADER);
//	if (!context1.Create(name.ConfigFile("Sector"), &header1))
//	{ return 1; }

	// Posix shared memory context
	// Header places inside the sector

	// Observe /dev/shm/shadow_1111

	char strSharedName[32];
	::sprintf(strSharedName, "shadow_%d", 1111);
//	Shared context1(Substrate::FLAG_SAVED_HEADER | Substrate::USE_INSIDE_HEADER);
//	Shared context2(Substrate::FLAG_SAVED_HEADER | Substrate::USE_INSIDE_HEADER);
	Shared context1(Substrate::FLAG_SAVED_HEADER);
	Shared context2(Substrate::FLAG_SAVED_HEADER);
	if (!context1.Create(strSharedName, &header1))
	{ return 1; }

	// Disk map context
//	DiskMap context1(Substrate::FLAG_SAVED_HEADER);
//	DiskMap context2(Substrate::FLAG_SAVED_HEADER);
//	if (!context1.Create(name.ConfigFile("Sector"), &header1))
//	{ return 1; }

	context1.Detach();

//	if (!context1.Attach(name.ConfigFile("Sector"), &header1))
	if (!context1.Attach(strSharedName, &header1))
	{ return 1; }

	if (!context1.Expand(100))
	{ return 1; }
	if (!context1.Expand(120))
	{ /* Size limit exceeded*/ }

	if (!context1.Renew(10))
	{ return 1; }
	if (!context1.Renew(129, true))
	{ return 1; }

//	context2.Attach(name.ConfigFile("Sector"), &header1);
	if (!context2.Attach(strSharedName, &header1))
	{ return 1; }

	D_SIZE size = context2.Size();
	if (!context2.Renew(128, true))
	{ return 1; }
	size = context2.Size();

	ulong1 = 0x0123456789ABCDEF;

	if (!context2.SetContent(&ulong1, sizeof(ULONG)))
	{ return 1; }

	// 	Single sector test
	///////////////////////////////////////////////////////////////

	if (!context1.Renew(128))
	{ return 1; }

	Header headerSector(32, 16, 16, 256, 0xAA, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	// Embedded Header is used in this test
	// It will work in case of single Sector object attaching
	// Two objects will not be able to synchronize the pointer to the Header in the sector
	// unless FLAG_UNMOVABLE is set
	// The sector must be cascaded with the substrate also
	// Multiple objects can operate on embedded headers, as long as the contexts don't change location
	// Otherwise, when one object changes the context address, Header will be lost for parallel objects

	Sector sector(Substrate::USE_INSIDE_HEADER);
	if (!sector.Create(&context1, &headerSector))
	{ return 1; }
	if (!sector.Layon(&context1))
	{ return 1; }

	if (!sector.MoveSector(96))
	{ return 1; }
	if (!sector.Detach())
	{ return 1; }

	// If the substrate (context) is not in memory, it doesn't work!
	if (!sector.Attach(&context1, nullptr, 96))
	{ return 1; }
	if (!sector.Layon(&context1))
	{ return 1; }
	if (!sector.MoveSector(128))
	{ return 1; }
	if (!sector.Renew(64))
	{ return 1; }
	if (!sector.MoveSector(32))
	{ return 1; }
	if (!sector.Delete())
	{ return 1; }

	// 	Cascade test using Chain Sectors
	///////////////////////////////////////////////////////////////

	// Context Settings changing
	header1.settings.Set(64, Settings::SIZE_UNLIMIT, 0xDD, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	// Chain sectors on context

	Header headerSector1(64, 32, 16, 256, 0xAA, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);
	Header headerSector2(0, 32, 16, 256, 0xBB, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);
	Header headerSector3(0, 64, 32, 512, 0xCC, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	ChainSector sector1;
	ChainSector sector2;
	ChainSector sector3;

	if (!sector2.Create(&context1, &headerSector2))
	{ return 1; }
	if (!sector2.Layon(&context1))
	{ return 1; }
	if (!sector3.CreateTail(&sector2, &headerSector3))
	{ return 1; }
	if (!sector1.CreateHead(&sector2, &headerSector1))
	{ return 1; }

	if (!sector2.MoveSector(96))
	{ return 1; }
	if (!sector2.Renew(128))
	{ return 1; }

	// Parallel objects
	ChainSector sector1_s;
	ChainSector sector2_s;
	ChainSector sector3_s;

	// Before performing parallel Sector operations, a CheckLayout() call is required in cases:
	// - HeaderStore is used for update the pointer to the Header in the HeaderStore
	// and/or
	// - When two or more objects attached to the same Sector,
	// and the context in memory can change the address, for example, when resizing
	// and/or
	// - When two or more objects attached to the same Sector,
	// and one of the objects can change the sequence of sectors
	// Sectors calculate their pointers from the context, so they are not checked

	context2.CheckLayout();

	if (!sector1_s.Attach(&context2, &headerSector1))
	{ return 1; }
	if (!sector1_s.Layon(&context2))
	{ return 1; }
	if (!sector2_s.AttachToChain(&sector1_s, &headerSector2))
	{ return 1; }
	if (!sector3_s.AttachToChain(&sector1_s, &headerSector3))
	{ return 1; }

	if (!context2.Collapse())
	{ return 1; }

//	sector3_s.MoveAfter(nullptr);

	if (!sector1_s.Renew(96))
	{ return 1; }

	// Chain sectors laying on Sector 3

	Header headerSector31(0, 16, 16, 256, 0x11, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);
	Header headerSector32(0, 16, 16, 256, 0x22, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);
	Header headerSector33(0, 16, 16, 256, 0x33, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	ChainSector sector31;
	ChainSector sector32;
	ChainSector sector33;

	context1.CheckLayout();

	if (!sector32.Create(&sector3, &headerSector32))
	{ return 1; }
	if (!sector32.Layon(&sector3))
	{ return 1; }
	if (!sector33.CreateTail(&sector32, &headerSector33))
	{ return 1; }
	if (!sector31.CreateHead(&sector32, &headerSector31))
	{ return 1; }

	// Parallel sectors
	ChainSector sector31_s;
	ChainSector sector32_s;
	ChainSector sector33_s;

	context2.CheckLayout();

	if (!sector31_s.Attach(&sector3_s, &headerSector31))
	{ return 1; }
	if (!sector31_s.Layon(&sector3_s))
	{ return 1; }
	if (!sector32_s.AttachToChain(&sector31_s, &headerSector32))
	{ return 1; }
	if (!sector33_s.AttachToChain(&sector31_s, &headerSector33))
	{ return 1; }

	if (!sector32.Renew(64))
	{ return 1; }
	if (!sector32.MoveSector(96))
	{ return 1; }
	if (!sector3.MoveOn(&sector2))
	{ return 1; }

	// In this case sector3_s and everything on it will be ordered. Chain with sector3_s is not ordered
	// Underlying sectors will also be checked vertically up to the context
	// Such a check takes less resources, but guarantees correct work only with sector3_s and everything it includes

//	sector3_s.CheckLayout();

	// Such a check guarantees correct operation with the entire cascade, but takes more resources.
	context2.CheckLayout();

	// Save Sector 3 to file
	Header headerFile(0, 1, 1, 1024, 0xDD, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	Disk contextFile(Substrate::FLAG_SAVED_HEADER);
	if (!contextFile.Create("SavedSector", &headerFile))
	{ return 1; }
	if (!contextFile.CopyContent(&sector3))
	{ return 1; }
	if (!sector1_s.DeleteContent())
	{ return 1; }
	if (!context2.Collapse())
	{ return 1; }

	// Copying from one file to another

	Header headerFile1(0, 1, 1, 1024, 0xFF, Settings::FLAG_RESIZABLE | Settings::FLAG_FILL_EMPTY | Settings::FLAG_CLEAN);

	Disk contextFile1;
	if (!contextFile1.Create("SavedSector1", &headerFile1))
	{ return 1; }

	if (!contextFile1.CopyContent(&contextFile))
	{ return 1; }

	// Copy from file to sector 1
	if (!sector1_s.CopyContent(&contextFile1))
	{ return 1; }

	if (!contextFile.Delete()		||
			!contextFile1.Delete()	||
			!sector31_s.Delete()	||
			!sector32_s.Delete()	||
			!sector33_s.Delete()	||

			!sector1_s.Delete()		||
			!sector2_s.Delete()		||
			!sector3_s.Delete()		||

			!context2.DeleteContent())
	{ return 1; }

	return 0;
}
