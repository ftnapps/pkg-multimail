/*
 * MultiMail offline mail reader
 * SOUP

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "soup.h"
#include "compress.h"

// ---------------------------------------------------------------------------
// The sheader methods
// ---------------------------------------------------------------------------

const char *sheader::compare[items] =
{
	"Date:", "From:", "To:", "Reply-To:", "Subject:", "Newsgroups:",
	"Followup-To:", "References:", "Message-ID:"
};

sheader::sheader()
{
	for (int c = 0; c < items; c++)
		values[c] = 0;
}

sheader::~sheader()
{
	for (int c = 0; c < items; c++)
		delete[] values[c];
}

// Read and parse header; assumes msg is already at start of text
bool sheader::init(FILE *msg)
{
	char buffer[1000], *end;
	int lastc = -1;

	do {
		if (feof(msg))
			return false;

		// Get first 999 bytes of header line
		fgets(buffer, sizeof buffer, msg);
		end = buffer + strlen(buffer) - 1;

		// If there's more, rewind to last space; remainder
		// will be handled as a continuation
		if (*end != '\n') {
			long skip = -1;
			while ((*end != ' ') && (end > (buffer + 1))) {
				end--;
				skip--;
			}
			if (end > (buffer + 1)) {
				fseek(msg, skip, SEEK_CUR);
				*end = '\0';
			} else		// give up on getting the rest
				while (fgetc(msg) != '\n');
		}

		// If the line isn't blank...
		if (buffer != end) {

			if (*end == '\n')
				*end = '\0';

			char *sp = strpbrk(buffer, " \t");
	// continuation
			if ((sp == buffer) && (lastc != -1)) {
			    while ((*sp == ' ') || (*sp == '\t'))
				sp++;
			    char *newval = new char[strlen(values[lastc]) +
				strlen(sp) + 2];
			    sprintf(newval, "%s %s", values[lastc], sp);
			    delete[] values[lastc];
			    values[lastc] = newval;
			} else
	// new header
			    if (sp) {
				*sp = '\0';
				int c;

				for (c = 0; c < items; c++)
				    if (!values[c] &&
					!strcasecmp(buffer, compare[c])) {
					    do
						sp++;
					    while ((*sp == ' ') ||
						(*sp == '\t'));
					    values[c] = strdupplus(sp);
					    lastc = c;
					    break;
					}
				if (c == items)
				    lastc = -1;
			    }
		}
	} while (buffer != end);	// End of header == blank line

	// If the last msgid is truncated, drop it

	if (values[refs]) {
		end = strrchr(values[refs], '<');
		if (!strchr(end, '>')) {
			if (end[-1] == ' ')
				end--;
			*end = '\0';
		}
	}

	return true;
}

// Set header values from strings
bool sheader::init(const char *fromA, const char *toName,
	const char *toAddr, const char *subjectA, const char *newsA,
	const char *refsA, long length)
{
	values[from] = strdupplus(fromA);
	values[subject] = strdupplus(subjectA);
	values[newsgrps] = strdupplus(newsA);
	values[refs] = strdupplus(refsA);

	if ((toName && toAddr) && strcmp(toName, "All")) {
		if (*toName && strcmp(toName, toAddr)) {
			values[to] = new char[strlen(toName) +
				strlen(toAddr) + 6];
			sprintf(values[to], quoteIt(toName) ?
				"\"%s\" <%s>" : "%s <%s>", toName, toAddr);
		} else
			values[to] = strdupplus(toAddr);
	}

	char dateA[40];
	time_t t;

	time(&t);
	strftime(dateA, sizeof dateA, "%a, %d %b %Y %X GMT", gmtime(&t));

	values[date] = strdupplus(dateA);

	msglen = length;

	return true;
}

// Write out the header in 'b' or 'B' form
void sheader::output(FILE *msg)
{
	char readername[80];
	sprintf(readername, "User-Agent: " MM_NAME "/%d.%d (SOUP; %s)",
		MM_MAJOR, MM_MINOR, sysname());

	long outlen = msglen + strlen(readername) + 2;

	for (int c = 0; c <= refs; c++)
		if (values[c] && values[c][0])
			outlen += strlen(compare[c]) + strlen(values[c]) + 2;

	unsigned char outlenA[4];
	putblong(outlenA, outlen);
	fwrite(outlenA, 4, 1, msg);

	for (int c = 0; c <= refs; c++)
		if (values[c] && values[c][0])
			fprintf(msg, "%s %s\n", compare[c], values[c]);

	fprintf(msg, "%s\n\n", readername);
}

const char *sheader::From()
{
	return values[from] ? values[from] : "";
}

const char *sheader::Subject()
{
	return values[subject] ? values[subject] : "";
}

const char *sheader::Date()
{
	return values[date] ? values[date] : "";
}

const char *sheader::ReplyTo()
{
	return values[reply];
}

const char *sheader::To()
{
	return values[to];
}

const char *sheader::Newsgrps()
{
	return values[newsgrps];
}

const char *sheader::Follow()
{
	return values[follow];
}

const char *sheader::Msgid()
{
	return values[msgid];
}

const char *sheader::Refs()
{
	return values[refs];
}

// ---------------------------------------------------------------------------
// The SOUP methods
// ---------------------------------------------------------------------------

soup::soup(mmail *mmA)
{
	mm = mmA;
	ID = 0;
	bodyString = 0;

	infile = 0;

	strncpy(packetBaseName, findBaseName(mm->resourceObject->
		get(PacketName)), 8);
	packetBaseName[8] = '\0';

	hasOffConfig = false; // :-( For now, at least.

	readAreas();
	buildIndices();

	const char x[1][13] = {"info"};
	listBulletins(x, 1, 0);

	hasPers = false;
}

soup::~soup()
{
	while (maxConf--) {
		delete[] body[maxConf];
		delete[] areas[maxConf]->name;
		delete areas[maxConf];
	}
	delete[] body;
	delete[] areas;
	delete[] bodyString;

	if (infile)
		fclose(infile);
}

bool soup::msgopen(int area)
{
	static int oldArea;

	if (!infile)
		oldArea = -1;	// Reset (new packet)

	if (area != oldArea) {
		if (oldArea != -1)
			fclose(infile);

		oldArea = area;

		const char *fname = areas[area]->msgfile;
		if (!(*fname))
			return false;
		if (!(infile = mm->workList->ftryopen(fname, "rb")))
			fatalError("Could not open .MSG file");
	}
	return true;
}

file_header *soup::getFileList()
{
	return 0;
}

area_header *soup::getNextArea()
{
	int cMsgNum = areas[ID]->nummsgs;

	area_header *tmp = new area_header(mm,
		ID + mm->driverList->getOffset(this), areas[ID]->numA,
		areas[ID]->msgfile, areas[ID]->name, "SOUP",
		areas[ID]->attr | (cMsgNum ? ACTIVE : 0), cMsgNum,
		0, 99, 511);
	ID++;
	return tmp;
}

int soup::getNoOfLetters()
{
	return areas[currentArea]->nummsgs;
}

// Build indices from *.MSG -- does not handle type 'M', nor *.IDX
void soup::buildIndices()
{
	int x, cMsgNum;

	numMsgs = 0;

	body = new bodytype *[maxConf];

	ndx_fake base, *oldndx, *tmpndx;

	for (x = 0; x < maxConf; x++) {
	    body[x] = 0;
	    cMsgNum = 0;

	    if (msgopen(x)) {

		tmpndx = &base;

		switch (areas[x]->mode) {
		case 'B':
		case 'b':
		case 'u':
			long offset, counter;

			while (!feof(infile)) {
			    if (toupper(areas[x]->mode) == 'B') {
				unsigned char offsetA[4];

				if (fread(offsetA, 1, 4, infile) == 4)
				    offset = getblong(offsetA);
				else
				    offset = -1;
			    } else {
				char buffer[128];

				if (myfgets(buffer, sizeof buffer, infile))
				    sscanf(buffer, "#! rnews %ld", &offset);
				else
				    offset = -1;
			    }
			    if (offset != -1) {
				counter = ftell(infile);
				fseek(infile, offset, SEEK_CUR);

				tmpndx->next = new ndx_fake;
				tmpndx = tmpndx->next;

				tmpndx->pointer = counter;
				tmpndx->length = offset;

				numMsgs++;
				cMsgNum++;
			    }
			}
			break;
		case 'm':
			char buffer[128];
			long c, lastc = -1;

			while (!feof(infile)) {
			    c = ftell(infile);
			    if (myfgets(buffer, sizeof buffer, infile))
				if (!strncmp(buffer, "From ", 5)) {
				    if (lastc != -1) {
					tmpndx->next = new ndx_fake;
					tmpndx = tmpndx->next;

					tmpndx->pointer = lastc;
					tmpndx->length = c - lastc;

					numMsgs++;
					cMsgNum++;
				    }
				    lastc = c;
				}
			}
			if (lastc != -1) {
				tmpndx->next = new ndx_fake;
				tmpndx = tmpndx->next;

				tmpndx->pointer = lastc;
				tmpndx->length = ftell(infile) - lastc;

				numMsgs++;
				cMsgNum++;
			}
		}
	    }
	    areas[x]->nummsgs = cMsgNum;

	    if (cMsgNum) {
		body[x] = new bodytype[cMsgNum];

		tmpndx = base.next;
		for (int y = 0; y < cMsgNum; y++) {
			body[x][y].pointer = tmpndx->pointer;
			body[x][y].msgLength = tmpndx->length;
			oldndx = tmpndx;
			tmpndx = tmpndx->next;
			delete oldndx;
		}
	    }
	}
}

letter_header *soup::getNextLetter()
{
	sheader sHead;

	long len = body[currentArea][currentLetter].msgLength;

	// Read in header:

	if (msgopen(currentArea)) {
		fseek(infile, body[currentArea][currentLetter].pointer,
			SEEK_SET);
		sHead.init(infile);
	}

	// Get address from "From:" line:

	net_address na;
	na = fromAddr(sHead.From());

	// Get name from "From:" line:

	const char *fr = fromName(sHead.From());

	// Join Message-ID and References lines:

	const char *refs = sHead.Refs(), *msgid = sHead.Msgid();
	char *fullref = 0;

	if (refs) {
		if (msgid) {
			fullref = new char[strlen(msgid) + strlen(refs) + 2];
			sprintf(fullref, "%s %s", refs, msgid);
			msgid = fullref;
		} else
			msgid = refs;
	}

	letter_header *tmp = new letter_header(mm, sHead.Subject(),
		sHead.To() ? sHead.To() : "All", *fr ? fr : na,
		sHead.Date(), msgid, 0, currentLetter, currentLetter + 1,
		currentArea, false, len, this, na, true, sHead.Newsgrps(),
		sHead.Follow(), sHead.ReplyTo());

	currentLetter++;

	delete[] fullref;

	return tmp;
}

// returns the body of the requested letter
const char *soup::getBody(letter_header &mhead)
{
	// Load the message to a temporary area, then copy the header
	// with hidden line markers. Uses twice the memory, but what
	// the hell.

	int AreaID, LetterID;

	AreaID = mhead.getAreaID() - mm->driverList->getOffset(this);
	LetterID = mhead.getLetterID();

	long len = body[AreaID][LetterID].msgLength;
	char *tmp = new char[len + 1];

	msgopen(AreaID);

	fseek(infile, body[AreaID][LetterID].pointer, SEEK_SET);
	fread(tmp, 1, len, infile);

	tmp[len] = '\0';

	// Find number of header lines:

	char *c;
	int headlines = 0;
	for (c = tmp; !((c[0] == '\n') && (c[1] == '\n')); c++)
		if (c[1] == '\n')
			headlines++;

	// Allocate space for ^A characters:

	delete[] bodyString;
	bodyString = new char[len + headlines + 1];

	// Copy header, hiding it:

	char *dest = bodyString, *source = tmp;
	do {
		*dest++ = 1;
		do
			*dest++ = *source++;
		while (source[-1] != '\n');
	} while (source[0] != '\n');

	// Copy body:

	memcpy(dest, source, len - (c - tmp));
	delete[] tmp;

	// Strip blank lines from end:

	dest += len - (c - tmp) - 1;
	do
		dest--;
	while ((*dest == ' ') || (*dest == '\n'));
	dest[1] = '\0';

	return bodyString;
}

// Area and packet init
void soup::readAreas()
{
	file_list *wl = mm->workList;

	// Info not available in SOUP:

	char *tmp;
	const char *defName = mm->resourceObject->get(UserName);
	const char *defAddr = mm->resourceObject->get(InetAddr);

	if (defAddr) {
		if (defName && *defName && strcmp(defName, defAddr)) {
			tmp = new char[strlen(defName) +
				strlen(defAddr) + 6];
			sprintf(tmp, quoteIt(defName) ?
				"\"%s\" <%s>" : "%s <%s>", defName, defAddr);
		} else
			tmp = strdupplus(defAddr);
	} else
		tmp = "";

	mm->resourceObject->set(LoginName, tmp);
	mm->resourceObject->set(AliasName, "");
	mm->resourceObject->set(BBSName, 0);
	mm->resourceObject->set(SysOpName, 0);

	// AREAS:

	maxConf = 1;

	AREAs base, *tmparea;

	// Email area is always present:

	base.next = new AREAs;
	tmparea = base.next;
	memset(tmparea, 0, sizeof(AREAs));
	tmparea->name = strdupplus("Email");
	tmparea->attr = ACTIVE | LATINCHAR | INTERNET | NETMAIL | PRIVATE;
	strcpy(tmparea->numA, "0");

	FILE *afile = wl->ftryopen("areas", "rb");
	if (afile) {
		char buffer[128], *msgfile, *name, *rawattr;
		do
		    if (myfgets(buffer, sizeof buffer, afile)) {

			msgfile = strtok(buffer, "\t");
			name = strtok(0, "\t");
			rawattr = strtok(0, "\t");

			// If the Email area has not been set yet, the
			// first email area in the packet (in any) becomes
			// it.

			if (!base.next->msgfile[0] && ((rawattr[2] == 'm') ||
			   ((rawattr[2] != 'n') && !((*rawattr == 'u') ||
			     (*rawattr == 'B'))))) {
				strncpy(base.next->msgfile, msgfile, 9);

				delete[] base.next->name;
				base.next->name = strdupplus(name);
				base.next->mode = *rawattr;
			} else {
				tmparea->next = new AREAs;
				tmparea = tmparea->next;
				memset(tmparea, 0, sizeof(AREAs));

				strncpy(tmparea->msgfile, msgfile, 9);
				tmparea->name = strdupplus(name);

				tmparea->mode = *rawattr;

				tmparea->attr = ACTIVE | LATINCHAR |
					INTERNET | (((rawattr[2] == 'n') ||
					((rawattr[2] != 'm') && ((*rawattr
					== 'B') || (*rawattr == 'u')))) ?
					PUBLIC : (NETMAIL | PRIVATE));

				sprintf(tmparea->numA, "%5d", maxConf++);
			}
		    }
		while (!feof(afile));
		fclose(afile);

		areas = new AREAs *[maxConf];
		tmparea = base.next;
		for (int i = 0; i < maxConf; i++) {
			areas[i] = tmparea;
			tmparea = tmparea->next;
		}
	} else
		areas = 0;
}

bool soup::isLatin()
{
	return true;
}

// ---------------------------------------------------------------------------
// The SOUP reply methods
// ---------------------------------------------------------------------------

souprep::souprep(mmail *mmA, specific_driver *baseClassA)
{
	mm = mmA;
	baseClass = (pktbase *) baseClassA;

	replyText = 0;
	uplListHead = 0;

	replyExists = false;
}

souprep::~souprep()
{
	cleanup();
}

// convert one reply to MultiMail's internal format
bool souprep::getRep1(FILE *rep, upl_soup *l)
{
	FILE *orgfile, *destfile;
	char buffer[128], *msgfile, *mnflag, *rawattr;
	long count = 0;

	if (!myfgets(buffer, sizeof buffer, rep))
		return false;

	msgfile = strtok(buffer, "\t");
	mnflag = strtok(0, "\t");
	rawattr = strtok(0, "\t");

	l->privat = (*mnflag == 'm');

	mytmpnam(l->fname);

	if ((orgfile = upWorkList->ftryopen(msgfile, "rb"))) {
		fseek(orgfile, 4, SEEK_SET);
		if (!l->sHead.init(orgfile))
			return false;
		const char *to = l->sHead.To();
		if (to)
			l->na = fromAddr(to);
		if ((destfile = fopen(l->fname, "wt"))) {
			int c;
			while ((c = fgetc(orgfile)) != EOF) {
				fputc(c, destfile);
				count++;
			}
			fclose(destfile);
		}
		fclose(orgfile);
	}

	l->msglen = l->sHead.msglen = count;
	l->refnum = 0;

	l->origArea = l->privat ? 1 : -1;

	remove(msgfile);

	return true;
}

// convert all replies
void souprep::getReplies(FILE *repFile)
{
	noOfLetters = 0;

	upl_soup baseUplList, *currUplList = &baseUplList;

	while (!feof(repFile)) {
		currUplList->nextRecord = new upl_soup;
		currUplList = (upl_soup *) currUplList->nextRecord;
		if (!getRep1(repFile, currUplList)) {
			delete currUplList;
			break;
		}
		noOfLetters++;
	}
	uplListHead = baseUplList.nextRecord;
}

area_header *souprep::getNextArea()
{
	return new area_header(mm, 0, "REPLY", "REPLIES",
		"Letters written by you", "SOUP replies",
		(COLLECTION | REPLYAREA | ACTIVE | PUBLIC | PRIVATE),
		noOfLetters, 0, 99, 511);
}

letter_header *souprep::getNextLetter()
{
	upl_soup *current = (upl_soup *) uplListCurrent;
	const char *to = current->sHead.To();
	const char *ng = current->sHead.Newsgrps();
	int cn = current->origArea;

	if (ng && (cn == -1)) {
		for (int c = 2; c < mm->areaList->noOfAreas(); c++)
			if (strstr(ng, mm->areaList->getDescription(c))) {
				cn = c;
				break;
			}
		if (cn == -1)
			cn = 2;

		current->origArea = cn;
	}

	letter_header *newLetter = new letter_header(mm,
		current->sHead.Subject(), to ? fromName(to) : "All",
		current->sHead.From(), current->sHead.Date(),
		current->sHead.Refs(), current->refnum, currentLetter,
		currentLetter, cn, current->privat, current->msglen, this,
		current->na, true, ng);

	currentLetter++;
	uplListCurrent = uplListCurrent->nextRecord;
	return newLetter;
}

void souprep::enterLetter(letter_header &newLetter,
				const char *newLetterFileName, int length)
{
	upl_soup *newList = new upl_soup;
	memset(newList, 0, sizeof(upl_soup));

	newList->origArea = mm->areaList->getAreaNo();
	newList->privat = newLetter.getPrivate();
	newList->refnum = newLetter.getReplyTo();
	newList->na = newLetter.getNetAddr();

	newList->sHead.init(newLetter.getFrom(), newLetter.getTo(),
		(const char *) newLetter.getNetAddr(),
		newLetter.getSubject(), newLetter.getNewsgrps(),
		newLetter.getMsgID(), length);

	strcpy(newList->fname, newLetterFileName);
	newList->msglen = length;

	addUpl(newList);
}

// write out one reply in SOUP format
void souprep::addRep1(FILE *rep, upl_base *node, int recnum)
{
	FILE *orgfile, *destfile;
	upl_soup *l = (upl_soup *) node;
	char dest[13];

	sprintf(dest, "R%07d.MSG", recnum);
	fprintf(rep, "R%07d\t%sn\n", recnum, l->privat ?
		"mail\tb" : "news\tB");

	if ((orgfile = fopen(l->fname, "rt"))) {

		char *replyText = new char[l->msglen + 1];

		fread(replyText, l->msglen, 1, orgfile);
		fclose(orgfile);

		replyText[l->msglen] = '\0';

		if ((destfile = fopen(dest, "wb"))) {

			l->sHead.output(destfile);

			char *lastsp = 0, *q = replyText;
			int count = 0;

			for (char *p = replyText; *p; p++) {
				if (*p == '\n') {
					*p = '\0';
					fprintf(destfile, "%s\n", q);
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
				fprintf(destfile, "%s\n", q);
			fclose(destfile);
		}
		delete[] replyText;
	}
}

void souprep::addHeader(FILE *repFile)
{
	repFile = repFile;	// nothing to do but supress warnings
}

// set names for reply packet files
void souprep::repFileName()
{
        const char *basename = baseClass->getBaseName();

	sprintf(replyPacketName, "%s.rep", basename);
	sprintf(replyInnerName, "REPLIES");
}

// list files to be archived when creating reply packet
const char *souprep::repTemplate(bool offres)
{
	offres = offres;
	return "REPLIES *.MSG";
}

// re-read an offline config file -- not implemented yet
bool souprep::getOffConfig()
{
	return false;
}

bool souprep::makeOffConfig()
{
	return false;
}
