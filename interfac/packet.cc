/*
 * MultiMail offline mail reader
 * packet list window, vanity plate

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "interfac.h"

void Welcome::MakeActive()
{
	window = new ShadowedWin(6, 50, 2, C_WBORDER);
	window->attrib(C_WELCOME1);
	window->put(1, 7,
		"Welcome to " MM_NAME " Offline Reader!");
	window->attrib(C_WELCOME2);
	window->put(3, 2,
		"Copyright (c) 1999 William McBrine, Kolossvary"); 
	window->put(4, 7,
		"Tamas, Toth Istvan, John Zero, et al.");
	window->touch();
}

void Welcome::Delete()
{
	delete window;
}

PacketListWindow::PacketListWindow()
{
	origDir = strdupplus(mm.resourceObject->get(PacketDir));
	sorttype = mm.resourceObject->getInt(PacketSort);
	packetList = dirList = 0;
	newList();
	if (noFiles)			// If there are any files,
		active = noDirs;	// set active to the first one.
}

PacketListWindow::~PacketListWindow()
{
	delete packetList;
	delete dirList;
	delete[] origDir;
}

void PacketListWindow::newList()
{
	delete packetList;
	delete dirList;

	mm.resourceObject->set(oldPacketName, 0);

	time(&currTime);
	currDir = mm.resourceObject->get(PacketDir);

	dirList = new file_list(currDir, false, true);
	packetList = new file_list(currDir, sorttype);

	noDirs = dirList->getNoOfFiles();
	noFiles = packetList->getNoOfFiles();

	if (!NumOfItems())
		error();
}

void PacketListWindow::error()
{
	char tmp[512];

	sprintf(tmp, "No packets found.\n\n"
		"Please place any packets to be read in:\n%s", currDir);
	fatalError(tmp);
}

void PacketListWindow::MakeActive()
{
	const int stline = 9;
	list_max_y = LINES - (stline + 9);
	list_max_x = 48;
	top_offset = 2;

	borderCol = C_PBBACK;

	if (list_max_y > NumOfItems())
		list_max_y = NumOfItems();

	welcome.MakeActive();

	list = new InfoWin(list_max_y + 3, 50, stline, borderCol, currDir,
		C_PHEADTEXT);
	list->attrib(C_PHEADTEXT);
	list->put(1, 3, "Packet                  Size    Date");
	list->touch();
	DrawAll();
}

int PacketListWindow::NumOfItems()
{
	return noDirs + noFiles;
}

void PacketListWindow::Delete()
{
	delete list;
	welcome.Delete();
}

void PacketListWindow::oneLine(int i)
{
	char *tmp = list->lineBuf;
	int absPos = position + i;
	time_t tmpt;

	if (absPos < noDirs) {
		dirList->gotoFile(absPos);

		absPos = sprintf(tmp, "  <%.32s", dirList->getName());
		char *tmp2 = tmp + absPos;
		*tmp2++ = '>';

		absPos = 32 - absPos;
		while (--absPos)
			*tmp2++ = ' ';
		tmpt = dirList->getDate();
	} else {
		packetList->gotoFile(absPos - noDirs);

		const char *tmp2 = packetList->getName();

		strcpy(tmp, "          ");

		if (*tmp2 == '.')
			sprintf(&tmp[2], "%-20.20s", tmp2);
		else {
			for (int j = 2; *tmp2 && (*tmp2 != '.') &&
				(j < 10); j++)
					tmp[j] = *tmp2++;

			sprintf(&tmp[10], "%-10.10s", tmp2);
		}

		sprintf(&tmp[20], "%12u",
			(unsigned) packetList->getSize());

		tmpt = packetList->getDate();
	}
	long dtime = currTime - tmpt;

	// 15000000 secs = approx six months (use year if older):
	strftime(&tmp[32], 17, ((dtime < 0 || dtime > 15000000) ?
		"  %b %d  %Y  " : "  %b %d %H:%M  "), localtime(&tmpt));

	DrawOne(i, C_PLINES);
}

searchret PacketListWindow::oneSearch(int x, const char *item, int mode)
{
	file_list *foo;
	const char *s;
	searchret retval;

	if (x < noDirs)
		foo = dirList;
	else {
		foo = packetList;
		x -= noDirs;
	}
	foo->gotoFile(x);

	s = foo->getName();
	retval = searchstr(s, item) ? True : False;

	if ((retval == False) && (foo == packetList) &&
	    (mode < s_pktlist)) {
		int oldactive = active;
		active = x + noDirs;
		if (OpenPacket() == PKT_OK) {
			mm.checkForReplies();
			mm.openReply();

			interface->redraw();
			ShadowedWin searchsay(3, 31, (LINES >> 1) - 2,
				C_WTEXT);
			searchsay.put(1, 2, "Searching (ESC to abort)...");
			searchsay.update();

			mm.areaList = new area_list(&mm);
			mm.areaList->relist();
			interface->changestate(arealist);
			interface->areas.setActive(-1);
			retval = interface->areas.search(item, mode);
			if (retval != True) {
				active = oldactive;
				mm.Delete();
				interface->changestate(packetlist);
			}
		} else
			active = oldactive;
	}

	return retval;
}

void PacketListWindow::Select()
{
	if (active < noDirs)
		dirList->gotoFile(active);
	else
		packetList->gotoFile(active - noDirs);
}

bool PacketListWindow::back()
{
	bool end = false;

	if (strcmp(currDir, origDir)) {
		mm.resourceObject->set(PacketDir, dirList->changeDir(".."));
		active = position = 0;
		extrakeys('U');
	} else
		end = true;
	return end;
}

bool PacketListWindow::extrakeys(int key)
{
	bool end = false;

	switch (key) {
	case 'S':
	case '$':
		packetList->resort();
		sorttype = !sorttype;
		DrawAll();
		break;
	case 'G':
		gotoDir();
		break;
	case 'R':
		renamePacket();
		break;
	case KEY_DC:
	case 'K':
		killPacket();
		break;
	case 'U':
		newList();
		interface->redraw();
	}
	return end;
}

void PacketListWindow::gotoDir()
{
	char pathname[70];
	pathname[0] = '\0';

	if (interface->savePrompt("New directory:", pathname) &&
	    pathname[0]) {
		mm.resourceObject->set(PacketDir,
			dirList->changeDir(pathname));
		active = position = 0;
		extrakeys('U');
	} else
		interface->redraw();
}

void PacketListWindow::renamePacket()
{
	if (active >= noDirs) {
		Select();

		char question[60], answer[60];
		sprintf(question, "New filename for %.39s:",
			packetList->getName());
		answer[0] = '\0';

		if (interface->savePrompt(question, answer) &&
		    answer[0]) {
			packetList->changeName(answer);
			extrakeys('U');
		} else
			interface->redraw();
	}
}

void PacketListWindow::killPacket()
{
	if (active >= noDirs) {
		Select();

		char tmp[128];
		sprintf(tmp, "Do you really want to delete %.90s?",
			packetList->getName());

		if (interface->WarningWindow(tmp)) {
			packetList->kill();
			noFiles = packetList->getNoOfFiles();
		}
		if (NumOfItems())
			interface->redraw();
		else
			error();
	}
}

pktstatus PacketListWindow::OpenPacket()
{
	Select();
	if (active < noDirs) {
		mm.resourceObject->set(PacketDir, dirList->changeDir());
		newList();
		position = 0;
		active = noFiles ? noDirs : 0;	// active = first file
		interface->redraw();
		return NEW_DIR;
	} else
		return mm.selectPacket(packetList->getName());
}
