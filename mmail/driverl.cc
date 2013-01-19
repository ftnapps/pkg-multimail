/*
 * MultiMail offline mail reader
 * driver_list

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>,
                    Robert Vukovic <vrobert@uns.ns.ac.yu>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "bw.h"
#include "qwk.h"
#include "omen.h"
#include "soup.h"
#include "opx.h"

enum pktype {PKT_QWK, PKT_BW, PKT_OMEN, PKT_SOUP, PKT_OPX, PKT_UNDEF};

enum {PERSONAL = 1, TEARLINE = 2};

// ------------------------------------------------
// Virtual specific_driver and reply_driver methods
// ------------------------------------------------

specific_driver::~specific_driver()
{
}

bool specific_driver::hasPersArea()
{
	return false;
}

bool specific_driver::isLatin()
{
	return false;
}

const char *specific_driver::saveOldFlags()
{
	return 0;
}

reply_driver::~reply_driver()
{
}

// ------------------
// DriverList methods
// ------------------

driver_list::driver_list(mmail *mm)
{
	pktype mode = PKT_UNDEF;
	file_list *wl = mm->workList;

	// This is the new way to set the packet type
	if (wl->exists("control.dat") && wl->exists("messages.dat"))
		mode = PKT_QWK;
	else
		if (wl->exists(".inf"))
			mode = PKT_BW;
		else
			if (wl->exists("brdinfo.dat"))
				mode = PKT_OPX;
			else
				if (wl->exists("areas"))
					mode = PKT_SOUP;
				else
					if (wl->exists("system"))
						mode = PKT_OMEN;

	switch (mode) {
	case PKT_BW:
		driverList[1].driver = new bluewave(mm);
		attributes = PERSONAL;
		break;
	case PKT_QWK:
		driverList[1].driver = new qwkpack(mm);
		attributes = TEARLINE;
		break;
	case PKT_OMEN:
		driverList[1].driver = new omen(mm);
		attributes = TEARLINE;
		break;
	case PKT_SOUP:
		driverList[1].driver = new soup(mm);
		attributes = 0;
		break;
	case PKT_OPX:
		driverList[1].driver = new opxpack(mm);
		attributes = TEARLINE;
		break;
	default:;
		driverList[1].driver = 0;
	}

	if (mode != PKT_UNDEF)
		driverList[1].read = new main_read_class(mm,
			driverList[1].driver);
	else
		driverList[1].read = 0;

	switch (mode) {
	case PKT_BW:
		driverList[0].driver = new bwreply(mm, driverList[1].driver);
		break;
	case PKT_QWK:
		driverList[0].driver = new qwkreply(mm, driverList[1].driver);
		break;
	case PKT_OMEN:
		driverList[0].driver = new omenrep(mm, driverList[1].driver);
		break;
	case PKT_SOUP:
		driverList[0].driver = new souprep(mm, driverList[1].driver);
		break;
	case PKT_OPX:
		driverList[0].driver = new opxreply(mm, driverList[1].driver);
		break;
	default:;
		driverList[0].driver = 0;
	}

	if (mode != PKT_UNDEF)
		driverList[0].read = new reply_read_class(mm,
			driverList[0].driver);
	else
		driverList[0].read = 0;

	driverList[0].offset = REPLY_AREA;
	driverList[1].offset = REPLY_AREA + 1;

	noOfDrivers = (mode != PKT_UNDEF) ? 2 : 0;
};

driver_list::~driver_list()
{
	while (noOfDrivers--) {
		delete driverList[noOfDrivers].read;
		delete driverList[noOfDrivers].driver;
	}
};

int driver_list::getNoOfDrivers() const
{
	return noOfDrivers;
};

specific_driver *driver_list::getDriver(int areaNo)
{
	int c = (areaNo != REPLY_AREA);
	return driverList[c].driver;
};

reply_driver *driver_list::getReplyDriver()
{
	return (reply_driver *) driverList[0].driver;
};

read_class *driver_list::getReadObject(specific_driver *driver)
{
	int c = (driver == driverList[1].driver);
	return driverList[c].read;
};

int driver_list::getOffset(specific_driver *driver)
{
	int c = (driver == driverList[1].driver);
	return driverList[c].offset;
};

bool driver_list::hasPersonal() const
{
	return !(!(attributes & PERSONAL));
}

bool driver_list::useTearline() const
{
	return !(!(attributes & TEARLINE));
}
