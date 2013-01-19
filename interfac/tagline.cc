/*
 * MultiMail offline mail reader
 * tagline selection, editing

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "interfac.h"

tagline::tagline(const char *tag)
{
	if (tag)
		strncpy(text, tag, TAGLINE_LENGTH);
	next = 0;
}

TaglineWindow::TaglineWindow()
{
	nodraw = true;
	NumOfTaglines = 0;
}

TaglineWindow::~TaglineWindow()
{
	DestroyChain();
}

void TaglineWindow::MakeActive()
{
	nodraw = false;

	srand((unsigned) time(0));

	list_max_y = LINES - 15;

	int xwidth = COLS - 2;
	if (xwidth > 78)
		xwidth = 78;
	list_max_x = xwidth - 2;
	top_offset = 1;

	borderCol = C_TBBACK;

	list = new InfoWin(LINES - 10, xwidth, 5, borderCol,
		"Select tagline", C_TTEXT, 5, top_offset);

	int x = xwidth / 3 + 1;
	int y = list_max_y + 1;

	list->attrib(C_TKEYSTEXT);
	list->horizline(y, xwidth - 2);
	list->put(++y, 2, "Q");
	list->put(y, x, "R");
	list->put(y, x * 2, "Enter");
	list->put(++y, 2, "K");
	list->put(y, x, "A");
	//list->put(y, x * 2, " /, .");
	list->put(y, x * 2 + 4, "E");
	list->attrib(C_TTEXT);
	list->put(y, 3, ": Kill current tagline");
	list->put(y, x + 1, ": Add new tagline");
	//list->put(y, x * 2 + 5, ": search / next");
	list->put(y, x * 2 + 5, ": Edit tagline");
	list->put(--y, 3, ": don't apply tagline");
	list->put(y, x + 1, ": Random select tagline");
	list->put(y, x * 2 + 5, ": apply tagline");
	DrawAll();
}

void TaglineWindow::Delete()
{
	delete list;
	nodraw = true;
}

bool TaglineWindow::extrakeys(int key)
{
	bool end = false;

	switch (key) {
	case 'A':
		EnterTagline();
		break;
	case 'E':
		EditTagline();
		break;
	case 'R':
		RandomTagline();
		break;
	case KEY_DC:
	case 'K':
		if (highlighted)
			kill();
		break;
	}
	return end;
}

void TaglineWindow::RandomTagline()
{
	int i = rand() / (RAND_MAX / NumOfTaglines);

	Move(HOME);
	for (int j = 1; j <= i; j++)
		Move(DOWN);
	DrawAll();
}

void TaglineWindow::EnterTagline(const char *tag)
{
	FILE *fd;
	char newtagline[TAGLINE_LENGTH + 1];
	int y;

	Move(END);
	if (NumOfTaglines >= list_max_y) {
		y = list_max_y;
		position++;
	} else
		y = NumOfTaglines + 1;
	active++;

	if (!nodraw) {
		NumOfTaglines++;
		Draw();
		NumOfTaglines--;
	} else {
		int xwidth = COLS - 2;
		if (xwidth > 78)
			xwidth = 78;
		list = new InfoWin(5, xwidth, (LINES - 5) >> 1, C_TBBACK);
		list->attrib(C_TTEXT);
		list->put(1, 1, "Enter new tagline:");
		list->update();
	}

	strcpy(newtagline, tag ? tag : "");

	if (list->getstring(nodraw ? 2 : y, 1, newtagline,
	    TAGLINE_LENGTH, C_TENTER, C_TENTERGET)) {

		cropesp(newtagline);

		if (newtagline[0]) {

			//check dupes; also move curr to end of list:
			bool found = false;

			curr = &head;
			while (curr->next && !found) {
				curr = curr->next;
				found = !strcmp(newtagline, curr->text);
			}

			if (!found) {
				curr->next = new tagline(newtagline);
				if ((fd = fopen(tagname, "at"))) {
					fputs(newtagline, fd);
					fputc('\n', fd);
					fclose(fd);
				}
				NumOfTaglines++;

				delete[] taglist;
				MakeChain();
			} else
				interface->nonFatalError("Already in file");
		}
	}
	Move(END);

	if (!nodraw) {
		DrawAll();
		doupdate();
	} else
		list->update();
}

void TaglineWindow::EditTagline()
{
	char newtagline[TAGLINE_LENGTH + 1];

	strcpy(newtagline, getCurrent());
	if (list->getstring(active - position + 1, 1, newtagline,
	    TAGLINE_LENGTH, C_TENTER, C_TENTERGET)) {
		cropesp(newtagline);
		if (newtagline[0])
			strcpy(taglist[active]->text, newtagline);
	}
	WriteFile(false);
	Draw();
}

void TaglineWindow::kill()
{
	if (interface->WarningWindow("Remove this tagline?")) {
		if (active)
			taglist[active - 1]->next = highlighted->next;
		else
			head.next = highlighted->next;

		if (position)
			position--;

		NumOfTaglines--;

		delete highlighted;
		delete[] taglist;

		MakeChain();

		WriteFile(false);
	}
	Delete();
	MakeActive();
}

bool TaglineWindow::ReadFile()
{
	FILE *fd;
	char newtag[TAGLINE_LENGTH + 1];
	bool flag = (fd = fopen(tagname, "rt"));

	if (flag) {
		char *end;

		curr = &head;
		do {
			end = myfgets(newtag, sizeof newtag, fd);

			if (end && (newtag[0] != '\n')) {
				if (*end == '\n')
					*end = '\0';
				curr->next = new tagline(newtag);
				curr = curr->next;
				NumOfTaglines++;
			}
		} while (end);
		fclose(fd);
	}
	return flag;
}

void TaglineWindow::WriteFile(bool message)
{
	FILE *tagx;

	if (message)
		printf("Creating %s...\n", tagname);

	if ((tagx = fopen(tagname, "wt"))) {
		for (int x = 0; x < NumOfTaglines; x++) {
			fputs(taglist[x]->text, tagx);
			fputc('\n', tagx);
		}
		fclose(tagx);
	}
}

void TaglineWindow::MakeChain()
{
	taglist = new tagline *[NumOfTaglines + 1];

	if (NumOfTaglines) {
		curr = head.next;
		int c = 0;
		while (curr) {
			taglist[c++] = curr;
			curr = curr->next;
		}
	}

	taglist[NumOfTaglines] = 0;	// hack for EnterTagline
}

void TaglineWindow::DestroyChain()
{
	while (NumOfTaglines)
		delete taglist[--NumOfTaglines];
	delete[] taglist;
}

void TaglineWindow::oneLine(int i)
{
	int z = position + i;
	curr = (z < NumOfTaglines) ? taglist[z] : 0;

	if (z == active)
		highlighted = curr;

	sprintf(list->lineBuf, "%-76.76s", curr ? curr->text : " ");

	DrawOne(i, C_TLINES);
}

searchret TaglineWindow::oneSearch(int x, const char *item, int ignore)
{
	ignore = ignore;
	return searchstr(taglist[x]->text, item) ? True : False;
}

int TaglineWindow::NumOfItems()
{
	return NumOfTaglines;
}

// Create tagline file if it doesn't exist.
void TaglineWindow::Init()
{
	// Default taglines:
#include "tagline.h"

	tagname = mm.resourceObject->get(TaglineFile);

	bool useDefault = !ReadFile();

	if (useDefault) {
		curr = &head;
		for (const char **p = defaultTags; *p; p++) {
			curr->next = new tagline(*p);
			curr = curr->next;
			NumOfTaglines++;
		}
	}

	MakeChain();

	if (useDefault)
		WriteFile(true);
}

const char *TaglineWindow::getCurrent()
{
	return taglist[active]->text;
}
