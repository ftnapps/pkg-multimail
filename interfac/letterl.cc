/*
 * MultiMail offline mail reader
 * message list

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "interfac.h"

LetterListWindow::LetterListWindow()
{
	lsorttype = mm.resourceObject->getInt(LetterSort);
}

void LetterListWindow::listSave()
{
	static const char *saveopts[] = { "Marked", "All", "This one",
		"Quit" };

	int marked = !(!mm.areaList->getNoOfMarked());

	int status = interface->WarningWindow("Save which?", saveopts +
		!marked, 3 + marked);
	if (status) {
		bool saveok = interface->letterwindow.Save(status);
		if ((status == 1) && saveok)
			Move(DOWN);
	}
}

void LetterListWindow::Next()
{
	do {
		Move(DOWN);
		mm.letterList->gotoActive(active);
	} while (mm.letterList->getRead() && ((active + 1) < NumOfItems()));
}

void LetterListWindow::FirstUnread()
{
	position = 0;
	active = 0;
}

void LetterListWindow::Prev()
{
	do {
		Move(UP);
		mm.letterList->gotoActive(active);
	} while (mm.letterList->getRead() && (active > 0));
}

int LetterListWindow::NumOfItems()
{
	return mm.letterList->noOfActive();
}

void LetterListWindow::oneLine(int i)
{
	mm.letterList->gotoActive(position + i);

	int st = mm.letterList->getStatus();

	char *p = list->lineBuf;
	p += sprintf(p, format, (st & MS_MARKED) ? 'M' : ' ',
	    (st & MS_REPLIED) ? '~' : ' ', (st & MS_READ) ? '*' : ' ',
		mm.letterList->getMsgNum(), mm.letterList->getFrom(),
		    mm.letterList->getTo(),
			stripre(mm.letterList->getSubject()));

	if (mm.areaList->isCollection()) {
		const char *origArea = mm.letterList->getNewsgrps();
		if (!origArea)
			origArea = mm.areaList->
			    getDescription(mm.letterList->getAreaID());
		if (origArea)
			sprintf(p - 15, " %-13.13s ", origArea);
	}

	chtype linecol = mm.letterList->isPersonal() ? C_LLPERSONAL :
		C_LISTWIN;

	letterconv_in(list->lineBuf);
	DrawOne(i, (st & MS_READ) ? noemph(linecol) : emph(linecol));
}

searchret LetterListWindow::oneSearch(int x, const char *item, int mode)
{
	const char *s;
	searchret retval;

	mm.letterList->gotoActive(x);

	s = searchstr(mm.letterList->getFrom(), item);
	if (!s) {
		s = searchstr(mm.letterList->getTo(), item);
		if (!s)
			s = searchstr(mm.letterList->getSubject(), item);
	}

	retval = s ? True : False;
	if (!s && (mode == s_fulltext)) {
		interface->changestate(letter);
		interface->letterwindow.setPos(-1);
		retval = interface->letterwindow.search(item);
		if (retval != True)
			interface->changestate(letterlist);
	}

	return retval;
}

void LetterListWindow::MakeActive()
{
	char *topline, tformat[50];
	int tot, maxFromLen, maxToLen, maxSubjLen;

	topline = new char[COLS + 1];

	tot = COLS - 19;
	maxSubjLen = tot / 2;
	tot -= maxSubjLen;
	maxToLen = tot / 2;
	maxFromLen = tot - maxToLen;

	if (!mm.areaList->hasTo() || (mm.areaList->isCollection() &&
	    !mm.areaList->isReplyArea())) {
		maxSubjLen += maxToLen;
		maxToLen = 0;
	}

	if (mm.areaList->isReplyArea()) {
		maxSubjLen += maxFromLen;
		maxFromLen = 0;
	}

	sprintf(format, "%%c%%c%%c%%6d  %%-%d.%ds %%-%d.%ds %%-%d.%ds",
		maxFromLen, maxFromLen, maxToLen, maxToLen,
			maxSubjLen, maxSubjLen);

	interface->areas.Select();

	sprintf(tformat, "Letters in %%.%ds (%%d)", COLS - 30);
	sprintf(topline, tformat, mm.areaList->getDescription(),
		NumOfItems());
	areaconv_in(topline);

	list_max_y = (NumOfItems() < LINES - 11) ? NumOfItems() : LINES - 11;
	list_max_x = COLS - 6;
	top_offset = 2;

	borderCol = C_LLBBORD;

	list = new InfoWin(list_max_y + 3, COLS - 4, 2, borderCol,
		topline, C_LLTOPTEXT2);

	list->attrib(C_LLTOPTEXT1);
	list->put(0, 3, "Letters in ");

	sprintf(tformat, "   Msg#  %%-%d.%ds %%-%d.%ds %%-%d.%ds",
		maxFromLen, maxFromLen, maxToLen, maxToLen,
			maxSubjLen, maxSubjLen);
	list->attrib(C_LLHEAD);
	sprintf(topline, tformat, "From", "To", "Subject");
	list->put(1, 3, topline);

	delete[] topline;

	if (mm.areaList->isCollection())
		list->put(1, COLS - 19, "Area");
	list->touch();

	DrawAll();
	Select();
}

void LetterListWindow::Select()
{
	mm.letterList->gotoActive(active);
}

void LetterListWindow::ResetActive()
{
        active = mm.letterList->getActive();
}

void LetterListWindow::Delete()
{
	delete list;
}

bool LetterListWindow::extrakeys(int key)
{
	Select();
	switch (key) {
	case 'U':
	case 'M':	// Toggle read/unread and marked from letterlist
		mm.letterList->setStatus(mm.letterList->getStatus() ^
			((key == 'U') ? MS_READ : MS_MARKED));
		interface->setAnyRead();
		Move(DOWN);
		Draw();
		break;
	case 'E':
		if (mm.areaList->isReplyArea())
			interface->letterwindow.KeyHandle('E');
		else
		    if (!mm.areaList->isCollection()) {
			if (mm.areaList->isEmail())
				interface->addressbook();
			interface->letterwindow.set_Letter_Params(
				mm.areaList->getAreaNo(), 'E');
			interface->letterwindow.EnterLetter();
		    }
		break;
	case 2:
	case 6:
	case KEY_DC:
	case 'K':
		if (mm.areaList->isReplyArea())
			interface->letterwindow.KeyHandle(key);
		break;
	case 'L':
		mm.letterList->relist();
		ResetActive();
		interface->redraw();
		break;
	case '$':
		mm.letterList->resort();
		DrawAll();
		break;
	case 'S':
		listSave();
		interface->redraw();
	}
	return false;
}
