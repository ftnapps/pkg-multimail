/*
 * MultiMail offline mail reader
 * resource class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <cstdio>

enum {
	UserName, InetAddr, QuoteHead, InetQuote, noOfRaw
};

enum {
	homeDir = noOfRaw, mmHomeDir, PacketDir, WorkDir,
	UncompressCommand, PacketName, BBSName, SysOpName, ReplyDir,
	LoginName, AliasName, CompressCommand, UpWorkDir, editor, SaveDir,
	AddressFile, TaglineFile, arjUncompressCommand,
	zipUncompressCommand, lhaUncompressCommand, rarUncompressCommand,
	unknownUncompressCommand, arjCompressCommand, zipCompressCommand,
	lhaCompressCommand, rarCompressCommand, unknownCompressCommand,
	sigFile, ColorFile, oldPacketName, noOfStrings
};

enum {
	PacketSort = noOfStrings, LetterSort, Charset, UseTaglines,
	AutoSaveReplies, AutoSaveRead, StripSoftCR, BeepOnPers,
	UseLynxNav, UseScrollBars, BuildPersArea, MakeOldFlags,
	QuoteWrapCols, MaxLines, noOfResources
};

class baseconfig
{
 protected:
	const char **names, **comments, **intro;
	int configItemNum;

	bool parseConfig(const char *);
	void newConfig(const char *);
	virtual void processOne(int, const char *) = 0;
	virtual const char *configLineOut(int) = 0;
 public:
	virtual ~baseconfig();
};

class resource : public baseconfig
{
	static const char *rc_names[], *rc_intro[], *rc_comments[];
	static const int startUp[], defInt[];
 
	char basedir[256];

	char *resourceData[noOfStrings];
	int resourceInt[noOfResources - noOfStrings];

	void homeInit();
	void mmEachInit(int, const char *);
	void subPath(int, const char *);
	void initinit();
	void mmHomeInit();
	void processOne(int, const char *);
	const char *configLineOut(int);
	const char *fixPath(const char *);
	bool checkPath(const char *, bool);
	bool verifyPaths();
 public:
	resource();
	~resource();
	const char *get(int) const;
	int getInt(int) const;
	void set(int, const char *);
	void set(int, int);
};
	  
#endif
