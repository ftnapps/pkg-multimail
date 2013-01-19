/*
 * MultiMail offline mail reader
 * Blue Wave

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef BW_H
#define BW_H

#include "pktbase.h"

#ifndef BIG_ENDIAN
#define BIG_ENDIAN
#endif

#include "bluewave.h"

class bluewave : public pktbase
{
	FILE *ftiFile;
	INF_AREA_INFO *areas;
	MIX_REC *mixRecord;

	struct perstype {
		int area, msgnum;
	} *persNdx;

	long personal;

	int *mixID;
	int noOfInfRecs, noOfMixRecs;
	int from_to_len;
	int subject_len;
	unsigned infoHeaderLen, infoAreainfoLen, mixStructLen, ftiStructLen;

	FILE *openFile(const char *);
	void findInfBaseName();
	void initInf();
	void initMixID();
 public:
	bluewave(mmail *);
	~bluewave();
	area_header *getNextArea();
	int getNoOfLetters();
	letter_header *getNextLetter();
	const char *getBody(letter_header &);
	const char *saveOldFlags();
};
	
class bwreply : public pktreply
{
	class upl_bw : public upl_base {
	 public:
		UPL_REC uplRec;
		char *msgid, *newsgrps, *extsubj;

		upl_bw();
		~upl_bw();
	};

	UPL_HEADER *uplHeader;

	int getAreaFromTag(const char *);
	bool getRep1(FILE *, upl_bw *, int);
	void getReplies(FILE *);
	const char *freeFileName();
	void addRep1(FILE *, upl_base *, int);
	void addHeader(FILE *);
	void repFileName();
	const char *repTemplate(bool);
	char *nextLine(FILE *);
 public:
	bwreply(mmail *, specific_driver *);
	~bwreply();
	area_header *getNextArea();
	letter_header *getNextLetter();
	void enterLetter(letter_header &, const char *, int);
	bool getOffConfig();
	bool makeOffConfig();
};

#endif
