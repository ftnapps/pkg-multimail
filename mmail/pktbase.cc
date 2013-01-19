/*
 * MultiMail offline mail reader
 * Packet base class -- common methods

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "compress.h"
#include "pktbase.h"

/* Not all these methods are used in all derived classes. The formats I
   think of as "QWK-like" -- QWK, OMEN and OPX -- have the most in common:
   message headers and bodies concatenated in a single file, a separate
   file for the area list, and replies matched by conference number.
*/

// ---------------------------------------------------------------------------
// The packet methods
// ---------------------------------------------------------------------------

// Clean up for the QWK-like packets
void pktbase::cleanup()
{
	delete[] bulletins;

	if (!hasPers) {
		areas--;
		maxConf++;
	}
	while (maxConf--) {
		delete[] body[maxConf];
		delete[] areas[maxConf].name;
	}
	delete[] body;
	delete[] areas;
	delete[] bodyString;

	fclose(infile);
}

// Final index build for QWK-like packets
void pktbase::initBody(ndx_fake *tmpndx, int personal)
{
	int x, cMsgNum;

	hasPers = !(!personal);
	if (hasPers) {
		body[0] = new bodytype[personal];
		areas[0].nummsgs = personal;
		personal = 0;
	} else {
		areas++;
		maxConf--;
	}

	for (x = hasPers; x < maxConf; x++) {
		cMsgNum = areas[x].nummsgs;
		if (cMsgNum) {
			body[x] = new bodytype[cMsgNum];
			areas[x].nummsgs = 0;
		}
	}

	for (x = 0; x < numMsgs; x++) {
		int current = tmpndx->confnum - !hasPers;
		body[current][areas[current].nummsgs].pointer =
			tmpndx->pointer;
		body[current][areas[current].nummsgs++].msgLength =
			tmpndx->length;

		if (tmpndx->pers) {
			body[0][personal].pointer = tmpndx->pointer;
			body[0][personal++].msgLength = tmpndx->length;
		}
		ndx_fake *oldndx = tmpndx;
		tmpndx = tmpndx->next;
		delete oldndx;
	}
}

// Match the conference number to the internal number
int pktbase::getXNum(int area)
{
	int c;

	for (c = 0; c < maxConf; c++)
		if (areas[c].num == area)
			break;
	return c;
}

// Find the original number of a message in a collection area
int pktbase::getYNum(int area, unsigned long rawpos)
{
	int c;

	for (c = 0; c < areas[area].nummsgs; c++)
		if ((unsigned) body[area][c].pointer == rawpos)
			break;
	return c;
}

int pktbase::getNoOfLetters()
{
	return areas[currentArea].nummsgs;
}

bool pktbase::hasPersArea()
{
	return hasPers;
}

int pktbase::getNoOfAreas()
{
	return maxConf;
}

void pktbase::selectArea(int area)
{
	currentArea = area;
	resetLetters();
}

void pktbase::resetLetters()
{
	currentLetter = 0;
}

// Check the character set kludge lines
void pktbase::checkLatin(letter_header &mhead)
{
	const char *s = strstr(bodyString, "\001CHRS: L");
	if (!s)
		s = strstr(bodyString, "\001CHARSET: L");
	if (s)
		mhead.setLatin(true);
}

// Find a hidden line and return its contents
const char *pktbase::getHidden(const char *pattern, char *&end)
{
	const char *s = strstr(bodyString, pattern);

	if (s) {
		s += strlen(pattern);
		end = strchr(s, '\n');
		if (end)
			*end = '\0';
	}
	return s;
}

// Build a list of bulletin files
void pktbase::listBulletins(const char x[][13], int d, int generic)
{
	file_list *wl = mm->workList;
	int filecount = 0;

	bulletins = new file_header *[wl->getNoOfFiles() + 1];

	for (int c = 0; c < d; c++)
		if (x[c][0])
			wl->addItem(bulletins, x[c], filecount);

	if (generic) {
		wl->addItem(bulletins, "blt", filecount);
		if (generic == 2)
			wl->addItem(bulletins, ".txt", filecount);
	}

	if (filecount)
		bulletins[filecount] = 0;
	else {
		delete[] bulletins;
		bulletins = 0;
	}
}

file_header *pktbase::getFileList()
{
	return mm->workList->existsF("newfiles.");
}

file_header **pktbase::getBulletins()
{
	return bulletins;
}

const char *pktbase::getBaseName()
{
	return packetBaseName;
}

char *pktbase::nextLine()
{
	static char line[128];

	char *end = myfgets(line, sizeof line, infile);
	if (end)
		while ((*end == '\n') || (*end == '\r'))
			*end-- = '\0';
	return line;
}

// ---------------------------------------------------------------------------
// The reply methods
// ---------------------------------------------------------------------------

void pktreply::cleanup()
{
	if (replyExists) {
		upl_base *next, *curr = uplListHead;

		while (noOfLetters--) {
			remove(curr->fname);
			next = curr->nextRecord;
			delete curr;
			curr = next;
		}
		delete[] replyText;
	}
}

bool pktreply::checkForReplies()
{
	repFileName();
	mychdir(mm->resourceObject->get(ReplyDir));
	replyExists = writeable(replyPacketName);
	return replyExists;
}

void pktreply::init()
{
	if (replyExists) {
		uncompress();
		readRep();
		currentLetter = 1;
	} else
		noOfLetters = currentLetter = 0;
}

void pktreply::uncompress()
{
	char fname[256];

	sprintf(fname, "%s/%s", mm->resourceObject->get(ReplyDir),
		replyPacketName);
	uncompressFile(mm->resourceObject, fname,
		mm->resourceObject->get(UpWorkDir));
}

int pktreply::getNoOfAreas()
{
	return 1;
}

void pktreply::selectArea(int ID)
{
	if (ID == 0)
		resetLetters();
}

int pktreply::getNoOfLetters()
{
	return noOfLetters;
}

void pktreply::resetLetters()
{
	currentLetter = 1;
	uplListCurrent = uplListHead;
}

const char *pktreply::getBody(letter_header &mhead)
{
	FILE *replyFile;
	upl_base *actUplList;
	size_t msglen;

	int ID = mhead.getLetterID();

	delete[] replyText;

	actUplList = uplListHead;
	for (int c = 1; c < ID; c++)
		actUplList = actUplList->nextRecord;

	if ((replyFile = fopen(actUplList->fname, "rt"))) {
		msglen = actUplList->msglen;
		replyText = new char[msglen + 1];
		msglen = fread(replyText, 1, msglen, replyFile);
		fclose(replyFile);
		replyText[msglen] = '\0';
	} else
		replyText = strdupplus("\n");

	return replyText;
}

file_header *pktreply::getFileList()
{
	return 0;
}

file_header **pktreply::getBulletins()
{
	return 0;
}

void pktreply::readRep()
{
	upWorkList = new file_list(mm->resourceObject->get(UpWorkDir));

	FILE *repFile = upWorkList->ftryopen(replyInnerName, "rb");

	if (repFile) {
		getReplies(repFile);
		fclose(repFile);
		remove(upWorkList->exists(replyInnerName));
	} else
		fatalError("Error opening reply packet");

	delete upWorkList;
}

void pktreply::addUpl(upl_base *newList)
{
        if (!noOfLetters)
                uplListHead = newList;
        else {
                upl_base *workList = uplListHead;
                for (int c = 1; c < noOfLetters; c++)   //go to last elem
                        workList = workList->nextRecord;
                workList->nextRecord = newList;
        }
        noOfLetters++;
        replyExists = true;
}

void pktreply::killLetter(int letterNo)
{
	upl_base *actUplList, *tmpUplList;

	if (!noOfLetters || (letterNo < 1) || (letterNo > noOfLetters))
		fatalError("Internal error in pktreply::killLetter");

	if (letterNo == 1) {
		tmpUplList = uplListHead;
		uplListHead = uplListHead->nextRecord;
	} else {
		actUplList = uplListHead;
		for (int c = 1; c < letterNo - 1; c++)
			actUplList = actUplList->nextRecord;
		tmpUplList = actUplList->nextRecord;
		actUplList->nextRecord = (letterNo == noOfLetters) ? 0 :
			actUplList->nextRecord->nextRecord;
	}
	noOfLetters--;
	remove(tmpUplList->fname);
	delete tmpUplList;
	resetLetters();
}

area_header *pktreply::refreshArea()
{
	return getNextArea();
}

bool pktreply::makeReply()
{
	if (mychdir(mm->resourceObject->get(UpWorkDir)))
		fatalError("Could not cd to upworkdir in pktreply::makeReply");

	// Delete old packet
	deleteReplies();

	bool offres = mm->areaList->anyChanged();
	if (offres)
		offres = makeOffConfig();
	if (!noOfLetters && !offres)
		return true;

	FILE *repFile;

	repFile = fopen(replyInnerName, "wb");	//!! no check yet

	addHeader(repFile);

	upl_base *actUplList = uplListHead;
	for (int c = 0; c < noOfLetters; c++) {
		addRep1(repFile, actUplList, c);
		actUplList = actUplList->nextRecord;
	};

	fclose(repFile);

	//pack the files
	int result = compressAddFile(mm->resourceObject,
		mm->resourceObject->get(ReplyDir), replyPacketName,
		repTemplate(offres));

	// clean up the work area
	clearDirectory(mm->resourceObject->get(UpWorkDir));

	return !result && checkForReplies();
}

void pktreply::deleteReplies()
{
	char tmp[256];

	sprintf(tmp, "%s/%s", mm->resourceObject->get(ReplyDir),
		replyPacketName);
	remove(tmp);
}
