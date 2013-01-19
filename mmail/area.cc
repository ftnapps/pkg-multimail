/*
 * MultiMail offline mail reader
 * area_header and area_list

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "mmail.h"

// ---------------------------------------------------------------------------
// Area header methods
// ---------------------------------------------------------------------------

area_header::area_header(mmail *mmA, int numA, const char *shortNameA,
			const char *nameA, const char *descriptionA,
			const char *areaTypeA, unsigned typeA,
			int noOfLettersA, int noOfPersonalA,
			int maxtolenA, int maxsublenA)
{
	mm = mmA;
	shortName = shortNameA;
	name = nameA;
	description = descriptionA;
	areaType = areaTypeA;
	type = typeA;
	num = numA;
	noOfLetters = noOfLettersA;
	noOfPersonal = noOfPersonalA;
	maxtolen = maxtolenA;
	maxsublen = maxsublenA;

	driver = mm->driverList->getDriver(num);
}

const char *area_header::getName() const
{
	return name;
}

const char *area_header::getShortName() const
{
	return shortName;
}

const char *area_header::getDescription() const
{
	return description;
}

const char *area_header::getAreaType() const
{
	return areaType;
}

unsigned area_header::getType() const
{
	return type;
}

int area_header::getNoOfLetters() const
{
	return noOfLetters;
}

int area_header::getNoOfUnread()
{
	return (mm->driverList->getReadObject(driver))->
		getNoOfUnread(num - mm->driverList->getOffset(driver));
}

int area_header::getNoOfMarked()
{
	return (mm->driverList->getReadObject(driver))->
		getNoOfMarked(num - mm->driverList->getOffset(driver));
}

int area_header::getNoOfPersonal() const
{
	return noOfPersonal;
}

bool area_header::getUseAlias() const
{
	return !(!(type & ALIAS));
}

bool area_header::isCollection() const
{
	return !(!(type & COLLECTION));
}

bool area_header::isReplyArea() const
{
	return !(!(type & REPLYAREA));
}

bool area_header::isActive() const
{
	return !(!(type & (ACTIVE | ADDED | DROPPED)));
}

bool area_header::isNetmail() const
{
	return (type & NETMAIL) && !(type & INTERNET);
}

bool area_header::isInternet() const
{
	return (type & NETMAIL) && (type & INTERNET);
}

bool area_header::isEmail() const
{
	return !(!(type & NETMAIL));
}

bool area_header::isUsenet() const
{
	return !(type & NETMAIL) && (type & INTERNET);
}

bool area_header::isLatin() const
{
	return !(!(type & LATINCHAR));
}

bool area_header::hasTo() const
{
	return !((type & INTERNET) && !(type & NETMAIL));
}

bool area_header::hasPublic() const
{
	return !(!(type & PUBLIC));
}

bool area_header::hasPrivate() const
{
	return !(!(type & PRIVATE));
}

int area_header::maxToLen() const
{
	return maxtolen;
}

int area_header::maxSubLen() const
{
	return maxsublen;
}

bool area_header::hasOffConfig() const
{
	return !(!(type & OFFCONFIG));
}

void area_header::Add()
{
	if (!(type & COLLECTION)) {
		if (type & DROPPED)
			type &= ~DROPPED;
		else
			if (!(type & ACTIVE) || !(type & SUBKNOWN))
				type |= ADDED;
	}
}

void area_header::Drop()
{
	if (!(type & COLLECTION)) {
		if (type & ADDED)
			type &= ~ADDED;
		else
			if ((type & ACTIVE) || !(type & SUBKNOWN))
				type |= DROPPED;
	}
}

// ---------------------------------------------------------------------------
// Arealist methods
// ---------------------------------------------------------------------------

area_list::area_list(mmail *mmA)
{
	mm = mmA;
	no = 0;

	int c = 0;
	do {
		no += mm->driverList->getDriver(no)->getNoOfAreas();
		c++;
	} while (c < mm->driverList->getNoOfDrivers());

	activeHeader = new int[no];
	areaHeader = new area_header *[no];

	specific_driver *actDriver;
	for (c = 0; c < no; c++) {
		actDriver = mm->driverList->getDriver(c);
		areaHeader[c] = actDriver->getNextArea();
	}

	current = 0;
	shortlist = false;
	relist();

	// 1. Find out what types of areas we have (i.e. qwk, usenet... ) 
	// 2. Create the appropriate driver objects
	// 3. Find out the number of areas for each type
	// 4. Allocate the memory for the area_header descriptions
	// 5. Fill the area headers
}

area_list::~area_list()
{
	while (no)
		delete areaHeader[--no];
	delete[] areaHeader;
	delete[] activeHeader;
}

void area_list::relist()
{
	noActive = 0;
	shortlist = !shortlist;
	int c = current;

	for (current = 0; current < no; current++)
		if (areaHeader[current]->isActive() || !shortlist)
			activeHeader[noActive++] = current;
	current = c;
}

void area_list::updatePers()
{
	// This routine makes some assumptions -- that there's at most one
	// PERS area, and that if present, it's the second area -- that
	// are valid as the program is currently written, but that are not
	// made elsewhere in this class.

	specific_driver *actDriver =
		mm->driverList->getDriver(REPLY_AREA + 1);
	if (actDriver->hasPersArea()) {
		int c = current;
		current = REPLY_AREA + 1;
		if (isCollection() && !isReplyArea()) {
			letter_list *ll = mm->letterList;
			getLetterList();
			delete mm->letterList;
			mm->letterList = ll;
		}
		current = c;
	}
}

bool area_list::isShortlist()
{
	return shortlist;
}

const char *area_list::getShortName() const
{
	return areaHeader[current]->getShortName();
}

const char *area_list::getName() const
{
	return areaHeader[current]->getName();
}

const char *area_list::getName(int area)
{
	if ((area < 0) || (area >= no))
		fatalError("Internal error in area_list::getName");
	return areaHeader[area]->getName();
}

const char *area_list::getDescription() const
{
	return areaHeader[current]->getDescription();
}

const char *area_list::getDescription(int area)
{
	if ((area < 0) || (area >= no))
		fatalError("Internal error in area_list::getDescription");
	return areaHeader[area]->getDescription();
}

const char *area_list::getAreaType() const
{
	return areaHeader[current]->getAreaType();
}

unsigned area_list::getType() const
{
	return areaHeader[current]->getType();
}

int area_list::getNoOfLetters() const
{
	return areaHeader[current]->getNoOfLetters();
}

int area_list::getNoOfUnread() const
{
	return areaHeader[current]->getNoOfUnread();
}

int area_list::getNoOfMarked() const
{
	return areaHeader[current]->getNoOfMarked();
}

int area_list::getNoOfPersonal() const
{
	return areaHeader[current]->getNoOfPersonal();
}

void area_list::getLetterList()
{
	mm->letterList = new letter_list(mm, current, isCollection() &&
					!isReplyArea());
}

int area_list::noOfAreas() const
{
	return no;
}

int area_list::noOfActive() const
{
	return noActive;
}

void area_list::gotoArea(int currentA)
{
	if ((currentA >= 0) && (currentA < no))
		current = currentA;
}

void area_list::gotoActive(int activeA)
{
	if ((activeA >= 0) && (activeA < noActive))
		current = activeHeader[activeA];
}

int area_list::getAreaNo() const
{
	return current;
}

int area_list::getActive()
{
	int c;

	for (c = 0; c < noActive; c++)
		if (activeHeader[c] >= current)
			break;
	return c;
}

void area_list::enterLetter(int areaNo, const char *from, const char *to,
			const char *subject, const char *replyID,
			const char *newsgrp, int replyTo, bool privat,
			net_address &netAddress, const char *filename,
			int length)
{
	reply_driver *replyDriver = mm->driverList->getReplyDriver();

	gotoArea(areaNo);
	letter_header newLetter(mm, subject, to, from, "", replyID,
		replyTo, 0, 0, areaNo, privat, 0, replyDriver,
		netAddress, isLatin(), newsgrp);

	replyDriver->enterLetter(newLetter, filename, length);

	refreshArea();
}

void area_list::killLetter(int letterNo)
{
	mm->driverList->getReplyDriver()->killLetter(letterNo);
	refreshArea();
}

void area_list::refreshArea()
{
	delete areaHeader[REPLY_AREA];

	areaHeader[REPLY_AREA] =
		mm->driverList->getReplyDriver()->refreshArea();
	if (current == REPLY_AREA)
		mm->letterList->rrefresh();
}

bool area_list::getUseAlias() const
{
	return areaHeader[current]->getUseAlias();
}

bool area_list::isCollection() const
{
	return areaHeader[current]->isCollection();
}

bool area_list::isReplyArea() const
{
	return areaHeader[current]->isReplyArea();
}

bool area_list::isEmail() const
{
	return areaHeader[current]->isEmail();
}

bool area_list::isNetmail() const
{
	return areaHeader[current]->isNetmail();
}

int area_list::findNetmail() const
{
	int c;

	for (c = 0; c < no; c++)
		if (areaHeader[c]->isNetmail())
			break;

	return (c < no) ? c : -1;
}

bool area_list::isInternet() const
{
	return areaHeader[current]->isInternet();
}

int area_list::findInternet() const
{
	int c;

	for (c = 0; c < no; c++)
		if (areaHeader[c]->isInternet())
			break;

	return (c < no) ? c : -1;
}

bool area_list::isUsenet() const
{
	return areaHeader[current]->isUsenet();
}

bool area_list::isLatin() const
{
	return areaHeader[current]->isLatin();
}

bool area_list::isLatin(int area)
{
	if ((area < 0) || (area >= no))
		fatalError("Internal error in area_list::isLatin");
	return areaHeader[area]->isLatin();
}

bool area_list::hasTo() const
{
	return areaHeader[current]->hasTo();
}

bool area_list::hasPublic() const
{
	return areaHeader[current]->hasPublic();
}

bool area_list::hasPrivate() const
{
	return areaHeader[current]->hasPrivate();
}

int area_list::maxToLen() const
{
	return areaHeader[current]->maxToLen();
}

int area_list::maxSubLen() const
{
	return areaHeader[current]->maxSubLen();
}

bool area_list::hasOffConfig() const
{
	return areaHeader[current]->hasOffConfig();
}

void area_list::Add()
{
	areaHeader[current]->Add();
}

void area_list::Drop()
{
	areaHeader[current]->Drop();
}

bool area_list::anyChanged() const
{
	for (int c = 0; c < no; c++)
		if (areaHeader[c]->getType() & (ADDED | DROPPED))
			return true;
	return false;
}
