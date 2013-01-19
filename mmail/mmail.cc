/*
 * MultiMail offline mail reader
 * mmail class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>,
                    Robert Vukovic <vrobert@uns.ns.ac.yu>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "mmail.h"
#include "compress.h"
#include "../interfac/error.h"

net_address::net_address()
{
	isSet = isInternet = false;
	zone = 0;
	inetAddr = 0;
}

net_address::net_address(net_address &x)
{
	copy(x);
}

net_address::~net_address()
{
	if (isSet && isInternet)
		delete[] inetAddr;
}

bool net_address::operator==(net_address &x)
{
	if (isInternet != x.isInternet)
		return false;

	if (isInternet)
		return !strcmp(inetAddr, x.inetAddr);
	else
		return (zone == x.zone) && (net == x.net) &&
			(node == x.node) && (point == x.point);
}

net_address &net_address::operator=(const char *source)
{
	isInternet = source ? !(!strchr(source, '@')) : false;

	if (isInternet) {
		delete[] inetAddr;
		inetAddr = strdupplus(source);
		isSet = true;
	} else {
		if (sscanf(source ? source : "", "%u:%u/%u.%u",
			&zone, &net, &node, &point) == 3)
				point = 0;

		isSet = !(!zone);
	}

	return *this;
}

void net_address::copy(net_address &x)
{
	isSet = x.isSet;
	if (isSet) {
		isInternet = x.isInternet;
		if (isInternet)
			inetAddr = strdupplus(x.inetAddr);
		else {
			zone = x.zone;
			net = x.net;
			node = x.node;
			point = x.point;
		}
	}
}

net_address &net_address::operator=(net_address &x)
{
	copy(x);
	return *this;
}

net_address::operator const char *()
{
	static char netText[25];

	if (isSet)
		if (isInternet)
			return inetAddr;
		else
			if (point)
				sprintf(netText, "%u:%u/%u.%u",
					zone, net, node, point);
			else
				sprintf(netText, "%u:%u/%u",
					zone, net, node);
	else
		netText[0] = '\0';

	return netText;
}

mmail::mmail()
{
	resourceObject = new resource();
};

mmail::~mmail()
{
	delete resourceObject;
};

void mmail::Delete()
{
	delete areaList;
	delete driverList;
	delete workList;
}

// Open a packet
pktstatus mmail::selectPacket(const char *packetName)
{
	pktstatus result;
	char fname[256];

	const char *x = strrchr(packetName, '/');
	if (!x)
		x = strrchr(packetName, '\\');
	if (x) {
		int len = x - packetName;
		strncpy(fname, packetName, len);
		fname[len] = '\0';

		mychdir(error.getOrigDir());
		mychdir(fname);

		mygetcwd(fname);
		resourceObject->set(PacketDir, fname);
		packetName = x + 1;
	}
	resourceObject->set(PacketName, packetName);

	// Uncompression is done here
	sprintf(fname, "%s/%s", resourceObject->get(PacketDir),
		packetName);

	if (!resourceObject->get(oldPacketName) ||
	    strcmp(packetName, resourceObject->get(oldPacketName))) {
		resourceObject->set(oldPacketName, packetName);
		result = uncompressFile(resourceObject, fname,
		    resourceObject->get(WorkDir), true);
		if (result != PKT_OK)
			return result;
	}

	workList = new file_list(resourceObject->get(WorkDir));
	driverList = new driver_list(this);

	if (!driverList->getNoOfDrivers()) {
		delete driverList;
		delete workList;
		return PTYPE_UNK;
	}
	return PKT_OK;
};

// Save last read pointers
bool mmail::saveRead()
{
	return driverList->getReadObject(driverList->getDriver(REPLY_AREA
		+ 1))->saveAll();
};

// Get the BBS' "new files" list, if available
file_header *mmail::getFileList()
{
	return (driverList->getDriver(REPLY_AREA + 1))->getFileList();
}

// Get extra files, if available
file_header **mmail::getBulletins()
{
	return (driverList->getDriver(REPLY_AREA + 1))->getBulletins();
}

// Overall character set for packet (used for files and bulletins)
bool mmail::isLatin()
{
	return (driverList->getDriver(REPLY_AREA + 1))->isLatin();
}

// Is there a reply packet?
bool mmail::checkForReplies()
{
	return (driverList->getReplyDriver())->checkForReplies();
}

// Create a reply packet
bool mmail::makeReply()
{
        return (driverList->getReplyDriver())->makeReply();
}

void mmail::deleteReplies()
{
	(driverList->getReplyDriver())->deleteReplies();

	// to reset the "replyExists" flag (inelegant, I know):
	checkForReplies();	
}

void mmail::openReply()
{
	(driverList->getReplyDriver())->init();
}

bool mmail::getOffConfig()
{
	return (driverList->getReplyDriver())->getOffConfig();
}

