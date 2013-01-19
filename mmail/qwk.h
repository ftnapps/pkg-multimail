/*
 * MultiMail offline mail reader
 * QWK

 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef QWK_H
#define QWK_H

#include "pktbase.h"

#define ndxRecLen 5

#define getQfield(d, s, l) { strncpy(d, s, l); d[l] = '\0'; }

class qheader {
	struct qwkmsg_header {
		char status;
		char msgnum[7];		// in ASCII
		char date[8];		// ASCII MM-DD-YY date
		char time[5];		// time in HH:MM ASCII
		char to[25];		// TO
		char from[25];		// FROM
		char subject[25];	// subject of message
		char password[12];	// message passw.
		char refnum[8];		// in ASCII
		char chunks[6];		// number of 128 byte chunks
		char alive;		// msg is alive/killed
		unsigned char confLSB;
		unsigned char confMSB;
		char res[3];
	};

 public:
	char from[26], to[26], subject[72], date[15];
	int msglen, msgnum, refnum, origArea;
	bool privat, netblock;
	//netaddress na;	// not yet used, but could be!

	bool init(FILE *);
	void output(FILE *);
};

class qwkpack : public pktbase
{
	char textfiles[3][13];
	char controlname[26];
	bool qwke;

	unsigned long MSBINtolong(unsigned const char *);
	void readControlDat();
	void readDoorId();
	void readToReader();
	bool externalIndex();
	void readIndices();
 public:
	qwkpack(mmail *);
	~qwkpack();
	area_header *getNextArea();
	letter_header *getNextLetter();
	const char *getBody(letter_header &);
	bool isQWKE();
	const char *ctrlName();
};

class qwkreply : public pktreply
{
	class upl_qwk : public upl_base {
	 public:
		qheader qHead;
	};

	bool qwke;

	bool getRep1(FILE *, upl_qwk *);
	void getReplies(FILE *);
	int monthval(const char *);
	void addRep1(FILE *, upl_base *, int);
	void addHeader(FILE *);
	void repFileName();
	const char *repTemplate(bool);
 public:
	qwkreply(mmail *, specific_driver *);
	~qwkreply();
	area_header *getNextArea();
	letter_header *getNextLetter();
	void enterLetter(letter_header &, const char *, int);
	bool getOffConfig();
	bool makeOffConfig();
};

#endif
