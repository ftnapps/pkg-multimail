/*
 * MultiMail offline mail reader
 * message display

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "interfac.h"

Line::Line(int size)
{
	next = 0;
	text = (size ? new char[size] : 0);
	quoted = hidden = tearline = tagline = origin = sigline = false;
}

Line::~Line()
{
	delete[] text;
}

LetterWindow::LetterWindow()
{
	linelist = 0;
	NM.isSet = hidden = rot13 = false;
	NumOfLines = 0;
	To[0] = tagline1[0] = '\0';
	stripCR = mm.resourceObject->getInt(StripSoftCR);
	beepPers = mm.resourceObject->getInt(BeepOnPers);
	lynxNav = mm.resourceObject->getInt(UseLynxNav);
}

net_address &LetterWindow::PickNetAddr()
{
	Line *line;
	static net_address result;
	int i;

	for (i = 0; i < NumOfLines; i++)
		if (!strncmp(" * Origin:", linelist[i]->text, 10))
			break;

	if (i != NumOfLines) {
		line = linelist[i];
		i = line->length;
		while (((line->text[i - 1]) != '(') && i > 0)
			i--;
		//we have the opening bracket

		while ((i < (int) line->length) &&
			((line->text[i] < '0') || (line->text[i] > '9')))
				i++;
		//we have the begining of the address

		result = &line->text[i];
	} else
		result = mm.letterList->getNetAddr();

	return result;
}

void LetterWindow::ReDraw()
{
	headbar->wtouch();
	header->wtouch();
	text->wtouch();
	statbar->cursor_on();	// to get it out of the way
	statbar->wtouch();
	statbar->cursor_off();
}

void LetterWindow::DestroyChain()
{
	if (linelist) {
		while (NumOfLines)
			delete linelist[--NumOfLines];
		delete[] linelist;
		linelist = 0;
	}
	letter_in_chain = -1;
}

/* Take a message as returned by getBody() -- the whole thing as one C
   string, with paragraphs separated by '\n' -- and turn it into a linked
   list with an array index, wrapping long paragraphs if needed. Also
   flags hidden and quoted lines.
*/
void LetterWindow::MakeChain(int columns, bool rejoin)
{
	static const char *seenby = "SEEN-BY:";
	const int tabwidth = 8;

	Line head(0), *curr;
	unsigned char each, compare = isoConsole ? 0xEC : 0x8D;
	char *j, *k, *begin;
	int c;
	bool end = false, wrapped = false, insig = false;
	bool strip = mm.letterList->isLatin() ? false : stripCR;
	bool skipSeenBy = false;

	int orgarea = -1;
	if (mm.areaList->isCollection() && !mm.areaList->isReplyArea()) {
		orgarea = mm.areaList->getAreaNo();
		mm.areaList->gotoArea(mm.letterList->getAreaID());
	}
	bool inet = (mm.areaList->isInternet() || mm.areaList->isUsenet());

	DestroyChain();
	j = (char *) mm.letterList->getBody();

	letterconv_in(j);

	letter_in_chain = mm.letterList->getCurrent();
	curr = &head;

	while (!end) {
	    if (!hidden)	// skip ^A lines
		while ((*j == 1) || (skipSeenBy && !strncmp(j, seenby, 8))) {
			do
				j++;
			while (*j && (*j != '\n'));
			while (*j == '\n')
				j++;
		}

	    if (*j) {
		curr->next = new Line(((columns > 80) || !rejoin) ?
			columns : 81);
		curr = curr->next;

		NumOfLines++;

		begin = 0;
		k = curr->text;
		c = 0;

		if (*j == 1) {		// ^A is hidden line marker
			j++;
			curr->hidden = true;
		} else
			if (skipSeenBy && !strncmp(j, seenby, 8))
				curr->hidden = true;

		while (!end && ((c < columns) || (rejoin &&
		    curr->quoted && (c < 80)))) {

			each = *j;

			// Strip soft CR's:

			if (strip && (each == compare))
				each = *++j;

			if (each == '\n') {
			    if (wrapped) {
				wrapped = false;

				// Strip any trailing spaces (1):

				while (c && (k[-1] == ' ')) {
					c--;
					k--;
				}

				// If the next character indicates a new
				// paragraph, quote or hidden line, this
				// is the end of the line; otherwise make
				// it a space:

				each = j[1];
				if (!each || (each == '\n') || (each == ' ')
				  || (each == '\t') || (each == '>') ||
				     (each == 1) || skipSeenBy) {
					j++;
					break;
				} else
					if (!c)
						j++;
					else
						each = ' ';
			    } else {		// if wrapped is false, EOL
				j++;
				break;
			    }
			}

			switch (each) {
			case '\t':
				{
				    int i = (tabwidth - 1) - (c % tabwidth);
				    c += i;
				    while (i--)
					*k++ = ' ';
				    each = ' ';
				}
			case ' ':
				// begin == start of last word (for wrapping):

				begin = 0;
				if (rejoin && !curr->quoted)
					if (c == (columns - 1))
						wrapped = true;
				break;
			case '>':		// Quoted line
				if (c < 5)
					curr->quoted = true;
				break;
			case ':':
				if ((j[1] == '-') || (j[1] == ')') ||
				    (j[1] == '('))
					break;
			case '|':
				if (c == 0)
					curr->quoted = true;
				break;
			default:
				if (rot13) {
				    if (each >= 'A' && each <= 'Z')
					each = (each - 'A' + 13) % 26 + 'A';
				    else if (each >= 'a' && each <= 'z')
					each = (each - 'a' + 13) % 26 + 'a';
				}
			}
			if ((each != ' ') && !begin)
				begin = j;

			if (each) {
				*k++ = each;
				j++;
				c++;
			}

			end = !each;	// a 0-terminated string
		}

		k = curr->text;

		// Start a new line on a word boundary (if needed):

		if ((c == columns) && begin && ((j - begin) < columns) &&
		   !(rejoin && curr->quoted)) {
			c -= j - begin;
			j = begin;
			wrapped = rejoin;
		}

		// Check for sigs:

		if (inet && !insig && (k[0] == '-') && (k[1] == '-') &&
		    (((k[2] == ' ') && (c == 3)) || (c == 2)))
			insig = true;
		curr->sigline = insig;

		// Strip any trailing spaces (2):

		while (c && (k[c - 1] == ' '))
			c--;

		k[c] = '\0';
		curr->length = c;

		// Check for taglines, tearlines, and origin lines:

		if (!inet) {
			each = *k;
			if ((k[1] == each) && (k[2] == each) &&
			   ((k[3] == ' ') || (c == 3)))
				switch (each) {
				case '.':
					curr->tagline = true;
					break;
				case '-':
				case '~':
					curr->tearline = true;
				}
			else
				if (!strncmp(k, " * Origin:", 10)) {
					curr->origin = true;

					// SEEN-BY: should only appear
					//after Origin:
					skipSeenBy = true;
				}
		}
	    } else
		end = true;
	}

	linelist = new Line *[NumOfLines];
	curr = head.next;
	c = 0;
	while (curr) {
		linelist[c++] = curr;
		curr = curr->next;
	}

	if (orgarea != -1)
		mm.areaList->gotoArea(orgarea);
}

void LetterWindow::StatToggle(int mask)
{
	int stat = mm.letterList->getStatus();
	stat ^= mask;
	mm.letterList->setStatus(stat);
	interface->setAnyRead();
	DrawHeader();
}

void LetterWindow::Draw(bool redo)
{
	if (redo) {
		rot13 = false;
		position = 0;
		tagline1[0] = '\0';
		if (!interface->dontRead()) {
			if (!mm.letterList->getRead()) {
				mm.letterList->setRead(); // nem ide kene? de.
				interface->setAnyRead();
			}
			if (beepPers && mm.letterList->isPersonal())
				beep();
		}
	}

	if (letter_in_chain != mm.letterList->getCurrent()) {
		MakeChain(COLS);
	        if (position >= NumOfLines)
        	        position = (NumOfLines > y) ? NumOfLines - y : 0;
	}
	DrawHeader();
	DrawBody();
	DrawStat();
}

char *LetterWindow::netAdd(char *tmp)
{
	net_address &na = mm.letterList->getNetAddr();
	if (na.isSet) {
		const char *p = na;
		if (*p)
			tmp += sprintf(tmp, (na.isInternet ?
				" <%s>" : " @ %s"), p);
	}
	return tmp;
}

#define flagattr(x) header->attrib((x) ? C_LHFLAGSHI : C_LHFLAGS)

void LetterWindow::DrawHeader()
{
	char tmp[256], format[12], sformat[12], *p;
	int stat = mm.letterList->getStatus();

	int orgarea = -1;
	if (mm.areaList->isCollection() && !mm.areaList->isReplyArea()) {
		orgarea = mm.areaList->getAreaNo();
		mm.areaList->gotoArea(mm.letterList->getAreaID());
	}
	bool hasTo = mm.areaList->hasTo();

	int maxToFromWidth = COLS - 42;
	if (maxToFromWidth > 255)
		maxToFromWidth = 255;
	sprintf(format, "%%.%ds", maxToFromWidth);

	int maxSubjWidth = COLS - 9;
	if (maxSubjWidth > 255)
		maxSubjWidth = 255;
	sprintf(sformat, "%%.%ds", maxSubjWidth);

	header->Clear(C_LHEADTEXT);

	header->put(0, 2, "Msg#:");
	header->put(1, 2, "From:");
	if (hasTo)
		header->put(2, 2, "  To:");
	else
		header->put(2, 2, "Addr:");
	header->put(3, 2, "Subj:");

	if (orgarea != -1)
		mm.areaList->gotoArea(orgarea);

	header->attrib(C_LHMSGNUM);
	sprintf(tmp, "%d (%d of %d)   ", mm.letterList->getMsgNum(),
		mm.letterList->getCurrent() + 1, mm.areaList->getNoOfLetters());
	header->put(0, 8, tmp);

	if (orgarea != -1)
		mm.areaList->gotoArea(mm.letterList->getAreaID());

	header->attrib(C_LHFROM);
	p = tmp + sprintf(tmp, format, mm.letterList->getFrom());
	if (mm.areaList->isEmail())
		netAdd(p);
	letterconv_in(tmp);
	header->put(1, 8, tmp);

	header->attrib(C_LHTO);
	if (hasTo) {
		p = (char *) mm.letterList->getTo();
		if (*p) {
			p = tmp + sprintf(tmp, format, p);
			if (mm.areaList->isReplyArea())
				if (p != tmp)
					netAdd(p);
		}
	} else
		sprintf(tmp, format,
			(const char *) mm.letterList->getNetAddr());
		
	letterconv_in(tmp);
	header->put(2, 8, tmp);

	header->attrib(C_LHSUBJ);
	int i = sprintf(tmp, sformat, mm.letterList->getSubject());
	letterconv_in(tmp);
	header->put(3, 8, tmp);

	for (i += 8; i < COLS; i++)
		header->put(3, i, ' ');

	header->attrib(C_LHEADTEXT);
	header->put(0, COLS - 33, "Date:");
	header->put(1, COLS - 33, "Line:");
	header->put(2, COLS - 33, "Stat: ");

	//header->put(4, 0, ACS_LLCORNER);
	//header->horizline(4, COLS-2);
	//header->put(4, COLS-1, ACS_LRCORNER);

	flagattr(mm.letterList->getPrivate());
	header->put(2, COLS - 27, "Pvt ");
	flagattr(stat & MS_READ);
	header->put(2, COLS - 23, "Read ");
	flagattr(stat & MS_REPLIED);
	header->put(2, COLS - 18, "Replied ");
	flagattr(stat & MS_MARKED);
	header->put(2, COLS - 10, "Marked  ");

	header->attrib(C_LHDATE);
	sprintf(tmp, "%.30s", mm.letterList->getDate());

	// Truncate the date to fit, if needed:
	p = tmp;
	while (p && (strlen(tmp) > 26)) {
		p = strrchr(tmp, ' ');
		if (p)
			*p = '\0';
	}

	letterconv_in(tmp);
	header->put(0, COLS - 27, tmp);

	if (orgarea != -1)
		mm.areaList->gotoArea(orgarea);

	lineCount();
}

void LetterWindow::lineCount()
{
	char tmp[80];
	int percent = ((position + y) > NumOfLines) ? 100 :
		((position + y) * 100 / NumOfLines);

	header->attrib(C_LHMSGNUM);
	sprintf(tmp, "%9d/%-10d%3d%%    ", position + 1, NumOfLines,
		percent);
	header->put(1, COLS-28, tmp);
	header->delay_update();
}

void LetterWindow::oneLine(int i)
{
	int j, length, z = position + i;

	if (z < NumOfLines) {
		Line *curr = linelist[z];

		text->attrib(curr->hidden ? C_LHIDDEN :
			(curr->origin ? C_LORIGIN :
			(curr->tearline ? C_LTEAR :
			((curr->tagline || curr->sigline) ? C_LTAGLINE :
			(curr->quoted ? C_LQTEXT : C_LTEXT)))));

		if (curr->tagline && (curr->length > 3)) {
			strncpy(tagline1, &curr->text[4], TAGLINE_LENGTH);
			tagline1[TAGLINE_LENGTH] = '\0';
		}

		length = text->put(i, 0, curr->text);
	} else
		length = 0;

	for (j = length; j < COLS; j++)
		text->put(i, j, ' ');
}

void LetterWindow::DrawBody()
{
	lineCount();

	for (int i = 0; i < y; i++)
		oneLine(i);
	text->delay_update();
}

void LetterWindow::DrawStat()
{
	static const char *helpmsg = " F1 or ? - Help ";
	char format[40], *tmp = new char[COLS + 1];

	int maxw = COLS - 17;

	bool collflag = false;
	if (mm.areaList->isCollection()) {
		if (mm.areaList->isReplyArea()) {
			maxw -= 10;
			sprintf(format, " REPLY in: %%-%d.%ds%%s",
				maxw, maxw);
		} else {
			maxw -= 13;
			sprintf(format, " PERSONAL in: %%-%d.%ds%%s",
				maxw, maxw);
		}
		mm.areaList->gotoArea(mm.letterList->getAreaID());
		collflag = true;
	} else
		sprintf(format, " %%-%d.%ds%%s", maxw, maxw);

	const char *s = mm.letterList->getNewsgrps();
	sprintf(tmp, format, s ? s : mm.areaList->getDescription(), helpmsg);
	areaconv_in(tmp);

	if (collflag)
		interface->areas.Select();

	statbar->cursor_on();
	statbar->put(0, 0, tmp);
	statbar->delay_update();
	statbar->cursor_off();

	delete[] tmp;
}

void LetterWindow::MakeActive(bool redo)
{
	DestroyChain();

	y = LINES - 7;

	headbar = new Win(1, COLS, 0, C_LBOTTSTAT);
	header = new Win(5, COLS, 1, C_LHEADTEXT);
	text = new Win(y, COLS, 6, C_LTEXT);
	statbar = new Win(1, COLS, LINES - 1, C_LBOTTSTAT);

	char *tmp = new char[COLS + 1];
	int i = sprintf(tmp, " " MM_TOPHEADER, MM_NAME, MM_MAJOR,
		MM_MINOR);
	for (; i < COLS; i++)
		tmp[i] = ' ';
	tmp[i] = '\0';

	headbar->put(0, 0, tmp);
	headbar->delay_update();

	delete[] tmp;

 	Draw(redo);
}

bool LetterWindow::Next()
{
	if (mm.letterList->getActive() < (mm.letterList->noOfActive() - 1)) {
		interface->letters.Move(DOWN);
		mm.letterList->gotoActive(mm.letterList->getActive() + 1);
		Draw(true);
		return true;
	} else {
		interface->back();
		doupdate();
		interface->back();
		doupdate();
		interface->areas.KeyHandle('+');
	}
	return false;
}

bool LetterWindow::Previous()
{
	if (mm.letterList->getActive() > 0) {
		interface->letters.Move(UP);
		mm.letterList->gotoActive(mm.letterList->getActive() - 1);
		Draw(true);
		return true;
	} else {
		interface->back();
		doupdate();
		interface->back();
		doupdate();
		interface->areas.Move(UP);
	}
	return false;
}

void LetterWindow::Move(int dir)
{
	switch (dir) {
	case KEY_UP:
		if (position > 0) {
			position--;
			text->wscroll(-1);
			oneLine(0);
			text->delay_update();
			lineCount();
		}
		break;
	case KEY_DOWN:
		if (position < NumOfLines - y) {
			position++;
			lineCount();
			text->wscroll(1);
			oneLine(y - 1);
			text->delay_update();
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
	case 'F':
	case KEY_NPAGE:
		if (position < NumOfLines - y) {
			position += y;
			if (position > NumOfLines - y)
				position = NumOfLines - y;
			DrawBody();
		}
		break;
	}
}

void LetterWindow::NextDown()
{
	position += y;
	if (position < NumOfLines)
		DrawBody();
	else
		Next();
}

void LetterWindow::Delete()
{
	DestroyChain();
	delete headbar;
	delete header;
	delete text;
	delete statbar;
}

bool LetterWindow::Save(int stype)
{
	FILE *fd;

	static const char *ntemplate[] = {
		"%.8s.%.03d", "%.8s.all", "%.8s.mkd"
	};

	static char keepname[3][128];
	char filename[128], oldfname[128];

	stype--;

	if (keepname[stype][0])
		strcpy(filename, keepname[stype]);
	else {
		switch (stype) {
		case 2:					// Marked
		case 1:					// All
			sprintf(filename, ntemplate[stype],
				mm.areaList->getName());
			break;
		case 0:					// This one
			sprintf(filename, ntemplate[0],
				mm.areaList->getName(),
					mm.letterList->getCurrent());
		}

		unspace(filename);
	}

	strcpy(oldfname, filename);

	if (interface->savePrompt("Save to file:", filename)) {
		mychdir(mm.resourceObject->get(SaveDir));
		if ((fd = fopen(filename, "at"))) {
			int num = mm.letterList->noOfActive();

			switch (stype) {
			case 2:
			case 1:
				for (int i = 0; i < num; i++) {
					mm.letterList->gotoActive(i);
					if ((stype == 1) ||
					  (mm.letterList->getStatus() &
					    MS_MARKED))
						write_to_file(fd);
				}
				break;
			case 0:
				write_to_file(fd);
			}
			fclose(fd);
			if (!stype)
				MakeChain(COLS);

			interface->setAnyRead();
		}
		if (strcmp(filename, oldfname))
			strcpy(keepname[stype], filename);

		return true;
	} else
		return false;

}

void LetterWindow::write_header_to_file(FILE *fd)
{
	enum {system, area, newsg, date, from, to, addr, subj, items};
	static const char *names[items] = {"System", "  Area", "Newsgr",
		"  Date", "  From", "    To", "  Addr", "  Subj"};
	char Header[512], *p;
	int j;

	for (j = 0; j < 72; j++)
		fputc('=', fd);
	fputc('\n', fd);

	mm.areaList->gotoArea(mm.letterList->getAreaID());

	const char *head[items] = {
		mm.resourceObject->get(BBSName),
		mm.areaList->getDescription(),
		mm.letterList->getNewsgrps(), mm.letterList->getDate(),
		mm.letterList->getFrom(), mm.letterList->getTo(),
		mm.letterList->getNetAddr(), mm.letterList->getSubject()
	};

	interface->areas.Select();

	head[to + mm.areaList->hasTo()] = 0;
	if (head[newsg])
		head[area] = 0;

	for (j = 0; j < items; j++)
		if (head[j]) {
			p = Header + sprintf(Header, "%.511s", head[j]);
			if (((j == from) && mm.areaList->isEmail())
			    || ((j == to) && mm.areaList->isReplyArea()))
				p = netAdd(p);
			letterconv_in(Header);
			fprintf(fd, " %s: %s\n", names[j], Header);
		}

	for (j = 0; j < 72; j++)
		fputc('-', fd);
	fputc('\n', fd);
}

void LetterWindow::write_to_file(FILE *fd)
{
	write_header_to_file(fd);

	// write chain to file

	MakeChain(80);
	for (int j = 0; j < NumOfLines; j++)
		fprintf(fd, "%s\n", linelist[j]->text);
	fputc('\n', fd);

	// set read, unmarked -- not part of writing to a file, but anyway

	int stat = mm.letterList->getStatus();
	int oldstat = stat;
	stat |= MS_READ;
	stat &= ~MS_MARKED;
	mm.letterList->setStatus(stat);
	if (stat != oldstat)
		interface->setAnyRead();
}

// For searches (may want to start at position == -1):
void LetterWindow::setPos(int x)
{
	position = x;
}

int LetterWindow::getPos()
{
	return position;
}

searchret LetterWindow::search(const char *item)
{
	searchret found = False;

	for (int x = position + 1; (x < NumOfLines) && (found == False);
	    x++) {

		if (text->keypressed() == 27) {
			found = Abort;
			break;
		}

		found = searchstr(linelist[x]->text, item) ? True : False;

		if (found == True) {
			position = x;
			if (interface->active() == letter)
				DrawBody();
		}
	}

	return found;
}

void LetterWindow::KeyHandle(int key)
{
	int t_area;

	switch (key) {
	case 'D':
		rot13 = !rot13;
		interface->redraw();
		break;
	case 'X':
		hidden = !hidden;
		interface->redraw();
		break;
	case 'I':
		stripCR = !stripCR;
		interface->redraw();
		break;
	case 'S':
		Save(1);
		DrawHeader();
		ReDraw();
		break;
	case '?':
	case KEY_F(1):
		interface->changestate(letter_help);
		break;
	case 'V':
	case 1: // CTRL-A
	case 11: // CTRL-V
		{
			int nextAns;
			bool cont = false;
			do {
			    nextAns = interface->ansiLoop(mm.letterList->
				getBody(), mm.letterList->getSubject(),
				mm.letterList->isLatin());
			    if (nextAns == 1)
				cont = Next();
			    else if (nextAns == -1)
				cont = Previous();
			} while (nextAns && cont);
		}
		break;
	case KEY_RIGHT:
		if (lynxNav)
			break;
	case MM_PLUS:
		Next();
		break;
	case KEY_LEFT:
		if (lynxNav) {
			interface->back();
			break;
		}
	case MM_MINUS:
		Previous();
		break;
	case ' ':
		NextDown();
		break;
	case 6:				// Ctrl-F
		EditLetter(true);
		interface->redraw();
		break;
	default:
	    if (mm.areaList->isReplyArea()) {
		switch(key) {
		case 'R':
		case 'E':
			EditLetter(false);
			interface->redraw();
			break;
		case KEY_DC:
		case 'K':
			interface->kill_letter();
			break;
		case 2:				// Ctrl-B
			SplitLetter();
			interface->redraw();
			break;
		default:
			Move(key);
		}
	    } else {
		switch (key) {
		case '\t':
			interface->letters.Next();
			Draw(true);
			break;
		case 'M':
		case 'U':
			StatToggle((key == 'M') ? MS_MARKED : MS_READ);
			break;
		case 'R':	// Allow re-editing from here:
		case 'O':
			if (mm.letterList->getStatus() & MS_REPLIED)
				if (EditOriginal()) {
					interface->redraw();
					break;
				}
		case 'E':
			t_area = interface->areaMenu();
			if (t_area != -1) {
				mm.areaList->gotoArea(t_area);
				set_Letter_Params(t_area, key);

				if (mm.areaList->isEmail())
					if (key == 'E')
						interface->addressbook();
					else {
						net_address nm = PickNetAddr();
						set_Letter_Params(nm, 0);
					}
				EnterLetter();
			}
			break;
		case 'N':
			t_area = (mm.areaList->isUsenet() ||
				mm.areaList->isInternet()) ?
				mm.areaList->findInternet() :
				mm.areaList->findNetmail();
			if (t_area != -1) {
			    net_address nm = PickNetAddr();
			    if (nm.isSet) {
				set_Letter_Params(t_area, 'N');
				set_Letter_Params(nm, 0);
				EnterLetter();
			    } else
				interface->nonFatalError(
					"No reply address");
			} else
				interface->nonFatalError(
					"Netmail is not available");
			break;
		case 'T':
			GetTagline();
			break;
		default:
			Move(key);
		}
	    }
	}
}
