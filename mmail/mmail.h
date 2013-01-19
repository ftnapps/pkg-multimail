/*
 * MultiMail offline mail reader
 * mmail class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef MMAIL_H 
#define MMAIL_H

#define MM_NAME "MultiMail"

#include "misc.h"
#include "resource.h"
#include "../interfac/mysystem.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

// Number of the Reply area
#define REPLY_AREA 0

// Area types -- bit f.
enum {COLLECTION = 1, REPLYAREA = 2, ACTIVE = 4, ALIAS = 8, NETMAIL = 16,
        INTERNET = 32, PUBLIC = 64, PRIVATE = 128, LATINCHAR = 256,
	ECHO = 512, PERSONLY = 1024, PERSALL = 2048, SUBKNOWN = 4096,
	ADDED = 8192, DROPPED = 16384, OFFCONFIG = 32768};

// Mail statuses -- bit f.
enum {MS_READ = 1, MS_REPLIED = 2, MS_MARKED = 4, MS_PERSTO = 8,
	MS_PERSFROM = 16};

// For letter_list::sort
enum {LS_SUBJ, LS_MSGNUM, LS_FROM, LS_TO};

enum pktstatus {PKT_OK, UNCOMP_FAIL, PTYPE_UNK, NEW_DIR, PKT_UNFOUND};

class mmail;
class resource;
class file_header;
class file_list;
class area_header;
class area_list;
class letter_header;
class letter_list;
class specific_driver;
class reply_driver;
class driver_list;
class read_class;

class net_address
{
	char *inetAddr;

	void copy(net_address &);
 public:
	int zone, net, node, point;
	bool isInternet, isSet;

	net_address();
	net_address(net_address &);
	~net_address();
	bool operator==(net_address &);
	net_address &operator=(const char *);
	net_address &operator=(net_address &);
	operator const char *();
};
 
class mmail
{
 public:
	resource *resourceObject;
	file_list *workList;
	driver_list *driverList;
	area_list *areaList;
	letter_list *letterList;

	mmail();
	~mmail();
	pktstatus selectPacket(const char *);
	void Delete();
	bool saveRead();
	file_header *getFileList();
	file_header **getBulletins();
	bool isLatin();
	bool checkForReplies();
	bool makeReply();
	void deleteReplies();
	void openReply();
	bool getOffConfig();
};	

class file_header
{
	char *name;
	time_t date;
	off_t size;
 public:
	file_header *next;

	file_header(const char *, time_t, off_t);
	~file_header();

	const char *getName() const;
	time_t getDate() const;
	off_t getSize() const;
};

class file_list
{
	file_header **files;
	char *DirName;
	size_t dirlen;
	int noOfFiles, activeFile;
	bool sorttype, dirlist;

	void initFiles();
	friend int fnamecomp(const void *, const void *);
	friend int ftimecomp(const void *, const void *);
	void sort();
 public:
	file_list(const char *, bool = false, bool = false);
	~file_list();

	void resort();

	int getNoOfFiles() const;
	const char *getDirName() const;
	void gotoFile(int);
	const char *changeDir(const char * = 0);
	int changeName(const char *);

	const char *getName() const;
	time_t getDate() const;
	off_t getSize() const;

	const char *getNext(const char *);
	file_header *getNextF(const char *);
	const char *exists(const char *);
	file_header *existsF(const char *);

	void addItem(file_header **, const char *, int &);

	const char *expandName(const char *);
	FILE *ftryopen(const char *, const char *);
	void kill();
};

class area_header
{
	mmail *mm;
	specific_driver *driver;
	const char *shortName, *name, *description, *areaType;
	int noOfLetters, noOfPersonal, num, maxtolen, maxsublen;
	unsigned type;
 public:
	area_header(mmail *, int, const char *, const char *, const char *,
		const char *, unsigned, int, int, int, int);
	inline const char *getShortName() const;
	inline const char *getName() const;
	inline const char *getDescription() const;
	inline const char *getAreaType() const;
	inline unsigned getType() const;
 	inline int getNoOfLetters() const;
	inline int getNoOfUnread();
	inline int getNoOfMarked();
	inline int getNoOfPersonal() const;
	inline bool getUseAlias() const;
	inline bool isCollection() const;
	inline bool isReplyArea() const;
	inline bool isNetmail() const;
	inline bool isInternet() const;
	inline bool isEmail() const;
	inline bool isUsenet() const;
	inline bool isLatin() const;
	inline bool hasTo() const;
	inline bool hasPublic() const;
	inline bool hasPrivate() const;
	inline int maxToLen() const;
	inline int maxSubLen() const;
	inline bool hasOffConfig() const;
	inline void Add();
	inline void Drop();
	bool isActive() const;
};

class area_list
{
	mmail *mm;
	area_header **areaHeader;
	int no, noActive, current, *activeHeader;
	bool shortlist;
 public:
	area_list(mmail *);
	~area_list();
	void relist();
	void updatePers();

	const char *getShortName() const;
	const char *getName() const;
	const char *getName(int);
	const char *getDescription() const;
	const char *getDescription(int);
	const char *getAreaType() const;
	unsigned getType() const;
	int getNoOfLetters() const;
	int getNoOfUnread() const;
	int getNoOfMarked() const;
	int getNoOfPersonal() const;
	bool getUseAlias() const;
	bool isCollection() const;
	bool isReplyArea() const;
	bool isEmail() const;
	bool isNetmail() const;
	bool isInternet() const;
	bool isUsenet() const;
	bool isLatin() const;
	bool isLatin(int);
	bool hasTo() const;
	bool hasPublic() const;
	bool hasPrivate() const;
	int maxToLen() const;
	int maxSubLen() const;
	bool hasOffConfig() const;
	void Add();
	void Drop();

	bool isShortlist();
	void getLetterList();
	void enterLetter(int, const char *, const char *, const char *,
			const char *, const char *, int, bool,
			net_address &, const char *, int);
	void killLetter(int);
	void refreshArea();
	void gotoArea(int);
	void gotoActive(int);
	int getAreaNo() const;
	int getActive();
	int noOfAreas() const;
	int noOfActive() const;
	int findNetmail() const;
	int findInternet() const;
	bool anyChanged() const;
};

class letter_header
{
	driver_list *dl;
	read_class *readO;
	specific_driver *driver;
	char *subject, *to, *from, *date, *msgid, *newsgrps, *follow, *reply;
	int replyTo, LetterID, AreaID, length, msgNum;
	bool privat, persfrom, persto, charset;
	net_address netAddr;
 public:
	letter_header(mmail *, const char *, const char *, const char *,
		const char *, const char *, int, int, int, int, bool, int,
		specific_driver *, net_address &, bool = false,
		const char * = 0, const char * = 0, const char * = 0);
	~letter_header();

	void changeSubject(const char *);
	void changeTo(const char *);
	void changeFrom(const char *);
	void changeDate(const char *);
	void changeMsgID(const char *);
	void changeNewsgrps(const char *);
	void changeFollow(const char *);
	void changeReplyTo(const char *);

	const char *getSubject() const;
	const char *getTo() const;
	const char *getFrom() const;
	const char *getDate() const;
	const char *getMsgID() const;
	const char *getNewsgrps() const;
	const char *getFollow() const;
	const char *getReply() const;
	net_address &getNetAddr();
	int getReplyTo() const;
	bool getPrivate() const;
	inline specific_driver *getDriver() const;
	inline const char *getBody();
	int getLetterID() const;
	int getAreaID() const;
	inline int getMsgNum() const;
	inline int getLength() const;
	inline bool isPersonal() const;
	inline bool isLatin() const;

	void setLatin(bool);

	inline bool getRead();
	inline void setRead();
	inline int getStatus();
	inline void setStatus(int);
};

class letter_list
{ 
	driver_list *dl;
	specific_driver *driver;
	read_class *readO;
	letter_header **letterHeader;
	int noOfLetters, noActive, areaNumber, currentLetter;
	int *activeHeader;
	bool isColl, shortlist;

	void init();
	void cleanup();
	friend int lettercomp(const void *, const void *);
	friend int lmsgncomp(const void *, const void *);
	void sort();
 public:
	letter_list(mmail *, int, bool);
	~letter_list();
	void relist();
	void resort();

	const char *getSubject() const;
	const char *getTo() const;
	const char *getFrom() const;
	const char *getDate() const;
	const char *getMsgID() const;
	const char *getNewsgrps() const;
	const char *getFollow() const;
	const char *getReply() const;
	const char *getBody();
	net_address &getNetAddr();
	int getReplyTo() const;
	int getMsgNum() const;
	int getAreaID() const;
	bool getPrivate() const;
	int getLength() const;
	bool isPersonal() const;
	bool isLatin() const;

	int getStatus();
	void setStatus(int);
	bool getRead();
	void setRead();
	
	void rrefresh();
	bool findReply(int, int);
	int noOfLetter() const;
	int noOfActive() const;
	void gotoLetter(int);
	void gotoActive(int);
	int getCurrent() const;
	int getActive() const;
};

class driver_list
{
	struct driver_struct {
		specific_driver *driver;
		read_class *read;
		int offset;
	} driverList[2];

	int noOfDrivers, attributes;
 public:
	driver_list(mmail *);
	~driver_list();
	int getNoOfDrivers() const;
	specific_driver *getDriver(int);
	reply_driver *getReplyDriver();
	read_class *getReadObject(specific_driver *);
	int getOffset(specific_driver *);
	bool hasPersonal() const;
	bool useTearline() const;
};

class read_class
{
 public:
	virtual ~read_class();
	virtual void setRead(int, int, bool) = 0;
	virtual bool getRead(int, int) = 0;
	virtual void setStatus(int, int, int) = 0;
	virtual int getStatus(int, int) = 0;
	virtual int getNoOfUnread(int) = 0;
	virtual int getNoOfMarked(int) = 0;
	virtual bool saveAll() = 0;
};

class main_read_class : public read_class
{
	resource *ro;
	specific_driver *driver;
	int noOfAreas, **readStore, *noOfLetters;
	bool hasPersArea, hasPersNdx;
 public:
	main_read_class(mmail *, specific_driver *);
	~main_read_class();
	void setRead(int, int, bool);
	bool getRead(int, int);
	void setStatus(int, int, int);
	int getStatus(int, int);
	int getNoOfUnread(int);
	int getNoOfMarked(int);
	bool saveAll();
	const char *readFilePath(const char *);
};

class reply_read_class: public read_class
{
 public:
	reply_read_class(mmail *, specific_driver *);
	~reply_read_class();
	void setRead(int, int, bool);
	bool getRead(int, int);
	void setStatus(int, int, int);
	int getStatus(int, int);
	int getNoOfUnread(int);
	int getNoOfMarked(int);
	bool saveAll();
};	

class specific_driver
{
 public:
	virtual ~specific_driver();
	virtual bool hasPersArea();
	virtual bool isLatin();
	virtual const char *saveOldFlags();
	virtual int getNoOfAreas() = 0;
	virtual area_header *getNextArea() = 0;
	virtual void selectArea(int) = 0;
	virtual int getNoOfLetters() = 0;
	virtual void resetLetters() = 0;
	virtual letter_header *getNextLetter() = 0;
	virtual const char *getBody(letter_header &) = 0;
	virtual file_header *getFileList() = 0;
	virtual file_header **getBulletins() = 0;
};

class reply_driver : public specific_driver
{
 public:	
	virtual ~reply_driver();
	virtual bool checkForReplies() = 0;
	virtual void init() = 0;
	virtual void enterLetter(letter_header &, const char *, int) = 0;
	virtual void killLetter(int) = 0;
	virtual area_header *refreshArea() = 0;
	virtual bool makeReply() = 0;
	virtual void deleteReplies() = 0;
	virtual bool getOffConfig() = 0;
	virtual bool makeOffConfig() = 0;
};

// Letter sort type flag
extern int lsorttype;

#endif
