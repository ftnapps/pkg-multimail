/*
 * MultiMail offline mail reader
 * OPX (Silver Xpress)

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "opx.h"
#include "compress.h"

struct tm *getdostime(unsigned long packed)
{
	static struct tm unpacked;

	unpacked.tm_mday = packed & 0x1f;
	packed >>= 5;
	unpacked.tm_mon = (packed & 0x0f) - 1;
	packed >>= 4;
	unpacked.tm_year = (packed & 0x7f) + 80;
	packed >>= 7;

	unpacked.tm_sec = (packed & 0x1f) << 1;
	packed >>= 5;
	unpacked.tm_min = packed & 0x3f;
	packed >>= 6;
	unpacked.tm_hour = packed & 0x1f;

	return &unpacked;
}

unsigned long mkdostime(struct tm *unpacked)
{
	unsigned long packed;

	packed = unpacked->tm_hour;
	packed <<= 6;
	packed |= unpacked->tm_min;
	packed <<= 5;
	packed |= unpacked->tm_sec >> 1;
	packed <<= 7;

	packed |= unpacked->tm_year - 80;
	packed <<= 4;
	packed |= unpacked->tm_mon + 1;
	packed <<= 5;
	packed |= unpacked->tm_mday;

	return packed;
}

// ---------------------------------------------------------------------------
// The OPX methods
// ---------------------------------------------------------------------------

opxpack::opxpack(mmail *mmA)
{
	mm = mmA;
	ID = 0;
	bodyString = 0;

	readBrdinfoDat();

	if (!(infile = mm->workList->ftryopen("mail.dat", "rb")))
		fatalError("Could not open MAIL.DAT");

	buildIndices();
}

opxpack::~opxpack()
{
	cleanup();
}

area_header *opxpack::getNextArea()
{
	int cMsgNum = areas[ID].nummsgs;
	bool x = (areas[ID].num == - 1);

	area_header *tmp = new area_header(mm,
		ID + mm->driverList->getOffset(this),
		areas[ID].numA, areas[ID].name,
		(x ? "Letters addressed to you" : areas[ID].name),
		(x ? "OPX personal" : "OPX"),
		areas[ID].attr | (cMsgNum ? ACTIVE : 0),
		cMsgNum, 0, 35, 71);
	ID++;
	return tmp;
}

void opxpack::buildIndices()
{
	msgHead mhead;
	int x;

	body = new bodytype *[maxConf];

	for (x = 0; x < maxConf; x++) {
		body[x] = 0;
		areas[x].nummsgs = 0;
	}

	ndx_fake base, *tmpndx = &base;

	int counter, length, personal = 0;
	const char *name = mm->resourceObject->get(LoginName);
	bool checkpers = mm->resourceObject->getInt(BuildPersArea);

	numMsgs = 0;

	while (fread(&mhead, sizeof mhead, 1, infile)) {
		counter = ftell(infile);
		x = getXNum(getshort(mhead.confnum));
		length = getshort(mhead.length) - 0xbe;

		fseek(infile, length, SEEK_CUR);

		tmpndx->next = new ndx_fake;
		tmpndx = tmpndx->next;

		tmpndx->confnum = x;

		if (checkpers && !strcasecmp(mhead.to, name)) {
			tmpndx->pers = true;
			personal++;
		} else
			tmpndx->pers = false;

		tmpndx->pointer = counter;
		tmpndx->length = length;

		numMsgs++;
		areas[x].nummsgs++;
	}

	initBody(base.next, personal);
}

letter_header *opxpack::getNextLetter()
{
	msgHead mhead;
	unsigned long pos, rawpos;
	int areaID, letterID;

	rawpos = body[currentArea][currentLetter].pointer;
	pos = rawpos - sizeof mhead;

	fseek(infile, pos, SEEK_SET);
	if (!fread(&mhead, sizeof mhead, 1, infile))
		fatalError("Error reading MAIL.DAT");

	if (areas[currentArea].num == -1) {
		areaID = getXNum(getshort(mhead.confnum));
		letterID = getYNum(areaID, rawpos);
	} else {
		areaID = currentArea;
		letterID = currentLetter;
	}

        net_address na;
	if (areas[areaID].attr & INTERNET)
		na = mhead.from;
	else {
		na.zone = getshort(mhead.orig_zone);
		if (na.zone) {
			na.net = getshort(mhead.orig_net);
			na.node = getshort(mhead.orig_node);
			na.point = 0;   // set from getBody()
			na.isSet = true;
		}
	}

	bool privat = getshort(mhead.attr) & OPX_PRIVATE;

	char date[30];
	strftime(date, 30, "%b %d %Y  %H:%M",
		getdostime(getlong(mhead.date_written)));

	currentLetter++;

	return new letter_header(mm, mhead.subject, mhead.to, mhead.from,
		date, 0, getshort(mhead.reply), letterID,
		getshort(mhead.msgnum), areaID, privat,
		getshort(mhead.length), this, na,
		!(!(areas[areaID].attr & LATINCHAR)));
}

// returns the body of the requested letter
const char *opxpack::getBody(letter_header &mhead)
{
	unsigned char *p;
	int c, kar, AreaID, LetterID;

	AreaID = mhead.getAreaID() - mm->driverList->getOffset(this);
	LetterID = mhead.getLetterID();

	delete[] bodyString;
	bodyString = new char[body[AreaID][LetterID].msgLength + 1];
	fseek(infile, body[AreaID][LetterID].pointer, SEEK_SET);

	for (c = 0, p = (unsigned char *) bodyString;
	     c < body[AreaID][LetterID].msgLength; c++) {
		kar = fgetc(infile);

		if (!kar)
			kar = ' ';

		// "Soft CRs" are misused in some OPX packets:
		if (kar == 0x8d)
			kar = '\n';

		if (kar != '\r')
			*p++ = kar;
	}
	do
		p--;
	while ((*p == ' ') || (*p == '\n'));	// Strip blank lines
	p[1] = '\0';


	// Extra header info embedded in the text:

	const char *s;
	char *end;

	net_address &na = mhead.getNetAddr();

	if ((s = getHidden("\001INETORIG ", end))) {
		na = s;
		*end = '\n';
	} else {
		// Add point to netmail address, if possible/necessary:
		if (na.isSet)
			if (!na.point) {
				s = strstr(bodyString, "\001FMPT");
				if (s)
					sscanf(s, "\001FMPT%d\n", &na.point);
			}
	}

	// Get MSGID:
	if (!mhead.getMsgID())
		if ((s = getHidden("\001MSGID: ", end))) {
			mhead.changeMsgID(s);
			*end = '\n';
		}

	// Change to Latin character set, if necessary:
	checkLatin(mhead);

	return bodyString;
}

char *opxpack::pstrget(void *src)
{
	unsigned len = (unsigned) *((unsigned char *) src);
	char *dest = new char[len + 1];

	strncpy(dest, ((const char *) src) + 1, len);
	dest[len] = '\0';

	return dest;
}

void opxpack::readBrdinfoDat()
{
	FILE *brdinfoFile;
	brdHeader header;
	brdRec boardrec;
	pstring(readerf,12);
	char *p;

	if (!(brdinfoFile = mm->workList->ftryopen("brdinfo.dat", "rb")))
		fatalError("Could not open BRDINFO.DAT");

	if (!fread(&header, sizeof header, 1, brdinfoFile))
		fatalError("Error reading BRDINFO.DAT");

	p = pstrget(&header.bbsid);
	strcpy(packetBaseName, p);
	delete[] p;

	p = pstrget(&header.bbsname);
	mm->resourceObject->set(BBSName, p);
	delete[] p;

	p = pstrget(&header.sysopname);
	mm->resourceObject->set(SysOpName, p);
	delete[] p;

	p = pstrget(&header.username);
	mm->resourceObject->set(LoginName, p);
	mm->resourceObject->set(AliasName, p);
	delete[] p;

	char *bulls = new char[header.readerfiles * 13];

	for (int c = 0; c < header.readerfiles; c++) {
		fread(&readerf, sizeof readerf, 1, brdinfoFile);
		strncpy(bulls + c * 13, readerf.data, readerf.len);
		bulls[c * 13 + readerf.len] = '\0';
	}

	listBulletins((const char (*)[13]) bulls,
		header.readerfiles, 1);

	maxConf = getshort(header.numofareas) + 1;
	areas = new AREAs[maxConf];

	areas[0].num = -1;
	strcpy(areas[0].numA, "PERS");
	areas[0].name = strdupplus("PERSONAL");
	areas[0].attr = PUBLIC | PRIVATE | COLLECTION;

	for (int c = 1; c < maxConf; c++) {
		fread(&boardrec, sizeof boardrec, 1, brdinfoFile);

		areas[c].num = getshort(boardrec.confnum);
		sprintf(areas[c].numA, "%d", areas[c].num);

		areas[c].name = pstrget(&boardrec.name);

		areas[c].attr = (((boardrec.attrib & OPX_NETMAIL) |
			(boardrec.attrib2 & OPX_INTERNET)) ? NETMAIL : 0) |
			((boardrec.attrib & OPX_PRIVONLY) ? 0 : PUBLIC) |
			((boardrec.attrib & OPX_PUBONLY) ? 0 : PRIVATE) |
			(((boardrec.attrib2 & OPX_USENET) |
			(boardrec.attrib2 & OPX_INTERNET)) ? (INTERNET |
			LATINCHAR) : 0);
	}

	fclose(brdinfoFile);
}

// ---------------------------------------------------------------------------
// The OPX reply methods
// ---------------------------------------------------------------------------

opxreply::upl_opx::upl_opx()
{
	msgid = 0;
}

opxreply::upl_opx::~upl_opx()
{
	delete[] msgid;
}

opxreply::opxreply(mmail *mmA, specific_driver *baseClassA)
{
	mm = mmA;
	baseClass = (pktbase *) baseClassA;

	replyText = 0;
	uplListHead = 0;

	replyExists = false;
}

opxreply::~opxreply()
{
	cleanup();
}

int opxreply::getArea(const char *fname)
{
	int sum = 0;

	fname = strchr(fname, '.') + 1;
	for (int x = 0; x < 3; x++) {
		sum *= 36;

		unsigned char c = toupper(fname[x]);
		if ((c >= '0') && (c <= '9'))
			sum += c - '0';
		else
			if ((c >= 'A') && (c <= 'Z'))
				sum += c - 'A' + 10;
	}
	return sum;
}

bool opxreply::getRep1(const char *orgname, upl_opx *l)
{
	FILE *orgfile, *destfile;
	int c, count = 0;

	mytmpnam(l->fname);

	if ((orgfile = fopen(orgname, "rb"))) {

	    fread(&l->rhead, sizeof(repHead), 1, orgfile);
	    l->area = getArea(orgname);

	    net_address na;
	    na.zone = getshort(l->rhead.dest_zone);
	    if (na.zone) {
		na.net = getshort(l->rhead.dest_net);
		na.node = getshort(l->rhead.dest_node);
		na.point = 0;
		na.isSet = true;
	    }

	    if ((destfile = fopen(l->fname, "wt"))) {
		while ((c = fgetc(orgfile)) != EOF) {
			if (c == '\001') {
				c = fgetc(orgfile);
				int x;
				bool isReply = (c == 'R'),
					isPoint = (c == 'T'),
					isInet = (c == 'I');

				if (isReply || isPoint || isInet) {
					for (x = 0; x < (isInet ? 8 :
					    (isPoint ? 4 : 6)); x++)
						fgetc(orgfile);
				}
				x = 0;
				while ((c != EOF) && (c != '\n')) {
					c = fgetc(orgfile);
					x++;
				}
				if (isReply || isPoint || isInet) {
					char *tmp = new char[x];
					fseek(orgfile, x * -1, SEEK_CUR);
					fread(tmp, 1, x, orgfile);
					strtok(tmp, "\r");
				
					if (isReply)
						l->msgid = tmp;
					else {
						if (isInet)
							na = tmp;
						else
							sscanf(tmp, "%d",
								&na.point);
						delete[] tmp;
					}
				}
				c = '\r';
			}
			if (c != '\r') {
				fputc(c, destfile);
				count++;
			}
		}
		fclose(destfile);
	    }

	    l->na = na;

	    fclose(orgfile);
	}

	l->msglen = count;

	remove(orgname);

	return true;
}

void opxreply::getReplies(FILE *repFile)
{
	noOfLetters = 0;

	upl_opx baseUplList, *currUplList = &baseUplList;
	const char *p;

	upWorkList->gotoFile(-1);
	while ((p = upWorkList->getNext("!"))) {
		currUplList->nextRecord = new upl_opx;
		currUplList = (upl_opx *) currUplList->nextRecord;
		if (!getRep1(p, currUplList)) {
			delete currUplList;
			break;
		}
		noOfLetters++;
	}
	uplListHead = baseUplList.nextRecord;

	repFile = repFile;	// warning supression
}

area_header *opxreply::getNextArea()
{
	return new area_header(mm, 0, "REPLY", "REPLIES",
		"Letters written by you", "OPX replies",
		(COLLECTION | REPLYAREA | ACTIVE | PUBLIC | PRIVATE),
		noOfLetters, 0, 35, 71);
}

letter_header *opxreply::getNextLetter()
{
	upl_opx *current = (upl_opx *) uplListCurrent;

	char date[30];
	strftime(date, 30, "%b %d %Y  %H:%M",
		getdostime(getlong(current->rhead.date_written)));

	int area = ((opxpack *) baseClass)->getXNum(current->area) + 1;

	letter_header *newLetter = new letter_header(mm,
		current->rhead.subject, current->rhead.to,
		current->rhead.from, date, current->msgid,
		getshort(current->rhead.reply), currentLetter, currentLetter,
		area, getshort(current->rhead.attr) & OPX_PRIVATE,
		current->msglen, this, current->na,
		mm->areaList->isLatin(area));

	currentLetter++;
	uplListCurrent = uplListCurrent->nextRecord;
	return newLetter;
}

void opxreply::enterLetter(letter_header &newLetter,
				const char *newLetterFileName, int length)
{
	upl_opx *newList = new upl_opx;
	memset(newList, 0, sizeof(upl_opx));

	int attrib = newLetter.getPrivate() ? OPX_PRIVATE : 0;

	strncpy(newList->rhead.subject, newLetter.getSubject(), 71);
	strncpy(newList->rhead.from, newLetter.getFrom(), 35);
	strncpy(newList->rhead.to, newLetter.getTo(), 35);

	const char *msgid = newLetter.getMsgID();
	if (msgid)
		newList->msgid = strdupplus(msgid);

	newList->area = atoi(mm->areaList->getShortName());
	newList->na = newLetter.getNetAddr();

	putshort(newList->rhead.attr, attrib);
	putshort(newList->rhead.reply, newLetter.getReplyTo());

	time_t now = time(0);
	strftime(newList->rhead.date, 20, "%d %b %y  %H:%M:%S",
		localtime(&now));
	unsigned long dostime = mkdostime(localtime(&now));
	putlong(newList->rhead.date_written, dostime);
	putlong(newList->rhead.date_arrived, dostime);

	strcpy(newList->fname, newLetterFileName);
	newList->msglen = length;

	addUpl(newList);
}

const char *opxreply::freeFileName(upl_opx *l)
{
	static char fname[13];
	char ext[4];
	unsigned area[3], x;

	area[2] = l->area;
	area[0] = area[2] / (36 * 36);
	area[2] %= (36 * 36);
        area[1] = area[2] / 36;
	area[2] %= 36;

	for (x = 0; x < 3; x++)
		ext[x] = area[x] + ((area[x] < 10) ? '0' : ('A' - 10));
	ext[3] = '\0';

	int reply = getshort(l->rhead.reply);
	if (reply) {
		sprintf(fname, "!R%d.%s", reply, ext);
		if (!readable(fname))
			return fname;
	}
	x = 1;
	do
		sprintf(fname, "!N%d.%s", x++, ext);
	while (readable(fname));

	return fname;
}

void opxreply::addRep1(FILE *rep, upl_base *node, int recnum)
{
	FILE *orgfile, *destfile;
	upl_opx *l = (upl_opx *) node;
	const char *dest;

	recnum = recnum;	// warning supression
	rep = rep;

	dest = freeFileName(l);

	if ((orgfile = fopen(l->fname, "rt"))) {

		char *replyText = new char[l->msglen + 1];

		fread(replyText, l->msglen, 1, orgfile);
		fclose(orgfile);

		replyText[l->msglen] = '\0';

		if ((destfile = fopen(dest, "wb"))) {

			fwrite(&l->rhead, sizeof(repHead), 1, destfile);

			bool skipPID = false;

			if (l->na.isSet) {
			    if (l->na.isInternet) {
				fprintf(destfile, "\001INETDEST %s\r\n",
					(const char *) l->na);
				skipPID = true;
			    } else {
				putshort(l->rhead.dest_zone, l->na.zone);
				putshort(l->rhead.dest_net, l->na.net);
				putshort(l->rhead.dest_node, l->na.node);

				// I should probably add the originating
				// address here, but it's frequently
				// bogus. Let's try it this way.

				if (l->na.point)
					fprintf(destfile, "\001TOPT %d\r\n",
						l->na.point);
			    }
			}

			if (l->msgid)
				fprintf(destfile, "\001REPLY: %s\r\n",
					l->msgid);

			if (!skipPID)
				fprintf(destfile, "\001PID: " MM_NAME
					"/%s v%d.%d\r\n", sysname(),
						MM_MAJOR, MM_MINOR);

			char *lastsp = 0, *q = replyText;
			int count = 0;

			for (char *p = replyText; *p; p++) {
				if (*p == '\n') {
					*p = '\0';
					fprintf(destfile, "%s\r\n", q);
					q = p + 1;
					count = 0;
					lastsp = 0;
				} else {
					count++;
					if (*p == ' ')
						lastsp = p;
				}

				// wrap at 80 characters
				if ((count >= 80) && lastsp) {
					*lastsp = '\n';
					p = lastsp - 1;
				}
			}
                        if (count)
                                fprintf(destfile, "%s\r\n", q);
                        fclose(destfile);
		}
		delete[] replyText;
	}
}

void opxreply::addHeader(FILE *repFile)
{
	// I don't really understand <BBSID>.ID, but I can fake it. I use
	// the version number "4.4" since that's the SX reader I've tested
	// with, and I don't bother stamping the fake date.

	long magic = -598939720L;	// What IS this, anyway?
	time_t now = time(0);

	fprintf(repFile, "FALSE\r\n\r\n4.4\r\nTRUE\r\n%ld\r\n0\r\n%ld\r\n",
		(signed long) mkdostime(localtime(&now)), magic);
}

void opxreply::repFileName()
{
	int x;
	const char *basename = baseClass->getBaseName();

	for (x = 0; basename[x]; x++) {
		replyPacketName[x] = tolower(basename[x]);
		replyInnerName[x] = toupper(basename[x]);
	}
	strcpy(replyPacketName + x, ".rep");
	strcpy(replyInnerName + x, ".ID");
}

const char *opxreply::repTemplate(bool offres)
{
	offres = offres;

	return "*.*";
}

bool opxreply::getOffConfig()
{
	return false;
}

bool opxreply::makeOffConfig()
{
	return false;
}
