/*
 * MultiMail offline mail reader
 * Blue Wave class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "bw.h"
#include "compress.h"

// ---------------------------------------------------------------------------
// The Blue Wave methods
// ---------------------------------------------------------------------------

bluewave::bluewave(mmail *mmA)
{
	mm = mmA;
	ID = 0;
	bodyString = 0;
	persNdx = 0;

	findInfBaseName();

	initInf();

	ftiFile = openFile("FTI");

	initMixID();

	body = new bodytype *[maxConf];

	infile = openFile("DAT");
}

bluewave::~bluewave()
{
	delete[] bulletins;

	if (!hasPers) {
		areas--;
		mixID--;
	}
	while (maxConf--)
		delete[] body[maxConf];
	delete[] body;
	delete[] mixID;
	delete[] areas;
	delete[] mixRecord;
	delete[] bodyString;
	delete[] persNdx;

	fclose(infile);
	fclose(ftiFile);
}

area_header *bluewave::getNextArea()
{
	int x, totmsgs, numpers, flags_raw, flags_cooked;
	bool isPers = hasPers && !ID;

	x = mixID[ID];

	if (isPers)
		totmsgs = numpers = personal;
	else
		if (x != -1) {
			totmsgs = getshort(mixRecord[x].totmsgs);
			numpers = getshort(mixRecord[x].numpers);
		} else
			totmsgs = numpers = 0;

	body[ID] = new bodytype[totmsgs];

	bool inet = isPers ? false : (areas[ID].network_type ==
		INF_NET_INTERNET);

	flags_raw = getshort(areas[ID].area_flags);
	flags_cooked = isPers ? (PUBLIC | PRIVATE | COLLECTION | ACTIVE) :
		(hasOffConfig | SUBKNOWN | ((x != -1) ? ACTIVE : 0) |
		((flags_raw & INF_ALIAS_NAME) ? ALIAS : 0) |
		((flags_raw & INF_NETMAIL) ? NETMAIL : 0) |
		(inet ? (INTERNET | LATINCHAR) : 0) |
		((flags_raw & INF_ECHO) ? ECHO : 0) |
		((flags_raw & INF_NO_PUBLIC) ? 0 : PUBLIC) |
		((flags_raw & INF_NO_PRIVATE) ? 0 : PRIVATE) |
		((flags_raw & INF_PERSONAL) ? PERSONLY : 0) |
		((flags_raw & INF_TO_ALL) ? PERSALL : 0));

	area_header *tmp = new area_header(mm,
		ID + mm->driverList->getOffset(this), isPers ? "PERS" :
		(char *) areas[ID].areanum, isPers ? "PERSONAL" :
		(char *) areas[ID].echotag, (isPers ?
		"Letters addressed to you" : (char *) areas[ID].title),
		(isPers ? "Blue Wave personal" : "Blue Wave"),
		flags_cooked, totmsgs, numpers, from_to_len,
		inet ? 511 : subject_len);
	ID++;
	return tmp;
}

int bluewave::getNoOfLetters()
{
	int x = mixID[currentArea];
	bool isPers = hasPers && !currentArea;

	return isPers ? personal : ((x != -1) ?
		getshort(mixRecord[x].totmsgs) : 0);
}

letter_header *bluewave::getNextLetter()
{
	FTI_REC ftiRec;
	int areaID, letterID;

	if (currentLetter >= getNoOfLetters())
		return 0;

	if (hasPers && !currentArea) {
		areaID = persNdx[currentLetter].area;
		letterID = persNdx[currentLetter].msgnum;
	} else {
		areaID = currentArea;
		letterID = currentLetter;
	}

	fseek(ftiFile, getlong(mixRecord[mixID[areaID]].msghptr) +
		letterID * ftiStructLen, SEEK_SET);

	if (!(fread(&ftiRec, sizeof(FTI_REC), 1, ftiFile)))
		fatalError("Error reading .FTI file");

	long msglength = getlong(ftiRec.msglength);

	body[areaID][letterID].pointer = getlong(ftiRec.msgptr);
	body[areaID][letterID].msgLength = msglength;

	cropesp((char *) ftiRec.from);
	cropesp((char *) ftiRec.to);
	cropesp((char *) ftiRec.subject);

	int flags = getshort(ftiRec.flags);

	net_address na;
	na.zone = getshort(ftiRec.orig_zone);
	if (na.zone) {
		na.net = getshort(ftiRec.orig_net);
		na.node = getshort(ftiRec.orig_node);
		na.point = 0;	// set from getBody()
		na.isSet = true;
	}

	currentLetter++;

	// Y2K precaution -- the date field may not be 0-terminated:
	ftiRec.date[19] = '\0';

	return new letter_header(mm, (char *) ftiRec.subject,
		(char *) ftiRec.to, (char *) ftiRec.from, (char *) ftiRec.date,
		0, getshort(ftiRec.replyto), letterID,
		getshort(ftiRec.msgnum), areaID,
		!(!(flags & MSG_NET_PRIVATE)), msglength, this, na,
		(areas[areaID].network_type == INF_NET_INTERNET));
}

// returns the body of the requested letter in the active area
const char *bluewave::getBody(letter_header &mhead)
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

		// Skip leading space, if present:
		if (!c && (kar == ' '))
			kar = fgetc(infile);

		// Some buggy packets:
		if (!kar || (kar == EOF))
			kar = ' ';

		if (kar == '\r')
			*p++ = '\n';
		else
			if (kar != '\n')
				*p++ = kar;
	}
        do
                p--;
        while ((*p == ' ') || (*p == '\n'));    // Strip blank lines
        p[1] = '\0';

	// Extra header info embedded in the text:

	const char *s;
	char *end;

	net_address &na = mhead.getNetAddr();

	if (areas[AreaID].network_type == INF_NET_INTERNET) {

		// Get address from "From:" line:
		if (!na.isSet) {
			if ((s = getHidden("\001From: ", end))) {
				na.isInternet = true;
				na = fromAddr(s);
				if (end)
					*end = '\n';
			}
		}

		// Get Message-ID/References:
		if (!mhead.getMsgID()) {
			if ((s = getHidden("\001Message-ID: ", end))) {
				const char *s2;
				char *end2, *final;

				if (end)
					*end = '\n';
				s2 = getHidden("\001References: ", end2);
				if (end)
					*end = '\0';

				if (s2) {
					final = new char[strlen(s) +
						strlen(s2) + 2];
					sprintf(final, "%s %s", s2, s);
					if (end2)
						*end2 = '\n';
				} else
					final = (char *) s;

				mhead.changeMsgID(final);
				if (s2)
					delete[] final;
				if (end)
					*end = '\n';
			}
		}

		// Get Newsgroups:
		if (!mhead.getNewsgrps()) {
			if ((s = getHidden("\001Newsgroups: ", end))) {
				mhead.changeNewsgrps(s);
				if (end)
					*end = '\n';
			}
		}

		// Get Followup-To:
		if (!mhead.getFollow()) {
			if ((s = getHidden("\001Followup-To: ", end))) {
				mhead.changeFollow(s);
				if (end)
					*end = '\n';
			}
		}

		// Get extended subject:
		if ((s = getHidden("\001Subject: ", end))) {
			mhead.changeSubject(s);
			if (end)
				*end = '\n';
		}

		// Replace the date (there's more info in the standard
		// Usenet date header; plus, it differs from the value
		// in the Blue Wave header in the packets I have):
		if ((s = getHidden("\001Date: ", end))) {
			mhead.changeDate(s);
			if (end)
				*end = '\n';
		}

	} else {

		// Add point to netmail address, if possible/necessary:
		if (na.isSet)
			if (!na.point) {
				s = strstr(bodyString, "\001FMPT");
				if (s)
					sscanf(s, "\001FMPT%d\n", &na.point);
			}

		// Get MSGID:
		if (!mhead.getMsgID())
			if ((s = getHidden("\001MSGID: ", end))) {
				mhead.changeMsgID(s);
				*end = '\n';
			}

		// Change to Latin character set, if necessary:
		checkLatin(mhead);
	}

	return bodyString;
}

void bluewave::findInfBaseName()
{
	const char *q = mm->workList->exists(".inf");
	int len = strlen(q) - 4;
	strncpy(packetBaseName, q, len);
	packetBaseName[len] = '\0';
}

// Read .INF file
void bluewave::initInf()
{
	INF_HEADER infoHeader;

	FILE *infFile = openFile("INF");

	// Header

	if (!fread(&infoHeader, sizeof(INF_HEADER), 1, infFile))
		fatalError("Error reading .INF file");

	infoHeaderLen = getshort(infoHeader.inf_header_len);
	if (infoHeaderLen < ORIGINAL_INF_HEADER_LEN)
		infoHeaderLen = ORIGINAL_INF_HEADER_LEN;

	infoAreainfoLen = getshort(infoHeader.inf_areainfo_len);
	if (infoAreainfoLen < ORIGINAL_INF_AREA_LEN)
		infoAreainfoLen = ORIGINAL_INF_AREA_LEN;

	from_to_len = infoHeader.from_to_len;
	if (!from_to_len || (from_to_len > 35))
		from_to_len = 35;

	subject_len = infoHeader.subject_len;
	if (!subject_len || (subject_len > 71))
		subject_len = 71;

	mixStructLen = getshort(infoHeader.mix_structlen);
	if (mixStructLen < ORIGINAL_MIX_STRUCT_LEN)
		mixStructLen = ORIGINAL_MIX_STRUCT_LEN;

	ftiStructLen = getshort(infoHeader.fti_structlen);
	if (ftiStructLen < ORIGINAL_FTI_STRUCT_LEN)
		ftiStructLen = ORIGINAL_FTI_STRUCT_LEN;

	hasOffConfig = (getshort(infoHeader.ctrl_flags) & INF_NO_CONFIG) ?
		0 : OFFCONFIG;

	mm->resourceObject->set(LoginName, (char *) infoHeader.loginname);
	mm->resourceObject->set(AliasName, (char *) infoHeader.aliasname);
	mm->resourceObject->set(SysOpName, (char *) infoHeader.sysop);
	mm->resourceObject->set(BBSName, (char *) infoHeader.systemname); 

	// Areas

	maxConf = (mm->workList->getSize() - infoHeaderLen) /
		infoAreainfoLen + 1;

	areas = new INF_AREA_INFO[maxConf];
	mixID = new int[maxConf];

	for (int d = 1; d < maxConf; d++) {
		fseek(infFile, (infoHeaderLen + (d - 1) * infoAreainfoLen),
			SEEK_SET);
		if (!fread(&areas[d], sizeof(INF_AREA_INFO), 1, infFile))
			fatalError("Premature EOF in bluewave::initInf");
		mixID[d] = -1;
	}

	fclose(infFile);

	// Bulletins, etc.

	listBulletins((const char (*)[13]) infoHeader.readerfiles, 5);
}

// Read .MIX file, match .INF records to .MIX records
void bluewave::initMixID()
{
	// Read .MIX file

	FILE *mixFile = openFile("MIX");

	noOfMixRecs = (int) (mm->workList->getSize() / mixStructLen);
	mixRecord = new MIX_REC[noOfMixRecs];

	fread(mixRecord, mixStructLen, noOfMixRecs, mixFile);
	fclose(mixFile);

	// Match records

	int mixIndex = 0, c, d;
	personal = 0;

	for (c = 0; c < noOfMixRecs; c++) {
		for (d = mixIndex + 1; d < maxConf; d++)
			if (!strncasecmp((char *) areas[d].areanum,
			(char *) mixRecord[c].areanum, 6)) {
				mixID[d] = c;
				mixIndex = d;
				break;
			}
		personal += getshort(mixRecord[c].numpers);
	}

	bool checkpers = mm->resourceObject->getInt(BuildPersArea);
	hasPers = checkpers && personal;
	if (hasPers) {
		persNdx = new perstype[personal];
		personal = 0;

		FTI_REC ftiRec;

		const char *name = mm->resourceObject->get(LoginName);
		const char *alias = mm->resourceObject->get(AliasName);

		for (c = 1; c < maxConf; c++)
		    if ((mixID[c] != -1) &&
			getshort(mixRecord[mixID[c]].numpers)) {
			    int totmsgs = getshort(mixRecord[mixID[c]].totmsgs);

			    for (d = 0; d < totmsgs; d++) {
				fseek(ftiFile,
				    getlong(mixRecord[mixID[c]].msghptr) +
				    d * ftiStructLen, SEEK_SET);

				if (!(fread(&ftiRec, sizeof(FTI_REC),
				    1, ftiFile)))
					fatalError("Error reading .FTI file");

				cropesp((char *) ftiRec.to);
				if (!strcasecmp((char *) ftiRec.to, name) ||
				    !strcasecmp((char *) ftiRec.to, alias)) {
					persNdx[personal].area = c;
					persNdx[personal++].msgnum = d;
				}
			    }
		    }
	} else {
		areas++;
		mixID++;
		maxConf--;
	}
}

FILE *bluewave::openFile(const char *extent)
{
	FILE *tmp;
	char fname[13];

	sprintf(fname, "%s.%s", packetBaseName, extent);

	if (!(tmp = mm->workList->ftryopen(fname, "rb"))) {
		sprintf(fname, "Could not open .%s file", extent);
		fatalError(fname);
	}
	return tmp;
}

// Write out an .XTI file
const char *bluewave::saveOldFlags()
{
	FILE *xtiFile;
	static char xtiFileN[13];

	if (mychdir(mm->resourceObject->get(WorkDir)))
		fatalError("Unable to change to work directory");

	sprintf(xtiFileN, "%s.xti", packetBaseName);
	xtiFile = fopen(xtiFileN, "wb");

	area_list *al = mm->areaList;
	int maxareas = al->noOfAreas();
	for (int c = 0; c < maxareas; c++) {
		al->gotoArea(c);
		if (!al->isCollection()) {
			al->getLetterList();
			letter_list *ll = mm->letterList;

			for (int d = 0; d < ll->noOfLetter(); d++) {
			    ll->gotoLetter(d);
			    int stat = ll->getStatus();

			    XTI_REC xtiRec;

			    xtiRec.flags =
				((stat & MS_READ) ? XTI_HAS_READ : 0) |
				((stat & MS_REPLIED) ? XTI_HAS_REPLIED : 0) |
				((stat & MS_PERSTO) ? XTI_IS_PERSONAL : 0);

			    xtiRec.marks =
				(stat & MS_MARKED) ? XTI_MARK_SAVE : 0;

			    fwrite(&xtiRec, sizeof xtiRec, 1, xtiFile);
			}
			delete ll;
		}
	}
	fclose(xtiFile);

	return xtiFileN;
}

// ---------------------------------------------------------------------------
// The Blue Wave reply methods
// ---------------------------------------------------------------------------

bwreply::upl_bw::upl_bw()
{
	msgid = newsgrps = extsubj = 0;
}

bwreply::upl_bw::~upl_bw()
{
	delete[] msgid;
	delete[] newsgrps;
	delete[] extsubj;
}

bwreply::bwreply(mmail *mmA, specific_driver *baseClassA)
{
	mm = mmA;
	baseClass = (pktbase *) baseClassA;

	uplHeader = new UPL_HEADER;
	uplListHead = 0;
	replyText = 0;

	replyExists = false;
}

bwreply::~bwreply()
{
	cleanup();
	delete uplHeader;
}

bool bwreply::getRep1(FILE *uplFile, upl_bw *l, int recnum)
{
	FILE *orgfile, *destfile;
	const char *orgname;
	int c, count = 0;

	fseek(uplFile, getshort(uplHeader->upl_header_len) + recnum *
		getshort(uplHeader->upl_rec_len), SEEK_SET);
	if (fread(&(l->uplRec), sizeof(UPL_REC), 1, uplFile) != 1)
		return false;

	orgname = upWorkList->exists((char *) l->uplRec.filename);
	mytmpnam(l->fname);

	l->msgid = l->newsgrps = 0;

	if ((orgfile = fopen(orgname, "rb"))) {
	    if ((destfile = fopen(l->fname, "wt"))) {
		while ((c = fgetc(orgfile)) != EOF) {
			if (c == '\001') {
				c = fgetc(orgfile);
				int x;
				bool isRef = (c == 'R'),
					isNews = (c == 'N'),
					isSubj = (c == 'S');

				if (isRef || isNews || isSubj) {
					for (x = 0; x < (isSubj ? 8 : 11); x++)
						fgetc(orgfile);
				}
				x = 0;
				while ((c != EOF) && (c != '\n')) {
					c = fgetc(orgfile);
					x++;
				}
				if (isRef || isNews || isSubj) {
					char *tmp = new char[x];
					fseek(orgfile, x * -1, SEEK_CUR);
					fread(tmp, 1, x, orgfile);
					strtok(tmp, "\r");
					if (isRef)
						l->msgid = tmp;
					else
						if (isNews)
							l->newsgrps = tmp;
						else
							l->extsubj = tmp;
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
	    fclose(orgfile);
	}

	l->msglen = count;

	remove(orgname);

	return true;
}

void bwreply::getReplies(FILE *uplFile)
{
	if (!fread(uplHeader, sizeof(UPL_HEADER), 1, uplFile))
		fatalError("Error reading UPL file");

	noOfLetters = (upWorkList->getSize() -
		getshort(uplHeader->upl_header_len)) /
		getshort(uplHeader->upl_rec_len);

	upl_bw baseUplList, *currUplList = &baseUplList;

	for (int c = 0; c < noOfLetters; c++) {
		currUplList->nextRecord = new upl_bw;
		currUplList = (upl_bw *) currUplList->nextRecord;
		if (!getRep1(uplFile, currUplList, c)) {
			delete currUplList;
			break;
		}
	}
	uplListHead = baseUplList.nextRecord;
}

area_header *bwreply::getNextArea()
{
	return new area_header(mm, 0, "REPLY", "REPLIES",
		"Letters written by you", "Blue Wave replies",
		(COLLECTION | REPLYAREA | ACTIVE | PUBLIC | PRIVATE),
		noOfLetters, 0, 35, 71);
}


int bwreply::getAreaFromTag(const char *tag)
{
	int areaNo = 0;
	for (int c = 0; c < mm->areaList->noOfAreas(); c++)
		if (!strcmp(mm->areaList->getName(c), tag)) {
			areaNo = c;
			break;
		}

	return areaNo;
}

letter_header *bwreply::getNextLetter()
{
	upl_bw *current = (upl_bw *) uplListCurrent;
	time_t unixTime = (time_t) getlong(current->uplRec.unix_date);

	char date[30];
	strftime(date, 30, "%b %d %Y  %H:%M", localtime(&unixTime));

	int areaNo = getAreaFromTag((char *) current->uplRec.echotag);

	const char *msgid;
	net_address na;
	bool isInet = (current->uplRec.network_type ==
		INF_NET_INTERNET);
	if (isInet) {
		na.isInternet = true;
		na = (char *) current->uplRec.net_dest;
		msgid = current->msgid;
	} else {
		na.zone = getshort(current->uplRec.destzone);
		if (na.zone) {
			na.net = getshort(current->uplRec.destnet);
			na.node = getshort(current->uplRec.destnode);
			na.point = getshort(current->uplRec.destpoint);
			na.isSet = true;
		}

		msgid = (char *) current->uplRec.net_dest;
		if (!strncasecmp(msgid, "REPLY: ", 7))
			msgid += 7;
		else
			msgid = 0;
	}

	letter_header *newLetter = new letter_header(mm, current->extsubj ?
		current->extsubj : (char *) current->uplRec.subj,
		(char *) current->uplRec.to,
		(char *) current->uplRec.from, date, msgid,
		getlong(current->uplRec.replyto),
		currentLetter, currentLetter, areaNo,
		!(!(getshort(current->uplRec.msg_attr) & UPL_PRIVATE)),
		current->msglen, this, na, isInet, current->newsgrps);

	currentLetter++;
	uplListCurrent = uplListCurrent->nextRecord;
	return newLetter;
}

void bwreply::enterLetter(letter_header &newLetter,
				const char *newLetterFileName, int length)
{
	upl_bw *newList = new upl_bw;
	memset(newList, 0, sizeof(upl_bw));

	int msg_attr = 0;

	// fill the fields of UPL_REC
	strncpy((char *) newList->uplRec.from, newLetter.getFrom(), 35);
	strncpy((char *) newList->uplRec.to, newLetter.getTo(), 35);
	strncpy((char *) newList->uplRec.subj, newLetter.getSubject(), 71);
	strcpy((char *) newList->uplRec.echotag, mm->areaList->getName());

	bool usenet = mm->areaList->isUsenet();
	bool internet = mm->areaList->isInternet();

	if (internet || usenet)
		newList->uplRec.network_type = INF_NET_INTERNET;

	const char *msgid = newLetter.getMsgID();
	if (msgid)
		if (!(internet || usenet))
			sprintf((char *) newList->uplRec.net_dest,
				"REPLY: %.92s", msgid);
		else
			if (usenet)
				newList->msgid = strdupplus(msgid);

	if (usenet) {
		newList->newsgrps = strdupplus(newLetter.getNewsgrps());
		if (strlen(newLetter.getSubject()) > 71)
			newList->extsubj = strdupplus(newLetter.getSubject());
	}

	strcpy(newList->fname, newLetterFileName);
	putlong(newList->uplRec.unix_date, (long) time(0));

	net_address &na = newLetter.getNetAddr();
	if (na.isSet) {
		msg_attr |= UPL_NETMAIL;
		if (na.isInternet)
			sprintf((char *) newList->uplRec.net_dest,
				"%.99s", (const char *) na);
		else {
			putshort(newList->uplRec.destzone, na.zone);
			putshort(newList->uplRec.destnet, na.net);
			putshort(newList->uplRec.destnode, na.node);
			putshort(newList->uplRec.destpoint, na.point);
		}
	}

	putlong(newList->uplRec.replyto,
		(unsigned long) newLetter.getReplyTo());

	if (newLetter.getPrivate())
		msg_attr |= UPL_PRIVATE;
	if (newLetter.getReplyTo())
		msg_attr |= UPL_IS_REPLY;
	putshort(newList->uplRec.msg_attr, msg_attr);

	newList->msglen = length;

	addUpl(newList);
}

void bwreply::addRep1(FILE *uplFile, upl_base *node, int recnum)
{
	FILE *orgfile, *destfile;
	upl_bw *l = (upl_bw *) node;
	const char *dest;
	int c;

	recnum = recnum;	// warning supression

	dest = freeFileName();
	strcpy((char *) l->uplRec.filename, dest);

	if ((orgfile = fopen(l->fname, "rt"))) {
		if ((destfile = fopen(dest, "wb"))) {
			if (l->uplRec.network_type == INF_NET_INTERNET) {
				fprintf(destfile, *(l->uplRec.net_dest) ?
					"\001X-Mail" : "\001X-News");
				fprintf(destfile, "reader: " MM_NAME
					" Offline Reader for %s v%d.%d\r\n",
					sysname(), MM_MAJOR, MM_MINOR);

				if (l->msgid)
					fprintf(destfile,
						"\001References: %s\r\n",
							l->msgid);
				if (l->newsgrps)
					fprintf(destfile,
						"\001Newsgroups: %s\r\n",
							l->newsgrps);
				if (l->extsubj)
					fprintf(destfile,
						"\001Subject: %s\r\n",
							l->extsubj);
			}

			while ((c = fgetc(orgfile)) != EOF) {
				if (c == '\n')
					fputc('\r', destfile);
				fputc(c, destfile);
			}
			fclose(destfile);
		}
		fclose(orgfile);
	}
	fwrite(&(l->uplRec), sizeof(UPL_REC), 1, uplFile);
}

void bwreply::addHeader(FILE *uplFile)
{
	UPL_HEADER newUplHeader;
	memset(&newUplHeader, 0, sizeof(UPL_HEADER));

	putshort(newUplHeader.upl_header_len, sizeof(UPL_HEADER));
	putshort(newUplHeader.upl_rec_len, sizeof(UPL_REC));

	newUplHeader.reader_major = MM_MAJOR;
	newUplHeader.reader_minor = MM_MINOR;
	sprintf((char *) newUplHeader.vernum, "%d.%d", MM_MAJOR, MM_MINOR);
	for (int c = 0; newUplHeader.vernum[c]; newUplHeader.vernum[c++] -= 10);

	int tearlen = sprintf((char *) newUplHeader.reader_name,
		MM_NAME "/%s", sysname());
	strncpy((char *) newUplHeader.reader_tear, ((tearlen < 17) ?
		(char *) newUplHeader.reader_name : MM_NAME), 16);

	strcpy((char *) newUplHeader.loginname,
		mm->resourceObject->get(LoginName));
	strcpy((char *) newUplHeader.aliasname,
		mm->resourceObject->get(AliasName));

	fwrite(&newUplHeader, sizeof(UPL_HEADER), 1, uplFile);
}

const char *bwreply::freeFileName()
{
	static char testFileName[13];

	for (long c = 0; c <= 99999; c++) {
		sprintf(testFileName, "%05ld.MSG", c);
		if (!readable(testFileName))
			break;
	}
	return testFileName;
}

void bwreply::repFileName()
{
	int x;
	const char *basename = baseClass->getBaseName();

	for (x = 0; basename[x]; x++) {
		replyPacketName[x] = tolower(basename[x]);
		replyInnerName[x] = toupper(basename[x]);
	}
	strcpy(replyPacketName + x, ".new");
	strcpy(replyInnerName + x, ".UPL");
}

const char *bwreply::repTemplate(bool offres)
{
	static char buff[30];

	if (offres) {
		const char *basename = findBaseName(replyInnerName);
		sprintf(buff, "%s *.MSG %s.OLC", replyInnerName, basename);
	} else
		sprintf(buff, "%s *.MSG", replyInnerName);

	return buff;
}

char *bwreply::nextLine(FILE *olc)
{
	static char line[128];

	char *end = myfgets(line, sizeof line, olc);
	if (end)
		while ((*end == '\n') || (*end == '\r'))
			*end-- = '\0';
	return line;
}

bool bwreply::getOffConfig()
{
	FILE *olc;
	char *line;
	bool status;

	upWorkList = new file_list(mm->resourceObject->get(UpWorkDir));

	if ((olc = upWorkList->ftryopen(".olc", "rb"))) {
		nextLine(olc);
		do
			line = nextLine(olc);
		while (line[0] != '[');

		int areaNo = -1;
		int maxareas = mm->areaList->noOfAreas();

		do {
			line[strlen(line) - 1] = '\0';

			for (int c = areaNo + 1; c < maxareas; c++) {
			    mm->areaList->gotoArea(c);
			    if (!strcmp(mm->areaList->getName(), line + 1)) {
				mm->areaList->Add();
				areaNo = c;
				break;
			    } else
				mm->areaList->Drop();
			}

			nextLine(olc);
			nextLine(olc);
			line = nextLine(olc);
		} while (!feof(olc));

		fclose(olc);
		upWorkList->kill();

		while (++areaNo < maxareas) {
			mm->areaList->gotoArea(areaNo);
			mm->areaList->Drop();
		}
		status = true;
	} else
		status = false;

	delete upWorkList;
	return status;
}

bool bwreply::makeOffConfig()
{
	FILE *olc;
	char fname[13];

	sprintf(fname, "%s.OLC", findBaseName(replyInnerName));

	olc = fopen(fname, "wb");
	if (!olc)
		return false;

	fprintf(olc, "[Global Mail Host Configuration]\r\n"
		"AreaChanges = ON\r\n\r\n");

	int maxareas = mm->areaList->noOfAreas();
	for (int x = 0; x < maxareas; x++) {
		mm->areaList->gotoArea(x);
		int attrib = mm->areaList->getType();
		if (!(attrib & COLLECTION) && (((attrib & ACTIVE)
		    && !(attrib & DROPPED)) || (attrib & ADDED)))
			fprintf(olc, "[%s]\r\nScan = %s\r\n\r\n",
			    mm->areaList->getName(), (attrib &
				PERSONLY) ? "PERSONLY" : ((attrib &
				    PERSALL) ? "PERSALL" : "ALL"));
	}
	fclose(olc);

	return true;
}
