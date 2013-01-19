/*
 * MultiMail offline mail reader
 * QWK

 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "qwk.h"
#include "compress.h"

unsigned char *onecomp(unsigned char *p, char *dest, const char *comp)
{
	int len = strlen(comp);

	if (!strncasecmp((char *) p, comp, len)) {
		p += len;
		while (*p == ' ')
			p++;
		int x;
		for (x = 0; *p && (*p != '\n') && (x < 71); x++)
			dest[x] = *p++;
		dest[x] = '\0';

		while (*p == '\n')
			p++;
		return p;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// The qheader methods
// ---------------------------------------------------------------------------

bool qheader::init(FILE *datFile)
{
	qwkmsg_header qh;
	char buf[9];

	if (!fread(&qh, 1, sizeof qh, datFile))
		return false;

	getQfield(from, qh.from, 25);
	getQfield(to, qh.to, 25);
	getQfield(subject, qh.subject, 25);

	cropesp(from);
	cropesp(to);
	cropesp(subject);

	getQfield(date, qh.date, 8);
	date[2] = '-';
	date[5] = '-';		// To deal with some broken messages
	strcat(date, " ");

	getQfield(buf, qh.time, 5);
	strcat(date, buf);

	getQfield(buf, qh.refnum, 8);
	refnum = atoi(buf);

	getQfield(buf, qh.msgnum, 7);
	msgnum = atoi(buf);

	getQfield(buf, qh.chunks, 6);
	msglen = (atoi(buf) - 1) << 7;

	privat = (qh.status == '*') || (qh.status == '+');

	// Is this a block of net-status flags?
	netblock = !qh.status || (qh.status == '\xff');

	origArea = getshort(&qh.confLSB);

	return true;
}

void qheader::output(FILE *repFile)
{
	qwkmsg_header qh;
	char buf[10];
	int chunks, length, sublen;

	length = msglen;
	sublen = strlen(subject);
	if (sublen > 25) {
		length += sublen + 11;
		sublen = 25;
	}

	memset(&qh, ' ', sizeof qh);

	chunks = (length + 127) / 128;
	if (!chunks)
		chunks = 1;

	sprintf(buf, " %-6d", msgnum);
	strncpy(qh.msgnum, buf, 7);

	putshort(&qh.confLSB, msgnum);

	if (refnum) {
		sprintf(buf, " %-7d", refnum);
		strncpy(qh.refnum, buf, 8);
	}
	strncpy(qh.to, to, strlen(to));
	strncpy(qh.from, from, strlen(from));
	strncpy(qh.subject, subject, sublen);

	qh.alive = (char) 0xE1;

	strncpy(qh.date, date, 8);
	strncpy(qh.time, &date[9], 5);

	sprintf(buf, "%-6d", chunks + 1);
	strncpy(qh.chunks, buf, 6);
	if (privat)
		qh.status = '*';

	fwrite(&qh, 1, sizeof qh, repFile);
}

// ---------------------------------------------------------------------------
// The QWK methods
// ---------------------------------------------------------------------------

qwkpack::qwkpack(mmail *mmA)
{
	mm = mmA;
	ID = 0;
	bodyString = 0;

	qwke = !(!mm->workList->exists("toreader.ext"));

	readControlDat();
	if (qwke)
		readToReader();
	else
		readDoorId();

	if (!(infile = mm->workList->ftryopen("messages.dat", "rb")))
		fatalError("Could not open MESSAGES.DAT");

	readIndices();

	listBulletins(textfiles, 3);
}

qwkpack::~qwkpack()
{
	cleanup();
}

unsigned long qwkpack::MSBINtolong(unsigned const char *ms)
{
	return ((((unsigned long) ms[0] + ((unsigned long) ms[1] << 8) +
		  ((unsigned long) ms[2] << 16)) | 0x800000L) >>
			(24 + 0x80 - ms[3]));
}

area_header *qwkpack::getNextArea()
{
	int cMsgNum = areas[ID].nummsgs;
	bool x = (areas[ID].num == -1);

	area_header *tmp = new area_header(mm,
		ID + mm->driverList->getOffset(this),
		areas[ID].numA, areas[ID].name,
		(x ? "Letters addressed to you" : areas[ID].name),
		(qwke ? (x ? "QWKE personal" : "QWKE") :
		(x ? "QWK personal" : "QWK")),
		areas[ID].attr | hasOffConfig | (cMsgNum ? ACTIVE : 0),
		cMsgNum, 0, 25, qwke ? 72 : 25);
	ID++;
	return tmp;
}

bool qwkpack::isQWKE()
{
	return qwke;
}

bool qwkpack::externalIndex()
{
	const char *p;
	int x, cMsgNum;

	hasPers = !(!mm->workList->exists("personal.ndx"));

	bool hasNdx = (p = mm->workList->exists(".ndx"));
	if (hasNdx) {
		struct {
        		unsigned char MSB[4];
        		unsigned char confnum;
		} ndx_rec;

		FILE *idxFile;
		char fname[13];

		if (!hasPers) {
			areas++;
			maxConf--;
		}

		while (p) {
			cMsgNum = 0;

			strncpy(fname, p, strlen(p) - 4);
			fname[strlen(p) - 4] = '\0';
			x = atoi(fname);
			if (!x)
				if (!strcasecmp(fname, "personal"))
					x = -1;
			x = getXNum(x);

			if ((idxFile = mm->workList->ftryopen(p, "rb"))) {
				cMsgNum = mm->workList->getSize() / ndxRecLen;
				body[x] = new bodytype[cMsgNum];
				for (int y = 0; y < cMsgNum; y++) {
					fread(&ndx_rec, ndxRecLen, 1, idxFile);
					body[x][y].pointer =
						MSBINtolong(ndx_rec.MSB);
				}
				fclose(idxFile);
			}

			areas[x].nummsgs = cMsgNum;
			numMsgs += cMsgNum;

			p = mm->workList->getNext(".ndx");
		}
	}
	return hasNdx;
}

void qwkpack::readIndices()
{
	int x;

	body = new bodytype *[maxConf];

	for (x = 0; x < maxConf; x++) {
		body[x] = 0;
		areas[x].nummsgs = 0;
	}

	numMsgs = 0;

	if (!externalIndex()) {
		ndx_fake base, *tmpndx = &base;

		int counter, personal = 0;
		const char *name = mm->resourceObject->get(LoginName);
		const char *alias = mm->resourceObject->get(AliasName);
		bool checkpers = mm->resourceObject->getInt(BuildPersArea);

		qheader qHead;

		fseek(infile, 128, SEEK_SET);
		while (qHead.init(infile))
		    if (!qHead.netblock) {	// skip net-status flags
			counter = ftell(infile) >> 7;
			x = getXNum(qHead.origArea);

			tmpndx->next = new ndx_fake;
			tmpndx = tmpndx->next;

			tmpndx->confnum = x;

			if (checkpers && (!strcasecmp(qHead.to, name) ||
			    (qwke && !strcasecmp(qHead.to, alias)))) {
				tmpndx->pers = true;
				personal++;
			} else
				tmpndx->pers = false;

			tmpndx->pointer = counter;
			tmpndx->length = 0;	// temp

			areas[x].nummsgs++;
			numMsgs++;

			fseek(infile, qHead.msglen, SEEK_CUR);
		    }

		initBody(base.next, personal);
	}
}

letter_header *qwkpack::getNextLetter()
{
	qheader q;
	unsigned long pos, rawpos;
	int areaID, letterID;

	rawpos = body[currentArea][currentLetter].pointer;
	pos = (rawpos - 1) << 7;

	fseek(infile, pos, SEEK_SET);
	if (!q.init(infile))
		fatalError("Error reading MESSAGES.DAT");

	if (areas[currentArea].num == -1) {
		areaID = getXNum(q.origArea);
		letterID = getYNum(areaID, rawpos);
	} else {
		areaID = currentArea;
		letterID = currentLetter;
	}

	body[areaID][letterID].msgLength = q.msglen;

	currentLetter++;

	static net_address nullNet;

	return new letter_header(mm, q.subject, q.to, q.from,
		q.date, 0, q.refnum, letterID, q.msgnum, areaID,
		q.privat, q.msglen, this, nullNet,
		!(!(areas[areaID].attr & LATINCHAR)));
}

// returns the body of the requested letter
const char *qwkpack::getBody(letter_header &mhead)
{
	unsigned char *p;
	int c, kar, AreaID, LetterID;

	AreaID = mhead.getAreaID() - mm->driverList->getOffset(this);
	LetterID = mhead.getLetterID();

	delete[] bodyString;
	bodyString = new char[body[AreaID][LetterID].msgLength + 1];
	fseek(infile, body[AreaID][LetterID].pointer << 7, SEEK_SET);

	for (c = 0, p = (unsigned char *) bodyString;
	     c < body[AreaID][LetterID].msgLength; c++) {
		kar = fgetc(infile);

		if (!kar)
			kar = ' ';

		*p++ = (kar == 227) ? '\n' : kar;	
	}
	do
		p--;
	while ((*p == ' ') || (*p == '\n'));	// Strip blank lines
	p[1] = '\0';

	// Get extended (QWKE-type) info, if available:

	char extsubj[72];	// extended subject or other line
	bool anyfound;
	unsigned char *q;
	p = (unsigned char *) bodyString;

	do {
		anyfound = false;

		q = onecomp(p, extsubj, "subject:");

		if (!q) {
			q = onecomp(p + 1, extsubj, "@subject:");
			if (q) {
				extsubj[strlen(extsubj) - 1] = '\0';
				cropesp(extsubj);
			}
		}

		// For WWIV QWK door:
		if (!q)
			q = onecomp(p, extsubj, "title:");

		if (q) {
			p = q;
			mhead.changeSubject(extsubj);
			anyfound = true;
		}

		q = onecomp(p, extsubj, "to:");
		if (q) {
			p = q;
			mhead.changeTo(extsubj);
			anyfound = true;
		}

		q = onecomp(p, extsubj, "from:");
		if (q) {
			p = q;
			mhead.changeFrom(extsubj);
			anyfound = true;
		}

	} while (anyfound);

	// Change to Latin character set, if necessary:
	checkLatin(mhead);

	return (char *) p;
}

void qwkpack::readControlDat()
{
	char *p, *q;

	if (!(infile = mm->workList->ftryopen("control.dat", "rb")))
		fatalError("Could not open CONTROL.DAT");

	mm->resourceObject->set(BBSName, nextLine());	// 1: BBS name
	nextLine();					// 2: city/state
	nextLine();					// 3: phone#

	q = nextLine();					// 4: sysop's name
	int slen = strlen(q);
	if (slen > 6) {
		p = q + slen - 5;
		if (!strcasecmp(p, "Sysop")) {
			if (*--p == ' ')
				p--;
			if (*p == ',')
				*p = '\0';
		}
	}
	mm->resourceObject->set(SysOpName, q);

	q = nextLine();					// 5: doorserno,BBSid
	p = strtok(q, ",");
	p = strtok(0, " ");
	strncpy(packetBaseName, p, 8);
	packetBaseName[8] = '\0';

	nextLine();					// 6: time&date

	p = nextLine();					// 7: user's name
	cropesp(p);
	mm->resourceObject->set(LoginName, p);
	mm->resourceObject->set(AliasName, p);

	nextLine();					// 8: blank/any
	nextLine();					// 9: anyth.
	nextLine();					// 10: # messages
							// (not reliable)
	maxConf = atoi(nextLine()) + 2;			// 11: Max conf #

	areas = new AREAs[maxConf];

	areas[0].num = -1;
	strcpy(areas[0].numA, "PERS");
	areas[0].name = strdupplus("PERSONAL");
	areas[0].attr = PUBLIC | PRIVATE | COLLECTION;

	for (int c = 1; c < maxConf; c++) {
		areas[c].num = atoi(nextLine());		// conf #
		sprintf(areas[c].numA, "%d", areas[c].num);
		areas[c].name = strdupplus(nextLine());		// conf name
		areas[c].attr = PUBLIC | PRIVATE;
	}

	for (int c = 0; c < 3; c++)
		strncpy(textfiles[c], nextLine(), 12);

	fclose(infile);
}

void qwkpack::readDoorId()
{
	const char *s;
	bool hasAdd = false, hasDrop = false;

	strcpy(controlname, "QMAIL");

	if ((infile = mm->workList->ftryopen("door.id", "rb"))) {
		while (!feof(infile)) {
			s = nextLine();
			if (!strcasecmp(s, "CONTROLTYPE = ADD"))
			    hasAdd = true;
			else
			    if (!strcasecmp(s, "CONTROLTYPE = DROP"))
				hasDrop = true;
			    else
				if (!strncasecmp(s, "CONTROLNAME", 11))
				    sprintf(controlname, "%.25s", s + 14);
		}
		fclose(infile);
	}
	hasOffConfig = (hasAdd && hasDrop) ? OFFCONFIG : 0;
}

// Read the QWKE file TOREADER.EXT
void qwkpack::readToReader()
{
	char *s;
	int cnum, attr;

	hasOffConfig = OFFCONFIG;

	if ((infile = mm->workList->ftryopen("toreader.ext", "rb"))) {
		while (!feof(infile)) {
			s = nextLine();

			if (!strncasecmp(s, "area ", 5)) {
			    if (sscanf(s + 5, "%d %s", &cnum, s) == 2) {

				attr = SUBKNOWN;

				// If a group is marked subscribed:
				if (strchr(s, 'a'))
					attr |= ACTIVE;
				else
				    if (strchr(s, 'p'))
					attr |= (ACTIVE | PERSONLY);
				    else
					if (strchr(s, 'g'))
					    attr |= (ACTIVE | PERSALL);

				// "Handles" or "Anonymous":
				if (strchr(s, 'H') || strchr(s, 'A'))
					attr |= ALIAS;

				// Public-only/Private-only:
				if (strchr(s, 'P'))
					attr |= PRIVATE;
				else
					if (strchr(s, 'O'))
						attr |= PUBLIC;
					else
						attr |= (PUBLIC | PRIVATE);

				/* Set character set to Latin-1 for
				   Internet or Usenet areas -- but is this
				   the right thing here?
				*/
				if (strchr(s, 'U') || strchr(s, 'I'))
					attr |= LATINCHAR;

				if (strchr(s, 'E'))
					attr |= ECHO;

				areas[getXNum(cnum)].attr = attr;
			    }
			} else
				if (!strncasecmp(s, "alias ", 6)) {
					cropesp(s);
					mm->resourceObject->set(AliasName,
						s + 6);
				}
		}
		fclose(infile);
	}
}

const char *qwkpack::ctrlName()
{
	return controlname;
}

// ---------------------------------------------------------------------------
// The QWK reply methods
// ---------------------------------------------------------------------------

qwkreply::qwkreply(mmail *mmA, specific_driver *baseClassA)
{
	mm = mmA;
	baseClass = (pktbase *) baseClassA;

	replyText = 0;
	qwke = ((qwkpack *) baseClass)->isQWKE();

	uplListHead = 0;

	replyExists = false;
}

qwkreply::~qwkreply()
{
	cleanup();
}

bool qwkreply::getRep1(FILE *rep, upl_qwk *l)
{
	FILE *replyFile;
	char *p, *q, *replyText;

	if (!l->qHead.init(rep))
		return false;

	mytmpnam(l->fname);

	if (!(replyFile = fopen(l->fname, "wt")))
		return false;

	replyText = new char[l->qHead.msglen + 1];
	fread(replyText, l->qHead.msglen, 1, rep);

	for (p = &replyText[l->qHead.msglen - 1]; ((*p == ' ') ||
		(*p == (char) 227)) && (p > replyText); p--);
	p[1] = '\0';

	for (p = replyText; *p; p++)
		if (*p == (char) 227)
			*p = '\n';	// PI-softcr

	// Get extended (QWKE-type) subject line, if available:

	bool anyfound;
	q = replyText;

	do {
		char *q2;
		anyfound = false;

		q2 = (char *) onecomp((unsigned char *) q,
				l->qHead.subject, "subject:");
		if (q2) {
			q = q2;
			anyfound = true;
		}

	} while (anyfound);

	l->qHead.msglen = l->msglen = p - q;	//strlen(replyText);

	fwrite(q, 1, l->msglen, replyFile);
	fclose(replyFile);
	delete[] replyText;

	return true;
}

void qwkreply::getReplies(FILE *repFile)
{
	fseek(repFile, 128, SEEK_SET);

	noOfLetters = 0;

	upl_qwk baseUplList, *currUplList = &baseUplList;

	while (!feof(repFile)) {
		currUplList->nextRecord = new upl_qwk;
		currUplList = (upl_qwk *) currUplList->nextRecord;
		if (!getRep1(repFile, currUplList)) {
			delete currUplList;
			break;
		}
		noOfLetters++;
	}
	uplListHead = baseUplList.nextRecord;
}

area_header *qwkreply::getNextArea()
{
	return new area_header(mm, 0, "REPLY", "REPLIES",
		"Letters written by you", (qwke ? "QWKE replies" :
		"QWK replies"), (COLLECTION | REPLYAREA | ACTIVE |
		PUBLIC | PRIVATE), noOfLetters, 0, 25, qwke ? 72 : 25);
}

letter_header *qwkreply::getNextLetter()
{
	static net_address nullNet;
	upl_qwk *current = (upl_qwk *) uplListCurrent;

	int area = ((qwkpack *) baseClass)->
		getXNum(current->qHead.msgnum) + 1;

	letter_header *newLetter = new letter_header(mm,
		current->qHead.subject, current->qHead.to,
		current->qHead.from, current->qHead.date, 0,
		current->qHead.refnum, currentLetter, currentLetter,
		area, current->qHead.privat, current->qHead.msglen, this,
		nullNet, mm->areaList->isLatin(area));

	currentLetter++;
	uplListCurrent = uplListCurrent->nextRecord;
	return newLetter;
}

int qwkreply::monthval(const char *abbr)
{
	static const char *month_abbr[] =
		{"Jan", "Feb", "Mar", "Apr", "Mar", "Jun",
	 	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	int c;

	for (c = 0; c < 12; c++)
		if (!strcmp(month_abbr[c], abbr))
			break;
	return c + 1;
}

void qwkreply::enterLetter(letter_header &newLetter,
				const char *newLetterFileName, int length)
{
	upl_qwk *newList = new upl_qwk;
	memset(newList, 0, sizeof(upl_qwk));

	strncpy(newList->qHead.subject, newLetter.getSubject(), 71);
	strncpy(newList->qHead.from, newLetter.getFrom(), 25);
	strncpy(newList->qHead.to, newLetter.getTo(), 25);

	newList->qHead.msgnum = atoi(mm->areaList->getShortName());
	newList->qHead.privat = newLetter.getPrivate();
	newList->qHead.refnum = newLetter.getReplyTo();
	strcpy(newList->fname, newLetterFileName);

	time_t tt = time(0);
	strftime(newList->qHead.date, 15, "%m-%d-%y %H:%M",
		localtime(&tt));

	newList->qHead.msglen = newList->msglen = length;

	addUpl(newList);
}

void qwkreply::addRep1(FILE *rep, upl_base *node, int recnum)
{
	FILE *replyFile;
	upl_qwk *l = (upl_qwk *) node;
	char *replyText, *p, *lastsp = 0;
	int chunks, length, sublen, count = 0;
	bool longsub;

	recnum = recnum;	// warning supression

	l->qHead.output(rep);

	length = l->qHead.msglen;
	sublen = strlen(l->qHead.subject);
	longsub = (sublen > 25);

	if (longsub)
		length += sublen + 11;

	chunks = (length + 127) / 128;
	if (!chunks)
		chunks = 1;

	replyText = new char[chunks * 128 + 1];
	memset(&replyText[(chunks - 1) * 128], ' ', 128);

	p = replyText;

	if (longsub)
		p += sprintf(p, "Subject: %s\n\n", l->qHead.subject);

	if ((replyFile = fopen(l->fname, "rt"))) {
		fread(p, l->qHead.msglen, 1, replyFile);
		fclose(replyFile);

		replyText[length] = 0;
		for (p = replyText; *p; p++) {
			if (*p == '\n') {
				*p = (char) 227;
				count = 0;
				lastsp = 0;
			} else {
				count++;
				if (*p == ' ')
					lastsp = p;
			}
			if ((count >= 80) && lastsp) {
				*lastsp = (char) 227;
				count = p - lastsp;
				lastsp = 0;
			}
		}
		replyText[length] = (char) 227;
	}
	fwrite(replyText, 1, chunks * 128, rep);
	delete[] replyText;
}

void qwkreply::addHeader(FILE *repFile)
{
	char tmp[256];
	sprintf(tmp, "%-128s", baseClass->getBaseName());
	fwrite(tmp, 128, 1, repFile);
}

void qwkreply::repFileName()
{
	int x;
	const char *basename = baseClass->getBaseName();

	for (x = 0; basename[x]; x++) {
		replyPacketName[x] = tolower(basename[x]);
		replyInnerName[x] = toupper(basename[x]);
	}
	strcpy(replyPacketName + x, ".rep");
	strcpy(replyInnerName + x, ".MSG");
}

const char *qwkreply::repTemplate(bool offres)
{
	static char buff[30];

	sprintf(buff, (offres && qwke) ? "%s TODOOR.EXT" : "%s",
		replyInnerName);

	return buff;
}

bool qwkreply::getOffConfig()
{
	bool status = false;

	if (qwke) {
		FILE *olc;

		upWorkList = new file_list(mm->resourceObject->get(UpWorkDir));

		if ((olc = upWorkList->ftryopen("todoor.ext", "rb"))) {
			char line[128], mode;
			int areaQWK, areaNo;

			while (!feof(olc)) {
				myfgets(line, sizeof line, olc);
				if (sscanf(line, "AREA %d %c", &areaQWK,
				    &mode) == 2) {
					areaNo = ((qwkpack *) baseClass)->
						getXNum(areaQWK) + 1;
					mm->areaList->gotoArea(areaNo);
					if (mode == 'D')
						mm->areaList->Drop();
					else
						mm->areaList->Add();
				}
			}
			fclose(olc);
			upWorkList->kill();

			status = true;
		}
		delete upWorkList;
	}
	return status;
}

bool qwkreply::makeOffConfig()
{
	FILE *todoor = 0;	// warning suppression
	net_address bogus;
	letter_header *ctrlMsg;
	const char *myname = 0, *ctrlName = 0;
	int maxareas = mm->areaList->noOfAreas();

	if (qwke) {
		todoor = fopen("TODOOR.EXT", "wb");
		if (!todoor)
			return false;
	} else {
		myname = mm->resourceObject->get(LoginName);
		ctrlName = ((qwkpack *) baseClass)->ctrlName();
	}

	for (int areaNo = 0; areaNo < maxareas; areaNo++) {
		mm->areaList->gotoArea(areaNo);
		int attrib = mm->areaList->getType();

		if (attrib & (ADDED | DROPPED)) {
			if (qwke)
				fprintf(todoor, "AREA %s %c\r\n",
					mm->areaList->getShortName(),
					(attrib & ADDED) ?
					((attrib & PERSONLY) ? 'p' :
					((attrib & PERSALL) ? 'g' : 'a'))
					: 'D');
			else {
				ctrlMsg = new letter_header(mm, (attrib &
				ADDED) ? "ADD" : "DROP", ctrlName, myname,
				"", 0, 0, 0, 0, areaNo, false, 0, this,
				bogus, false);

				enterLetter(*ctrlMsg, "", 0);

				delete ctrlMsg;

				if (attrib & ADDED)
					mm->areaList->Drop();
				else
					mm->areaList->Add();
			}
		}
	}
	if (qwke)
		fclose(todoor);
	else
		mm->areaList->refreshArea();
	return true;
}
