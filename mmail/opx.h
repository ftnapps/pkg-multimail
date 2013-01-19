/*
 * MultiMail offline mail reader
 * OPX (Silver Xpress)

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef OPX_H
#define OPX_H

#include "pktbase.h"
#include "opxstrct.h"

class opxpack : public pktbase
{
	char *pstrget(void *);
	void readBrdinfoDat();
	void buildIndices();
 public:
	opxpack(mmail *);
	~opxpack();
	area_header *getNextArea();
	letter_header *getNextLetter();
	const char *getBody(letter_header &);
};

class opxreply : public pktreply
{
	class upl_opx : public upl_base {
	 public:
		repHead rhead;
		net_address na;
		char *msgid;
		int area;

		upl_opx();
		~upl_opx();
	};

	int getArea(const char *);
	bool getRep1(const char *, upl_opx *);
	void getReplies(FILE *);
	const char *freeFileName(upl_opx *);
	void addRep1(FILE *, upl_base *, int);
	void addHeader(FILE *);
	void repFileName();
	const char *repTemplate(bool);
 public:
	opxreply(mmail *, specific_driver *);
	~opxreply();
	area_header *getNextArea();
	letter_header *getNextLetter();
	void enterLetter(letter_header &, const char *, int);
	bool getOffConfig();
	bool makeOffConfig();
};

#endif
