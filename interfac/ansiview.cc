/*
 * MultiMail offline mail reader
 * ANSI image/text viewer

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "interfac.h"

AnsiLine::AnsiLine(int space, AnsiLine *parent)
{
	next = 0;
	prev = parent;
	if (space) {
		text = new chtype[space];
		for (int i = 0; i < (space - 1); i++)
			text[i] = ' ' | C_ANSIBACK;
		text[space - 1] = 0;
	} else
		text = 0;
	length = 0;
}

AnsiLine::~AnsiLine()
{
	delete[] text;
}

AnsiLine *AnsiLine::getnext(int space)
{
	if (!next)
		if (space)
			next = new AnsiLine(space, this);
	return next;
}

AnsiLine *AnsiLine::getprev()
{
	return prev;
}

void AnsiWindow::DestroyChain()
{
	while (NumOfLines)
		delete linelist[--NumOfLines];
	delete[] linelist;
}

int AnsiWindow::getparm()
{
	char *parm;
	int value;

	if (escparm[0]) { 
		for (parm = escparm; (*parm != ';') && *parm; parm++);

		if (*parm == ';') {			// more params after
			*parm++ = '\0';
			if (parm == escparm + 1)	// empty param
				value = 1;
			else
				value = atoi(escparm);
			strcpy(escparm, parm);
		} else {				// last parameter
			value = atoi(escparm);
			escparm[0] = '\0';
		}
	} else						// empty last param
		value = 1;

	return value;
}

void AnsiWindow::colreset()
{
	cfl = crv = cbr = 0;
	ccf = COLOR_WHITE;
	ccb = COLOR_BLACK;
}

void AnsiWindow::colorset()
{
	static const int colortable[8] = {COLOR_BLACK, COLOR_RED,
		COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA,
		COLOR_CYAN, COLOR_WHITE};
	int tmp;

	while(escparm[0]) {
		tmp = getparm();
		switch (tmp) {
		case 0:			// reset colors
			colreset();
			break;
		case 1:			// bright
			cbr = 1;
			break;
		case 5:			// flashing
			cfl = 1;
			break;
		case 7:			// reverse
			crv = 1;
			break;
		default:
			if ((tmp > 29) && (tmp < 38))		// foreground
				ccf = colortable[tmp - 30];
			else if ((tmp > 39) && (tmp < 48))	// background
				ccb = colortable[tmp - 40];
		}
	}

	// Attribute set

	// Check bold and blinking:

	attrib = (cbr ? A_BOLD : 0) | (cfl ? A_BLINK : 0) |

	// If animating, check for color pair 0 (assumes COLOR_BLACK == 0),
	// and for remapped black-on-black:

		((!anim || ((ccb | ccf) || !(oldcolorx | oldcolory))) ?

		(crv ? REVERSE(ccf, ccb) : COL(ccf, ccb)) :

		(crv ? REVERSE(oldcolorx, oldcolory) : COL(oldcolorx,
		oldcolory)));

	// If not animating, mark color pair as used:

	if (!anim)
#ifdef __PDCURSES__
		if (crv)
			colorsused[(ccb << 3) + ccf] = true;
		else
#endif
			colorsused[(ccf << 3) + ccb] = true;
}

const unsigned char *AnsiWindow::escfig(const unsigned char *escode)
{
	char a[2];

	a[0] = *escode;
	a[1] = '\0';

	switch (a[0]) {
	case 'A':			// cursor up
		cpy -= getparm();
		if (cpy < 0)
			cpy = 0;
		break;
	case 'B':			// cursor down
		cpy += getparm();
		if (anim && (cpy > (LINES - 2)))
			cpy = LINES - 2;
		break;
	case 'C':			// cursor right
		cpx += getparm();
		if (cpx > (COLS - 1))
			cpx = COLS - 1;
		break;
	case 'D':			// cursor left
		cpx -= getparm();
		if (cpx < 0)
			cpx = 0;
		break;
	case 'J':			// clear screen
		if (getparm() == 2) {
			if (anim) {
				animtext->Clear(C_ANSIBACK);
				animtext->attrib(0);
			} else {
				cpy = NumOfLines - baseline - 1;
				checkpos();

				if (baseline < (NumOfLines - 1))
					baseline = NumOfLines - 1;
			}
			posreset();
		}
		break;
	case 'H':			// set cursor position
	case 'f':
		cpy = getparm() - 1;
		cpx = getparm() - 1;
		break;
	case 's':			// save position
		spx = cpx;
		spy = cpy;
		break;
	case 'u':			// restore position
		cpx = spx;
		cpy = spy;
		break;
	case 'm':			// attribute set (colors, etc.)
		colorset();
		break;

	// I know this is strange, but I think it beats the alternatives:

	default:
		if ((a[0] >= '0') && (a[0] <= '9'))

	case '=':			// for some weird mutant codes
	case '?':

	case ';':			// parameter separator
		{
			strcat(escparm, a);
			return escfig(++escode);
		}
	}
	return escode;
}

void AnsiWindow::posreset()
{
	cpx = cpy = lpy = spx = spy = 0;
}

void AnsiWindow::checkpos()
{
	if (cpy > lpy) {
		for (; lpy != cpy; lpy++)		 // move down and
			curr = curr->getnext(COLS + 1);  // allocate space
	} else
		if (cpy < lpy) {
			for (; lpy != cpy; lpy--)	 // move up
				curr = curr->getprev();
		}
	if ((cpy + 1) > (NumOfLines - baseline))
		NumOfLines = cpy + baseline + 1;
}

void AnsiWindow::update(unsigned char c)
{
	static const chtype acstrans[] = {
		ACS_BOARD, ACS_BOARD, ACS_CKBOARD, ACS_VLINE, ACS_RTEE,
		ACS_RTEE, ACS_RTEE, ACS_URCORNER, ACS_URCORNER, ACS_RTEE,
		ACS_VLINE, ACS_URCORNER, ACS_LRCORNER, ACS_LRCORNER,
		ACS_LRCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_BTEE,
		ACS_TTEE, ACS_LTEE, ACS_HLINE, ACS_PLUS, ACS_LTEE,
		ACS_LTEE, ACS_LLCORNER, ACS_ULCORNER, ACS_BTEE, ACS_TTEE,
		ACS_LTEE, ACS_HLINE, ACS_PLUS, ACS_BTEE, ACS_BTEE,
		ACS_TTEE, ACS_TTEE, ACS_LLCORNER, ACS_LLCORNER,
		ACS_ULCORNER, ACS_ULCORNER, ACS_PLUS, ACS_PLUS,
		ACS_LRCORNER, ACS_ULCORNER, ACS_BLOCK,

		// There are no good equivalents for these four:

		'_', '[', ']', '~'

		//ACS_BULLET, ACS_VLINE, ACS_VLINE, ACS_BULLET
	};


	if (!ansiAbort) {
		chtype ouch;

		if (isoConsole && !isLatin) {
			if ((c > 175) && (c < 224)) {
				ouch = acstrans[c - 176];

				// suppress or'ing of A_ALTCHARSET:
				c = ' ';

			} else {
				if (c & 0x80)
					c = (unsigned char)
						dos2isotab[c & 0x7f];
				ouch = c;
			}
		} else {
			if (!isoConsole && isLatin)
				if (c & 0x80)
					c = (unsigned char)
						iso2dostab[c & 0x7f];
			ouch = c;
		}

		if ((c < ' ') || ((c > 126) && (c < 160)))
			ouch |= A_ALTCHARSET;

		ouch |= attrib;

		int limit = LINES - 2;

		if (anim) {
			if (cpy > limit) {
				animtext->wscroll(cpy - limit);
				cpy = limit;
			}
			animtext->put(cpy, cpx++, ouch);
			animtext->update();
			ansiAbort |= (animtext->keypressed() != ERR);
		} else {
			checkpos();
				curr->text[cpx++] = ouch;
			if (cpx > (int) curr->length)
					curr->length = cpx;
		}
		if (cpx == COLS) {
			cpx = 0;
			cpy++;
		}
	}
}

void AnsiWindow::ResetChain()
{
	head = new AnsiLine();
	curr = head;
	posreset();
	curr = curr->getnext(COLS + 1);
	NumOfLines = 1;
	baseline = 0;
}

void AnsiWindow::MakeChain(const unsigned char *message)
{
	ansiAbort = false;
	if (!anim)
		ResetChain();
	attrib = C_ANSIBACK;
	colreset();

	while (*message && !ansiAbort) {
		switch (*message) {
		case 1:			// hidden lines (only in pos 0)
			if (!cpx) {
                                do
                                        message++;
                                while (*message && (*message != '\n'));
                                if (!*message)
                                        message--;
                        }
			break;
		case 8:			// backspace
			if (cpx)
				cpx--;
			break;
		case 10:
			cpy++;
		case 13:
			cpx = 0;
		case 7:			// ^G: beep
		case 26:		// ^Z: EOF for DOS
			break;
		case 12:		// form feed
			if (anim) {
				animtext->Clear(C_ANSIBACK);
				animtext->attrib(0);
				posreset();
			}
			break;
#ifndef __PDCURSES__			// unprintable control codes
		case 14:		// double musical note
			update(19);
			break;
		case 15:		// much like an asterisk
			update('*');
			break;
		case 155:		// ESC + high bit = slash-o,
			update('o');	// except in CP 437
			break;
#endif
		case '`':
		case 27:		// ESC
			if (message[1] == '[') {
				escparm[0] = '\0';
				message = escfig(message + 2);
			} else
				update('`');
			break;
		case '\t':		// TAB
			cpx = ((cpx / 8) + 1) * 8;
			while (cpx >= COLS) {
				cpx -= COLS;
				cpy++;
			}
			break;
		default:
			update(*message);
		}
		message++;
	}

	if (!anim && !ansiAbort) {
		linelist = new AnsiLine *[NumOfLines];
		curr = head->getnext();
		int i = 0;
		while (curr) {
			linelist[i++] = curr;
			curr = curr->getnext();
		}
		delete head;
	}
}

void AnsiWindow::statupdate(const char *intro)
{
	static const char *helpmsg = "F1 or ? - Help ",
		*mainstat = " ANSI View";
	char format[30], *tmp = new char[COLS + 1];

	sprintf(format, "%%s: %%-%d.%ds %%s", COLS - 28, COLS - 28);
	sprintf(tmp, format, (intro ? intro : mainstat), title, helpmsg);

	statbar->cursor_on();
	statbar->put(0, 0, tmp);
	statbar->update();
	statbar->cursor_off();

	delete[] tmp;
}

void AnsiWindow::animate()
{
	animtext = new Win(LINES - 1, COLS, 0, C_ANSIBACK);
	animtext->attrib(0);
	animtext->cursor_on();

	posreset();
	colreset();
	anim = true;

	statupdate(" Animating");

	MakeChain(source);

	if (!ansiAbort)
		statupdate("      Done");

	anim = false;
	while (!ansiAbort && (animtext->inkey() == ERR));

	animtext->cursor_off();
	delete animtext;

	header->wtouch();
	text->wtouch();
	statupdate();
}

void AnsiWindow::oneLine(int i)
{
	text->put(i, 0, linelist[position + i]->text);
}

void AnsiWindow::lineCount()
{
	char tmp[25];
	int percent = (position + y) * 100 / NumOfLines;
	if (percent > 100)
		percent = 100;

	sprintf(tmp, "Lines: %6d/%-6d %3d%%", position + 1, NumOfLines,
		percent);
	header->put(0, COLS-26, tmp);
	header->delay_update();
}

void AnsiWindow::DrawBody()
{
	lineCount();

	for (int i = 0; i < y; i++)
		if ((position + i) < NumOfLines)
			oneLine(i);
		else
			for (int j = 0; j < x; j++)
				text->put(i, j, (chtype) ' ' | C_ANSIBACK);
}

void AnsiWindow::set(const char *ansiSource, const char *winTitle,
	bool latin)
{
	fromFile = 0;
	source = (const unsigned char *) ansiSource;

	position = 0;
	sourceDel = false;

	isLatin = latin;

	title = winTitle;
}

void AnsiWindow::setFile(file_header *f, const char *winTitle, bool latin)
{
	fromFile = f;
	source = 0;

	position = 0;
	sourceDel = true;

	isLatin = latin;
	
	title = winTitle;
}

void AnsiWindow::getFile()
{
	FILE *flist;
	char *list;

	if ((flist = mm.workList->ftryopen(fromFile->getName(), "rb"))) {
		off_t size = fromFile->getSize();

		list = new char[size + 1];
		if (fread(list, 1, size, flist))
			list[size] = '\0';
		else {
			delete list;
			list = 0;
		}
		fclose(flist);
	} else
		list = 0;
	
	source = (unsigned char *) list;
}

void AnsiWindow::MakeActive()
{
	int i;
	bool end, oldAbort;

	header = new Win(1, COLS, 0, C_LBOTTSTAT);
	text = new Win(LINES - 2, COLS, 1, C_ANSIBACK);
	statbar = new Win(1, COLS, LINES - 1, C_LBOTTSTAT);

	text->attrib(0);

	anim = false;
	oldAbort = ansiAbort;

	char *tmp = new char[COLS + 1];
	i = sprintf(tmp, " " MM_TOPHEADER, MM_NAME, MM_MAJOR, MM_MINOR);
	for (; i < COLS; i++)
		tmp[i] = ' ';
	tmp[i] = '\0';

	header->put(0, 0, tmp);
	header->delay_update();
	delete[] tmp;

	y = LINES - 2;
	x = COLS;

	for (i = 0; i < 64; i++)
		colorsused[i] = false;

	colorsused[PAIR_NUMBER(C_LBOTTSTAT)] = 1;  //don't remap stat bars

	oldcolorx = oldcolory = 0;

	init_pair(((COLOR_WHITE) << 3) + (COLOR_WHITE),
		COLOR_WHITE, COLOR_WHITE);

	if (fromFile)
		getFile();

	MakeChain(source);

	// This deals with the unavailability of color pair 0:

	if (colorsused[0]) {	// assumes COLOR_BLACK == 0

		// Find an unused color pair for black-on-black:

		for (end = false, i = 1; i < 64 && !end; i++)
			if (!colorsused[i]) {
				end = true;
				oldcolorx = i >> 3;
				oldcolory = i & 7;
				init_pair(i, COLOR_BLACK, COLOR_BLACK);
			}

		// Remap all instances of color pair 0 to the new pair:

		if (end) {
			for (int j = 0; j < NumOfLines; j++) {
				curr = linelist[j];
				for (i = 0; i < (int) curr->length; i++)
					if (!PAIR_NUMBER(curr->text[i])) {
						curr->text[i] &= ~A_COLOR;
						curr->text[i] |=
						  COL(oldcolorx, oldcolory);
					}
			}
		}
	}

	DrawBody();
	text->delay_update();

	statupdate();

	ansiAbort = oldAbort;
}

void AnsiWindow::Delete()
{
	ansiAbort = true;

	// Restore remapped color pairs:

	if (oldcolorx + oldcolory)
		init_pair((oldcolorx << 3) + oldcolory,
			oldcolorx, oldcolory);
	init_pair(((COLOR_WHITE) << 3) + (COLOR_WHITE),
		COLOR_BLACK, COLOR_BLACK);

	DestroyChain();
	delete header;
	delete text;
	delete statbar;
	if (sourceDel)
		delete[] source;
}

void AnsiWindow::setPos(int x)
{
	position = x;
}

int AnsiWindow::getPos()
{
	return position;
}

searchret AnsiWindow::search(const char *item)
{
	chtype *ch;
	searchret found = False;

	unsigned char *a, *buffer = new unsigned char[COLS + 1];

	for (int x = position + 1; (x < NumOfLines) && (found == False);
	    x++) {

		if (text->keypressed() == 27) {
			found = Abort;
			break;
		}

		a = buffer;
		ch = linelist[x]->text;
		int count = linelist[x]->length;

		for (int y = 0; y < count; y++)
			*a++ = (*ch++ & A_CHARTEXT);
		*a = '\0';

		found = searchstr((char *) buffer, item) ? True : False;

		if (found == True) {
			position = x;
			DrawBody();
			text->delay_update();
		}
	}

	delete[] buffer;
	return found;
}

void AnsiWindow::Save()
{
	FILE *fd;
	const unsigned char *message = source;

	static char keepname[128];
	char filename[128], oldfname[128];

	if (keepname[0])
		strcpy(filename, keepname);
	else {
		sprintf(filename, "%.8s.ans", title);
		unspace(filename);
	}

	strcpy(oldfname, filename);

	if (interface->savePrompt("Save to file:", filename)) {
		mychdir(mm.resourceObject->get(SaveDir));
		if ((fd = fopen(filename, "at"))) {
			while (*message)
				fputc(*message++, fd);
			fclose(fd);
		}
		if (strcmp(filename, oldfname))
			strcpy(keepname, filename);
	}
}

void AnsiWindow::KeyHandle(int key)
{
	switch (key) {
	case KEY_UP:
		if (position > 0) {
			position--;
			text->wscroll(-1);
			oneLine(0);
			lineCount();
		}
		break;
	case KEY_DOWN:
		if (position < NumOfLines - y) {
			position++;
			text->wscroll(1);
			oneLine(y - 1);
			lineCount();
		}
		break;
	case KEY_HOME:
		position = 0;
		DrawBody();
		break;
	case KEY_END:
		if (NumOfLines > y) {
			position = NumOfLines - y;
			DrawBody();
		}
		break;
	case 'B':
	case KEY_PPAGE:
		position -= ((y < position) ? y : position);
		DrawBody();
		break;
	case ' ':
		position += y;
		if (position < NumOfLines)
			DrawBody();
		else
			interface->setKey('\n');
		break;
	case 'F':
	case KEY_NPAGE:
		if (position < NumOfLines - y) {
			position += y;
			if (position > NumOfLines - y)
				position = NumOfLines - y;
			DrawBody();
		}
		break;
	case 'V':
	case 1:
	case 22:
		animate();
		break;
	case 'S':
		Save();
		DrawBody();
		break;
	case KEY_F(1):
	case '?':
		interface->changestate(ansi_help);
	}
	text->delay_update();
}
