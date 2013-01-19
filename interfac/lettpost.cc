/*
 * MultiMail offline mail reader
 * message editing (continuation of the LetterWindow class)

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "interfac.h"

void LetterWindow::set_Letter_Params(net_address &nm, const char *to)
{
	NM = nm;
	strncpy(To, to ? to : "", 29);
}

void LetterWindow::set_Letter_Params(int area, char param_key)
{
	key = param_key;
	replyto_area = area;
}

void LetterWindow::QuoteText(FILE *reply)
{
	char TMP[81];
	int i;
	bool inet = mm.areaList->isInternet() || mm.areaList->isUsenet();
	const char *TMP2 = mm.letterList->getFrom();

	int width = mm.resourceObject->getInt(QuoteWrapCols);
	if ((width < 20) || (width > 80))
		width = 78;
	width -= inet ? 2 : 6;

	char c;
	const char *s, *quotestr = mm.resourceObject->get(inet ? InetQuote
		: QuoteHead);

	while ((c = *quotestr++)) {
		if (c != '%')
			fputc(c, reply);
		else {
			s = 0;
			switch (tolower(*quotestr)) {
			case 'f':
				s = TMP2;
				break;
			case 't':
				s = mm.letterList->getTo();
				break;
			case 'd':
				s = mm.letterList->getDate();
				break;
			case 's':
				s = mm.letterList->getSubject();
				break;
			case 'a':
				s = mm.areaList->getDescription();
				break;
			case 'n':
				s = "\n";
				break;
			case '%':
				s = "%";
			}
			if (s) {
				sprintf(TMP, "%.80s", s);
				letterconv_in(TMP);
				fputs(TMP, reply);
				quotestr++;
			}
		}
	}
	fputs("\n\n", reply);

	if (!inet) {
		char mg[4];

		strncpy(mg, TMP2, 2);
		mg[2] = '\0';
		mg[3] = '\0';

		i = 1;
		for (int j = 1; j < 3; j++) {
			bool end = false;
			while (TMP2[i] && !end) {
				if ((TMP2[i - 1] == ' ') && (TMP2[i] != ' ')) {
					mg[j] = TMP2[i];
					if (j == 1)
						end = true;
				}
				i++;
			}
		}
		letterconv_in(mg);

		sprintf(TMP, " %s> %%s\n", mg);
	} else
		strcpy(TMP, "> %s\n");

	MakeChain(width, true);

	for (i = 0; i < NumOfLines; i++) {
		Line *curr = linelist[i];

		if (!(curr->sigline || (!curr->length &&
		    (i < (NumOfLines - 1)) && linelist[i + 1]->sigline)))
			fprintf(reply, curr->length ? (curr->quoted ? (inet ?
				">%s\n" : ((curr->text[0] == ' ') ? "%s\n" :
				" %s\n")) : TMP) : (inet ? ">%s\n" : "%s\n"),
				curr->text);
	}

	MakeChain(COLS);
}

int LetterWindow::HeaderLine(ShadowedWin &win, char *buf, int limit,
				int pos, int color)
{
	areaconv_in(buf);
	int getit = win.getstring(pos, 8, buf, limit, color, color);
	areaconv_out(buf);
	return getit;
}

int LetterWindow::EnterHeader(char *FROM, char *TO, char *SUBJ, bool &privat)
{
	static const char *noyes[] = { "No", "Yes" };

	char NETADD[100];
	int result, current, maxitems = 2;
	bool end = false;
	bool hasNet = mm.areaList->isEmail();
	bool hasNews = mm.areaList->isUsenet();
	bool hasTo = hasNews || mm.areaList->hasTo();

	if (hasNet) {
		strcpy(NETADD, NM);
		maxitems++;
	} else
		NM.isSet = false;

	if (hasTo)
		maxitems++;

	ShadowedWin rep_header(maxitems + 2, COLS - 2, (LINES / 2) - 3,
		C_LETEXT);

	rep_header.put(1, 2, "From:");
	if (hasTo)
		rep_header.put(2, 2, "  To:");
	if (hasNet)
		rep_header.put(3, 2, "Addr:");
	rep_header.put(maxitems, 2, "Subj:");

	rep_header.attrib(C_LEGET2);
	areaconv_in(FROM);
	rep_header.put(1, 8, FROM, 69);
	areaconv_out(FROM);

	if (hasNet && !NM.isSet) {
		//NM.isInternet = mm.areaList->isInternet();
		TO[0] = '\0';
		current = 1;
	} else {
		if (hasTo) {
			rep_header.attrib(C_LEGET1);
			areaconv_in(TO);
			rep_header.put(2, 8, TO, 69);
			areaconv_out(TO);
		}
		if (hasNet) {
			rep_header.attrib(C_LEGET2);
			areaconv_in(NETADD);
			rep_header.put(3, 8, NETADD, 69);
			areaconv_out(NETADD);
		}
		current = maxitems - 1;
	}

	rep_header.update();

	do {
		result = HeaderLine(rep_header, (current == (maxitems - 1)) ?
			SUBJ : (((current == 2) && hasNet) ?
			NETADD : ((current && hasTo) ? TO : FROM)),
			((current == 1) && hasNews) ? 512 : ((current ==
			(maxitems - 1)) ? mm.areaList->maxSubLen() :
			(((current == 2) && NM.isInternet) ? 72 :
			mm.areaList->maxToLen())), current + 1, (current &
			1) ? C_LEGET1 : C_LEGET2);

		switch (result) {
		case 0:
			end = true;
			break;
		case 1:
			current++;
			if (current == maxitems)
				end = true;
			break;
		case 2:
			if (current > 0)
				current--;
			break;
		case 3:
			if (current < maxitems - 1)
				current++;
		}
	} while (!end);

	if (result) {
		int pmode = (!hasNet ? mm.areaList->hasPublic() : 0) +
			((mm.areaList->hasPrivate() &&
			!mm.areaList->isUsenet()) ? 2 : 0);

		if (hasNet)
			NM = NETADD;

		switch (pmode) {
		case 1:
			privat = false;
			break;
		case 2:
			privat = true;
			break;
		case 3:
			if (!interface->WarningWindow(
				"Make letter private?", privat ? 0 : noyes))
						privat = !privat;
		}
	}
	return result;
}

long LetterWindow::reconvert(const char *reply_filename)
{
	FILE *reply;

	reply = fopen(reply_filename, "rt");
	fseek(reply, 0, SEEK_END);
	long replen = ftell(reply);
	rewind(reply);

	char *body = new char[replen + 1];
	replen = (long) fread(body, 1, replen, reply);
	fclose(reply);

	body[replen] = '\0';
	areaconv_out(body);
	while (body[replen - 1] == '\n')
		replen--;

	reply = fopen(reply_filename, "wt");
	fwrite(body, 1, replen, reply);
	fclose(reply);

	delete[] body;
	return replen;
}

void LetterWindow::setToFrom(char *TO, char *FROM)
{
	char format[7];
	sprintf(format, "%%.%ds", mm.areaList->maxToLen());

	bool usealias = mm.areaList->getUseAlias();
	if (usealias) {
		sprintf(FROM, format, mm.resourceObject->get(AliasName));
		if (!FROM[0])
			usealias = false;
	}
	if (!usealias)
		sprintf(FROM, format, mm.resourceObject->get(LoginName));

	if (mm.areaList->isUsenet()) {
		const char *newsgrps = 0;

		if (key != 'E') {
			newsgrps = mm.letterList->getFollow();
			if (!newsgrps)
				newsgrps = mm.letterList->getNewsgrps();
		}
		if (!newsgrps)
			newsgrps = mm.areaList->getDescription();

		sprintf(TO, "%.512s", newsgrps);
	} else
		if (key == 'E')
			strcpy(TO, (To[0] ? To : "All"));
		else
			if (mm.areaList->isInternet()) {
				if (key == 'O') {
				    sprintf(TO, format,
					fromName(mm.letterList->getTo()));
				    NM = fromAddr(mm.letterList->getTo());
				} else {
				    const char *rep =
					mm.letterList->getReply();

				    sprintf(TO, format,
					mm.letterList->getFrom());
				    if (rep)
					NM = fromAddr(rep);
				}
			} else
				sprintf(TO, format, (key == 'O') ?
				    mm.letterList->getTo() :
					mm.letterList->getFrom());
}

void LetterWindow::EnterLetter()
{
	FILE *reply;
	char reply_filename[256], FROM[74], TO[514], SUBJ[514];
	const char *orig_id;

	mystat fileStat;
	time_t oldtime;

	long replen;
	int replyto_num;
	bool privat;

	mm.areaList->gotoArea(replyto_area);

	bool news = mm.areaList->isUsenet();
	bool inet = news || mm.areaList->isInternet();

	// HEADER

	setToFrom(TO, FROM);

	if (key == 'E')
		SUBJ[0] = '\0';	//we don't have subject yet
	else {
		const char *s = stripre(mm.letterList->getSubject());
		int len = strlen(s);

		sprintf(SUBJ, ((len + 4) > mm.areaList->maxSubLen()) ?
			"%.512s" : "Re: %.509s", s);
	}

	privat = (key == 'E') ? false : mm.letterList->getPrivate();

	if (!EnterHeader(FROM, TO, SUBJ, privat)) {
		NM.isSet = false;
		interface->areas.Select();
		interface->redraw();
		return;
	}

	// Don't use refnum if posting in different area:

	replyto_num = ((key == 'E') || (key == 'N') || (replyto_area !=
		mm.letterList->getAreaID())) ? 0 : mm.letterList->getMsgNum();

	orig_id = replyto_num ? mm.letterList->getMsgID() : 0;

	// BODY

	mytmpnam(reply_filename);

	// Quote the old text 

	if (key != 'E') {
		reply = fopen(reply_filename, "wt");
		QuoteText(reply);
		fclose(reply);
	}
	fileStat.init(reply_filename);
	oldtime = fileStat.date;

	// Edit the reply

	edit(reply_filename);
	interface->areas.Select();
	interface->redraw();

	// Check if modified

	fileStat.init(reply_filename);
	if (fileStat.date == oldtime)
		if (interface->WarningWindow("Cancel this letter?")) {
			remove(reply_filename);
			interface->redraw();
			return;
		}

	// Mark original as replied

	if (key != 'E') {
		int origatt = mm.letterList->getStatus();
		mm.letterList->setStatus(origatt | MS_REPLIED);
		if (!(origatt & MS_REPLIED))
			interface->setAnyRead();
	}

	reply = fopen(reply_filename, "at");

	// Signature

	bool sigset = false;

	const char *sg = mm.resourceObject->get(sigFile);
	if (sg && *sg) {
		FILE *s = fopen(sg, "rt");
		if (s) {
			char bufx[81];

			fputs(inet ? "\n-- \n" : "\n", reply);
			while (myfgets(bufx, sizeof bufx, s))
				fputs(bufx, reply);
			fclose(s);

			sigset = true;
		}
	}

	// Tagline

	bool useTag = mm.resourceObject->get(UseTaglines) &&
		interface->Tagwin();
	if (useTag)
		fprintf(reply, inet ? (sigset ? "\n%s\n" : "\n-- \n%s\n")
			: "\n... %s\n", interface->taglines.getCurrent());
	else
		if (!inet)
			fprintf(reply, " \n");

	// Tearline (not for Blue Wave -- it does its own)

	if (!inet && mm.driverList->useTearline())
		fprintf(reply, "--- %s/%s v%d.%d\n", MM_NAME, sysname(),
			MM_MAJOR, MM_MINOR);

	fclose(reply);

	// Reconvert the text

	mm.areaList->gotoArea(replyto_area);
	replen = reconvert(reply_filename);

	mm.areaList->enterLetter(replyto_area, FROM, news ? "All" : TO,
		SUBJ, orig_id, news ? TO : 0, replyto_num, privat, NM,
		reply_filename, (int) replen);

	NM.isSet = false;
	To[0] = '\0';

	interface->areas.Select();
	interface->setUnsaved();
}

void LetterWindow::forward_header(FILE *fd, const char *FROM,
	const char *TO, const char *SUBJ, int replyto_area, bool is_reply)
{
	enum {area, date, from, to, subj, items};
	static const char *names[items] = {"ly in", "ly on", "ly by",
		"ly to", " subj"};
	char Header[512], *p;
	int j;

	int oldarea = mm.letterList->getAreaID();
	mm.areaList->gotoArea(mm.letterList->getAreaID());

	const char *head[items] = {
		mm.areaList->getDescription(), mm.letterList->getDate(),
		mm.letterList->getFrom(), mm.letterList->getTo(),
		mm.letterList->getSubject()
	};

	const char *org[items] = {
		0, 0, FROM, TO, SUBJ
	};

	bool use[items];

	use[area] = (oldarea != replyto_area);
	use[date] = !is_reply;
	for (j = from; j < items; j++)
		use[j] = !(!strcasecmp(org[j], head[j]));

	interface->areas.Select();

	bool anyused = false;

	for (j = 0; j < items; j++)
		if (use[j]) {
			p = Header + sprintf(Header, "%.511s", head[j]);
			if (((j == from) && mm.areaList->isEmail())
			    || ((j == to) && mm.areaList->isReplyArea()))
				p = netAdd(p);
			letterconv_in(Header);
			fprintf(fd, " * Original%s: %s\n", names[j], Header);
			anyused = true;
		}

	if (anyused)
		fprintf(fd, "\n");
}

void LetterWindow::EditLetter(bool forwarding)
{
	FILE *reply;
	char reply_filename[256], FROM[74], TO[514], SUBJ[514], *body;
	const char *msgid;
	long siz;
	int replyto_num, replyto_area;
	bool privat, newsflag = false;

	bool is_reply_area = (mm.areaList->getAreaNo() == REPLY_AREA);

	NM = mm.letterList->getNetAddr();

	replyto_area = interface->areaMenu();
	if (replyto_area == -1)
		return;

	// The refnum is only good for the original area:

	replyto_num = (replyto_area != mm.letterList->getAreaID()) ?
		0 : mm.letterList->getReplyTo();

	msgid = replyto_num ? mm.letterList->getMsgID() : 0;

	mm.areaList->gotoArea(replyto_area);

	if (forwarding) {
		if (mm.areaList->isEmail()) {
			interface->areas.Select();
			interface->addressbook();
			mm.areaList->gotoArea(replyto_area);
		} else {
			NM.isSet = false;
			newsflag = mm.areaList->isUsenet();
		}
		key = 'E';
		setToFrom(TO, FROM);
		sprintf(SUBJ, "%.513s", mm.letterList->getSubject());
		privat = false;
	} else {
		const char *newsgrps = mm.letterList->getNewsgrps();
		newsflag = !(!newsgrps);

		strcpy(FROM, mm.letterList->getFrom());
		sprintf(TO, newsflag ? "%.512s" : "%.72s", newsflag ?
			newsgrps : mm.letterList->getTo());
		sprintf(SUBJ, "%.512s", mm.letterList->getSubject());
		privat = mm.letterList->getPrivate();
	}

	if (!EnterHeader(FROM, TO, SUBJ, privat)) {
		NM.isSet = false;
		interface->areas.Select();
		interface->redraw();
		return;
	}

	DestroyChain();		// current letter's chain reset

	mytmpnam(reply_filename);

	body = (char *) mm.letterList->getBody();
	letterconv_in(body);

	// Can't use MakeChain here, or it will wrap the text; so:
	if (!hidden)		// skip hidden lines at start
		while (*body == 1) {
			do
				body++;
			while (*body && (*body != '\n'));
			while (*body == '\n')
				body++;
		}

	reply = fopen(reply_filename, "wt");
	if (forwarding)
		forward_header(reply, FROM, TO, SUBJ, replyto_area,
			is_reply_area);
	fwrite(body, strlen(body), 1, reply);
	fclose(reply);
	body = 0;		// it will be auto-dealloc'd by next getBody

	edit(reply_filename);
	siz = reconvert(reply_filename);

	if (!forwarding)
		mm.areaList->killLetter(mm.letterList->getMsgNum());

	mm.areaList->enterLetter(replyto_area, FROM, newsflag ? "All" :
		TO, SUBJ, msgid, newsflag ? TO : 0, replyto_num, privat,
		NM, reply_filename, (int) siz);

	if (is_reply_area) {
		mm.letterList->rrefresh();
		interface->letters.ResetActive();
	}
	interface->areas.Select();

	NM.isSet = false;

	interface->setUnsaved();
}

bool LetterWindow::SplitLetter(int lines)
{
	static int eachmax = mm.resourceObject->getInt(MaxLines);

	if (!lines) {
		char maxlinesA[55];

		sprintf(maxlinesA, "%d", eachmax);
		if (!interface->savePrompt(
	    	"Max lines per part? (WARNING: Split is not reversible!)",
			maxlinesA) || !sscanf(maxlinesA, "%d", &eachmax))
				return false;
	}
	int maxlines = lines ? lines : eachmax;

	if (maxlines < 20)
		return false;

	MakeChain(80);
	int orglines = NumOfLines;
	int parts = (orglines - 1) / maxlines + 1;

	if (parts == 1)
		return false;

	NM = mm.letterList->getNetAddr();

	int replyto_area = mm.letterList->getAreaID();
	int replyto_num = mm.letterList->getReplyTo();

	char ORGSUBJ[514], *from, *to, *msgid, *newsgrps;

	from = strdupplus(mm.letterList->getFrom());
	to = strdupplus(mm.letterList->getTo());
	msgid = replyto_num ? strdupplus(mm.letterList->getMsgID()) : 0;
	newsgrps = strdupplus(mm.letterList->getNewsgrps());

	sprintf(ORGSUBJ, "%.510s", mm.letterList->getSubject());

	bool privat = mm.letterList->getPrivate();

	int clines = 0;

	mm.areaList->gotoArea(replyto_area);
	mm.areaList->killLetter(mm.letterList->getMsgNum());

	for (int partno = 1; partno <= parts; partno++) {
		FILE *reply;
		char reply_filename[256], SUBJ[514];

		mytmpnam(reply_filename);

		int endline = (orglines > maxlines) ? maxlines: orglines;

		reply = fopen(reply_filename, "wt");
		for (int i = clines; i < (clines + endline); i++)
			fprintf(reply, "%s\n", linelist[i]->text);
		fclose(reply);

		mystat fileStat;
		fileStat.init(reply_filename);

		sprintf(SUBJ, "%s [%d/%d]", ORGSUBJ, partno, parts);

		mm.areaList->enterLetter(replyto_area, from, to, SUBJ, msgid,
			newsgrps, replyto_num, privat, NM, reply_filename,
			(int) fileStat.size);

		clines += endline;
		orglines -= endline;
	}
	delete[] newsgrps;
	delete[] msgid;
	delete[] to;
	delete[] from;

	NM.isSet = false;

	mm.letterList->rrefresh();

	if (!lines) {
		interface->letters.ResetActive();
		interface->areas.Select();
		interface->setUnsaved();
	}
	return true;
}

void LetterWindow::GetTagline()
{
	interface->taglines.EnterTagline(tagline1);
	ReDraw();
}

bool LetterWindow::EditOriginal()
{
	int old_area = mm.letterList->getAreaID();
	int old_mnum = mm.letterList->getMsgNum();
	int orig_area = mm.areaList->getAreaNo();
	int orig_mnum = mm.letterList->getCurrent();
	int oldPos = position;
	position = 0;

	letter_list *old_list = mm.letterList;
	mm.areaList->gotoArea(REPLY_AREA);
	mm.areaList->getLetterList();
	interface->areas.ResetActive();

	bool found = mm.letterList->findReply(old_area, old_mnum);

	if (found && interface->WarningWindow("A reply exists. Re-edit it?"))
		EditLetter(false);
	else
		found = false;

	position = oldPos;
	delete mm.letterList;
	mm.letterList = old_list;
	mm.areaList->gotoArea(orig_area);
	interface->areas.ResetActive();
	mm.letterList->gotoLetter(orig_mnum);
	interface->letters.ResetActive();

	return found;
}

void LetterWindow::SplitAll(int lines)
{
	letter_list *old_list = 0;
	statetype state = interface->active();
	bool list_is_active = (state == letter) || (state == letterlist);

	interface->areas.Select();
	int orig_area = mm.areaList->getAreaNo();

	bool is_reply_area = (orig_area == REPLY_AREA) && list_is_active;

	if (!is_reply_area) {
		if (list_is_active)
			old_list = mm.letterList;

		mm.areaList->gotoArea(REPLY_AREA);
		mm.areaList->getLetterList();
		interface->areas.ResetActive();
	}

	bool anysplit = false;
	int last = mm.letterList->noOfActive();
	for (int i = 0; i < last; i++) {
		mm.letterList->gotoActive(i);
		if (SplitLetter(lines)) {
			i--;
			last--;
			anysplit = true;
		}
	}

	if (is_reply_area)
		interface->letters.ResetActive();
	else {
		delete mm.letterList;
		if (list_is_active)
			mm.letterList = old_list;
	}
	mm.areaList->gotoArea(orig_area);
	interface->areas.ResetActive();

	if ((state == letter) && !is_reply_area)
		MakeChain(COLS);

	if (anysplit)
		interface->nonFatalError("Some replies were split");
}
