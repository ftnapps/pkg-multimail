/*
 * MultiMail offline mail reader
 * Packet base class

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef PKT_H
#define PKT_H

#include "mmail.h"

class pktbase : public specific_driver
{
 protected:
	struct bodytype {
		long pointer, msgLength;
	} **body;

	struct AREAs {
		char *name;
		int num, nummsgs, attr;
		char numA[10];  // padded to deal with alignment bug (EMX)
	} *areas;

	struct ndx_fake {
		int confnum;
		long pointer, length;
		bool pers;
		ndx_fake *next;
	};

	mmail *mm;
	file_header **bulletins;

	FILE *infile;
	char packetBaseName[9], *bodyString;
	int maxConf, numMsgs, ID, currentArea, currentLetter;
	unsigned hasOffConfig;
	bool hasPers;

	void cleanup();
	void initBody(ndx_fake *, int);
	int getYNum(int, unsigned long);
	bool hasPersArea();
	void checkLatin(letter_header &);
	const char *getHidden(const char *, char *&);
	void listBulletins(const char [][13], int, int = 2);
	char *nextLine();
 public:
	int getXNum(int);
	int getNoOfAreas();
	virtual int getNoOfLetters();
	void selectArea(int);
	void resetLetters();
	virtual file_header *getFileList();
	file_header **getBulletins();
	const char *getBaseName();
};

class pktreply : public reply_driver
{
 protected:
	class upl_base {
	 public:
		char fname[50];
		upl_base *nextRecord;
		long msglen;
	} *uplListHead, *uplListCurrent;

	mmail *mm;
	pktbase *baseClass;
	file_list *upWorkList;

	char replyPacketName[13], replyInnerName[13], *replyText;
	int currentLetter, noOfLetters;
	bool replyExists;

	void cleanup();
	void uncompress();
	virtual void getReplies(FILE *) = 0;
	void readRep();
	virtual void repFileName() = 0;
	void addUpl(upl_base *);
	virtual void addRep1(FILE *, upl_base *, int) = 0;
	virtual void addHeader(FILE *) = 0;
	virtual const char *repTemplate(bool) = 0;
 public:
	bool checkForReplies();
	void init();
	int getNoOfAreas();
	void selectArea(int);
	int getNoOfLetters();
	void resetLetters();
	const char *getBody(letter_header &);
	file_header *getFileList();
	file_header **getBulletins();
	void killLetter(int);
	area_header *refreshArea();
	bool makeReply();
	void deleteReplies();
};

#endif
