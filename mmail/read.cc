/*
 * MultiMail offline mail reader
 * main_read_class, reply_read_class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "mmail.h"
#include "compress.h"

/* read_class -- virtual */

read_class::~read_class()
{
}

/* main_read_class -- for regular areas */

main_read_class::main_read_class(mmail *mm, specific_driver *driverA)
{
	FILE *readFile;
	file_header *redfile, *xtifile;
	const char *readFileN;
	file_list *wl = mm->workList;
	driver = driverA;
	bool xti = false;

	ro = mm->resourceObject;

	noOfAreas = driver->getNoOfAreas();
	noOfLetters = new int[noOfAreas];
	readStore = new int *[noOfAreas];

	hasPersArea = driver->hasPersArea();
	hasPersNdx = !(!(wl->exists("personal.ndx")));

	// If basename.red not found, look for any .red file;
	// then look for an .xti file, and use the most recent:

	redfile = wl->existsF(readFilePath(ro->get(PacketName)));
	if (!redfile)
		redfile = wl->existsF(".red");

	xtifile = wl->existsF(".xti");

	if (xtifile && (!redfile || (xtifile->getDate() >
	    redfile->getDate()))) {
		xti = true;
		readFileN = xtifile->getName();
	} else
		readFileN = redfile ? redfile->getName() : 0;

	readFile = readFileN ? wl->ftryopen(readFileN, "rb") : 0;

	// Don't init personal area, unless using .red, and QWK
	// personal.ndx (this is for backwards compatibility):

	bool noskip = !hasPersArea || (!xti && hasPersNdx);

	for (int c = 0; c < noOfAreas; c++) {
		driver->selectArea(c);
		noOfLetters[c] = driver->getNoOfLetters();
		readStore[c] = new int[noOfLetters[c]];

		// .xti -> .red mapping: crude, but effective:

		for (int d = 0; d < noOfLetters[c]; d++)
		    readStore[c][d] = readFile ? ((c || noskip) ? (xti ?
			(fgetc(readFile) & (MS_READ | MS_REPLIED)) +
			(fgetc(readFile) ? MS_MARKED : 0) :
			fgetc(readFile)) : 0) : 0;
	}
	if (readFile)
		fclose(readFile);
}

main_read_class::~main_read_class()
{
	while(noOfAreas)
		delete[] readStore[--noOfAreas];
	delete[] readStore;
	delete[] noOfLetters;
}

void main_read_class::setRead(int area, int letter, bool value)
{
	if (value)
		readStore[area][letter] |= MS_READ;
	else
		readStore[area][letter] &= ~MS_READ;
}

bool main_read_class::getRead(int area, int letter)
{
	return !(!(readStore[area][letter] & MS_READ));
}

void main_read_class::setStatus(int area, int letter, int value)
{
	readStore[area][letter] = value;
}

int main_read_class::getStatus(int area, int letter)
{
	return readStore[area][letter];
}

int main_read_class::getNoOfUnread(int area)
{
	int tmp = 0;

	for (int c = 0; c < noOfLetters[area]; c++)
		if (!(readStore[area][c] & MS_READ))
			tmp++;
	return tmp;
}

int main_read_class::getNoOfMarked(int area)
{
	int tmp = 0;

	for (int c = 0; c < noOfLetters[area]; c++)
		if (readStore[area][c] & MS_MARKED)
			tmp++;
	return tmp;
}

bool main_read_class::saveAll()
{
	FILE *readFile;
	int oldsort = lsorttype;

	lsorttype = LS_MSGNUM;

	const char *readFileN = 0, *oldFileN = ro->getInt(MakeOldFlags) ?
		driver->saveOldFlags() : 0;

	lsorttype = oldsort;

	if (!oldFileN) {
		readFileN = readFilePath(ro->get(PacketName));
		readFile = fopen(readFileN, "wb");

		for (int c = (hasPersArea && !hasPersNdx); c < noOfAreas; c++)
			for (int d = 0; d < noOfLetters[c]; d++)
				fputc(readStore[c][d], readFile);

		fclose(readFile);
	}

	// add the .red file to the packet
	return !compressAddFile(ro, ro->get(PacketDir),
		ro->get(PacketName), oldFileN ? oldFileN : readFileN);
}

const char *main_read_class::readFilePath(const char *FileN)
{
	static char tmp[13];

	if (mychdir(ro->get(WorkDir)))
		fatalError("Unable to change to work directory");
	sprintf(tmp, "%.8s.red", findBaseName(FileN));
	return tmp;
}

/* reply_read_class -- for reply areas */
/* (Formerly known as dummy_read_class, because it does almost nothing) */

reply_read_class::reply_read_class(mmail *mmA, specific_driver *driverA)
{
	mmA = mmA;
	driverA = driverA;
}

reply_read_class::~reply_read_class()
{
}

void reply_read_class::setRead(int area, int letter, bool value)
{
	area = area;
	letter = letter;
	value = value;
}

bool reply_read_class::getRead(int area, int letter)
{
	area = area;
	letter = letter;
	return true;
}

void reply_read_class::setStatus(int area, int letter, int value)
{
	area = area;
	letter = letter;
	value = value;
}

int reply_read_class::getStatus(int area, int letter)
{
	area = area;
	letter = letter;
	return 1;
}

int reply_read_class::getNoOfUnread(int area)
{
	area = area;
	return 0;
}

int reply_read_class::getNoOfMarked(int area)
{
	area = area;
	return 0;
}

bool reply_read_class::saveAll()
{
	return true;
}
