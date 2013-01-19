/*
 * MultiMail offline mail reader
 * resource class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "mmail.h"
#include "../interfac/error.h"

/* Default filenames. Note that EMX now serves for the Win32 port (via
   RSXNT), as well as for OS/2; one of the few differences is the default
   editor defined here:
*/

#ifdef __MSDOS__
# define DEFEDIT "edit"
# define DEFZIP "pkzip"
# define DEFUNZIP "pkunzip -o"
# define DEFLHA "lha a /m"
# define DEFUNLHA "lha e"
#else
# ifdef __EMX__
#  ifdef __WIN32__
#   define DEFEDIT "edit"
#  else
#   define DEFEDIT "tedit"
#  endif
# else
#  define DEFEDIT "vi"
# endif
# define DEFZIP "zip -jk"
# define DEFUNZIP "unzip -joL"
# define DEFLHA "lha af"
# define DEFUNLHA "lha efi"
#endif

#define DEFARJ "arj a -e"
#define DEFUNARJ "arj e"
#define DEFRAR "rar u -ep"
#define DEFUNRAR "rar e -cl -o+"

#define DEFNONE "xxcompress"
#define DEFUNNONE "xxuncompress"

#if defined (__MSDOS__) || defined (__EMX__)
# define RCNAME "mmail.rc"
# define ADDRBOOK "address.bk"
#else
# define RCNAME ".mmailrc"
# define ADDRBOOK "addressbook"
#endif

// ================
// baseconfig class
// ================

baseconfig::~baseconfig()
{
}

bool baseconfig::parseConfig(const char *configFileName)
{
	FILE *configFile;
	char buffer[256], *pos, *resName, *resValue;
	int vermajor = 0, verminor = 0;

	if ((configFile = fopen(configFileName, "rt"))) {
	    while (myfgets(buffer, sizeof buffer, configFile)) {
		if ((buffer[0] != '#') && (buffer[0] != '\n')) {
			pos = buffer;

			//leading spaces
			while (*pos == ' ' || *pos == '\t')
				pos++;

			//skip "bw" -- for backwards compatiblity
			if (*pos == 'b' && pos[1] == 'w')
				pos += 2;

			//resName
			resName = pos;
			while (*pos != ':' && *pos != '=' &&
				*pos != ' ' && *pos != '\t' && *pos)
					pos++;
			if (*pos)
				*pos++ = '\0';

			//chars between strings
			while (*pos == ' ' || *pos == '\t' ||
				*pos == ':' || *pos == '=')
					pos++;

			//resValue
			resValue = pos;
			while (*pos != '\n' && *pos)
				pos++;
			*pos = '\0';

			if (!strncasecmp("ver", resName, 3))
				sscanf(resValue, "%d.%d", &vermajor,
					&verminor);
			else
				for (int c = 0; c < configItemNum; c++)
					if (!strcasecmp(names[c], resName)) {
						processOne(c, resValue);
						break;
					}
		}
	    }
	    fclose(configFile);
	}

	// Does the config file need updating?
	return (vermajor < MM_MAJOR) || ((vermajor == MM_MAJOR) &&
		(verminor < MM_MINOR));
}

void baseconfig::newConfig(const char *configname)
{
	FILE *fd;
	const char **p;

	printf("Updating %s...\n", configname);

	if ((fd = fopen(configname, "wt"))) {

		for (p = intro; *p; p++)
			fprintf(fd, "# %s\n", *p);

		fprintf(fd, "\nVersion: %d.%d\n", MM_MAJOR, MM_MINOR);

		for (int x = 0; x < configItemNum; x++) {
			if (comments[x])
				fprintf(fd, "\n# %s\n", comments[x]);
			fprintf(fd, "%s: %s\n", names[x],
				configLineOut(x));
		}
		fclose(fd);
	} else
		fatalError("Error writing config file");
}

// ==============
// resource class
// ==============

const int startUpLen = 38;

const char *resource::rc_names[startUpLen] =
{
	"UserName", "InetAddr", "QuoteHead", "InetQuote",
	"homeDir", "mmHomeDir", "signature", "editor",
	"PacketDir", "ReplyDir", "SaveDir", "AddressBook", "TaglineFile",
	 "ColorFile", "arjUncompressCommand", "zipUncompressCommand",
	"lhaUncompressCommand", "rarUncompressCommand",
	"unknownUncompressCommand", "arjCompressCommand",
	"zipCompressCommand", "lhaCompressCommand", "rarCompressCommand",
	"unknownCompressCommand", "PacketSort", "LetterSort", "Charset",
	"UseTaglines", "AutoSaveReplies", "AutoSaveRead", "StripSoftCR",
	"BeepOnPers", "UseLynxNav", "UseScrollBars", "BuildPersArea",
	"MakeOldFlags", "QuoteWrapCols", "MaxLines"
};

const char *resource::rc_intro[] = {
 "-----------------------",
 MM_NAME " configuration",
 "-----------------------",
 "",
 "Any of these keywords may be omitted, in which case the default values",
 "(shown here) will be used.",
 "",
 "If you change either of the base directories, all the subsequent paths",
 "will be changed, unless they're overriden in the individual settings.",
 0
};

const char *resource::rc_comments[startUpLen] = {
 "Your name, as you want it to appear on replies (used mainly in SOUP)",
 "Your Internet email address (used only in SOUP replies)",
 "Quote header for replies (non-Internet)",
 "Quote header for Internet email and Usenet replies",
 "Base directories (derived from $HOME or $MMAIL)", 0,
 "Signature (file) that should be appended to each message. (Not used\n"
 "# unless specified here.)",
 "Editor for replies = $EDITOR; or if not defined, " DEFEDIT,
 MM_NAME " will look for packets here",
 "Reply packets go here",
 "Saved messages go in this directory, by default",
 "Full paths to the address book and tagline file", 0,
 "Full path to color specification file",
 "Decompression commands (must include an option to junk/discard paths!)",
	0, 0, 0, 0,
 "Compression commands (must include an option to junk/discard paths!)",
	0, 0, 0, 0,
 "Default sort for packet list: by Name or Time (most recent first)",
 "Default sort for letter list: by Subject, Number, From or To",
 "Console character set: CP437 (IBM PC) or Latin-1 (ISO-8859-1)",
 "Prompt to add taglines to replies?",
 "Save replies after editing without prompting?",
 "Save lastread pointers without prompting?",
 "Strip \"soft carriage returns\" (char 141) from messages?",
 "Beep when a personal message is opened in the letter window?",
 "Use Lynx-like navigation (right arrow selects, left backs out)?",
 "Put scroll bars on list windows when items exceed window lines?",
 "Build \"Personal\" area (letters addressed to you)",
 "Generate .XTI file instead of .RED for Blue Wave packets",
 "Wrap quoted text at this column width (including quote marks)",
 "Maximum lines per part for reply split (see docs)"
};

const int resource::startUp[startUpLen] =
{
	UserName, InetAddr, QuoteHead, InetQuote, homeDir, mmHomeDir,
	sigFile, editor, PacketDir, ReplyDir, SaveDir, AddressFile,
	TaglineFile, ColorFile, arjUncompressCommand,
	zipUncompressCommand, lhaUncompressCommand, rarUncompressCommand,
	unknownUncompressCommand, arjCompressCommand, zipCompressCommand,
	lhaCompressCommand, rarCompressCommand, unknownCompressCommand,
	PacketSort, LetterSort, Charset, UseTaglines, AutoSaveReplies,
	AutoSaveRead, StripSoftCR, BeepOnPers, UseLynxNav, UseScrollBars,
	BuildPersArea, MakeOldFlags, QuoteWrapCols, MaxLines
};

const int resource::defInt[] =
{
	1,	// PacketSort == by time
	0,	// LetterSort == by subject
#if defined (__MSDOS__) || defined (__EMX__)
	0,	// Charset == CP437
#else
	1,	// Charset == Latin-1
#endif
	1,	// UseTaglines == Yes
	0,	// AutoSaveReplies == No
	0,	// AutoSaveRead == No
	0,	// StripSoftCR == No
	0,	// BeepOnPers == No
	0,	// UseLynxNav == No
	1,	// UseScrollBars == Yes
	1,	// BuildPersArea == Yes
	0,	// MakeOldFlags == No
	78,	// QuoteWrapCols
	0	// MaxLines == disabled
};

resource::resource()
{
	char configFileName[256], inp;
	const char *envhome, *greeting =
		"\nWelcome to " MM_NAME " v%d.%d!\n\n"
		"A new or updated " RCNAME " has been written. "
		"If you continue now, " MM_NAME " will\nuse the default "
		"values for any new keywords. (Existing keywords have been "
		"\npreserved.) If you wish to edit your " RCNAME " first, "
		"say 'Y' at the prompt.\n\nEdit " RCNAME " now? (y/n) ";

	names = rc_names;
	intro = rc_intro;
	comments = rc_comments;
	configItemNum = startUpLen;

	for (int c = 0; c < noOfStrings; c++)
		resourceData[c] = 0;
	for (int c = noOfStrings; c < noOfResources; c++) {
		int d = c - noOfStrings;
		resourceInt[d] = defInt[d];
	}
	envhome = getenv("MMAIL");
	if (!envhome)
		envhome = getenv("HOME");
	if (!envhome)
		envhome = error.getOrigDir();

	set(homeDir, fixPath(envhome));
	sprintf(configFileName, "%.243s/" RCNAME, get(homeDir));

	initinit();
	homeInit();
	mmHomeInit();

	if (parseConfig(configFileName)) {
		newConfig(configFileName);
		printf(greeting, MM_MAJOR, MM_MINOR);
		inp = fgetc(stdin);

		if (toupper(inp) == 'Y') {
			char tmp[512];
			sprintf(tmp, "%.255s %.255s", get(editor),
				canonize(configFileName));
			mysystem(tmp);
			parseConfig(configFileName);
		}
	}

	if (!verifyPaths())
		fatalError("Unable to access data directories");

	mytmpnam(basedir);
	checkPath(basedir, false);
	subPath(WorkDir, "work");
	subPath(UpWorkDir, "upwork");
}

resource::~resource()
{
	clearDirectory(resourceData[WorkDir]);
	clearDirectory(resourceData[UpWorkDir]);
	mychdir("..");
	myrmdir(resourceData[WorkDir]);
	myrmdir(resourceData[UpWorkDir]);
	mychdir("..");
	myrmdir(basedir);
	for (int c = 0; c < noOfStrings; c++)
		delete[] resourceData[c];
}

// For consistency, no path should end in a slash:
const char *resource::fixPath(const char *path)
{
	static char tmp[256];
	char d = path[strlen(path) - 1];

	if ((d == '/') || (d == '\\')) {
		sprintf(tmp, "%.254s.", path); 
		return tmp;
	} else
		return path;
}

bool resource::checkPath(const char *onepath, bool show)
{
	if (mychdir(onepath)) {
		if (show)
			printf("Creating %s...\n", onepath);
		if (mymkdir(onepath))
			return false;
	}
	return true;
}

bool resource::verifyPaths()
{
	if (checkPath(resourceData[mmHomeDir], true))
	    if (checkPath(resourceData[PacketDir], true))
		if (checkPath(resourceData[ReplyDir], true))
		    if (checkPath(resourceData[SaveDir], true))
			return true;
	return false;
}

void resource::processOne(int c, const char *resValue)
{
	if (*resValue) {
		c = startUp[c];
		if (c < noOfStrings) {
			// Canonized for the benefit of the Win32 version:
			set(c, (c > noOfRaw) ? canonize(fixPath(resValue))
				: resValue);
			switch (c) {
			case homeDir:
				homeInit();
			case mmHomeDir:
				mmHomeInit();
			}
		} else {
			int x = 0;
			char r = toupper(*resValue);

			switch (c) {
			case PacketSort:
				x = (r == 'T');
				break;
			case LetterSort:
				switch (r) {
				case 'N':
					x = 1;
					break;
				case 'F':
					x = 2;
					break;
				case 'T':
					x = 3;
				}
				break;
			case Charset:
				x = (r == 'L');
				break;
			case QuoteWrapCols:
			case MaxLines:
				sscanf(resValue, "%d", &x);
				break;
			default:
				x = (r == 'Y');
			}

			set(c, x);
		}
	}
}

const char *resource::configLineOut(int x)
{
	static const char *pktopt[] = {"Name", "Time"},
		*lttopt[] = {"Subject", "Number", "From", "To"},
		*charopt[] = {"CP437", "Latin-1"},
		*stdopt[] = {"No", "Yes"};

	x = startUp[x];

	if ((x == MaxLines) || (x == QuoteWrapCols)) {
		static char value[8];
		sprintf(value, "%d", getInt(x));
		return value;
	} else
		return (x < noOfStrings) ? get(x) : ((x == PacketSort) ?
			pktopt : ((x == LetterSort) ? lttopt : ((x == Charset) ?
				charopt : stdopt)))[getInt(x)];
}

const char *resource::get(int ID) const
{
	return resourceData[ID];
}

int resource::getInt(int ID) const
{
	ID -= noOfStrings;
	return resourceInt[ID];
}

void resource::set(int ID, const char *newValue)
{
	delete[] resourceData[ID];
	resourceData[ID] = strdupplus(newValue);
}

void resource::set(int ID, int newValue)
{
	ID -= noOfStrings;
	resourceInt[ID] = newValue;
}

//------------------------------------------------------------------------
// The resource initializer functions
//------------------------------------------------------------------------

void resource::homeInit()
{
	char tmp[256];

	sprintf(tmp, "%.243s/mmail", resourceData[homeDir]);
	set(mmHomeDir, tmp);
}

void resource::mmEachInit(int index, const char *dirname)
{
	char tmp[256];

	sprintf(tmp, "%.243s/%s", resourceData[mmHomeDir], dirname);
	set(index, tmp);
}

void resource::subPath(int index, const char *dirname)
{
	char tmp[256];

	sprintf(tmp, "%.243s/%s", basedir, dirname);
	set(index, tmp);
	if (!checkPath(tmp, 0))
		fatalError("tmp Dir could not be created");
}

void resource::initinit()
{
	set(arjUncompressCommand, DEFUNARJ);
	set(zipUncompressCommand, DEFUNZIP);
	set(lhaUncompressCommand, DEFUNLHA);
	set(rarUncompressCommand, DEFUNRAR);
	set(unknownUncompressCommand, DEFUNNONE);
	set(arjCompressCommand, DEFARJ);
	set(zipCompressCommand, DEFZIP);
	set(lhaCompressCommand, DEFLHA);
	set(rarCompressCommand, DEFRAR);
	set(unknownCompressCommand, DEFNONE);

	set(UncompressCommand, DEFUNZIP);
	set(CompressCommand, DEFZIP);

	set(sigFile, "");
	set(UserName, "");
	set(InetAddr, "");
	set(QuoteHead, "-=> %f wrote to %t <=-");
	set(InetQuote, "On %d, %f wrote:");

	char *p = getenv("EDITOR");
	set(editor, (p ? p : DEFEDIT));
}

void resource::mmHomeInit()
{
	mmEachInit(PacketDir, "down");
	mmEachInit(ReplyDir, "up");
	mmEachInit(SaveDir, "save");
	mmEachInit(AddressFile, ADDRBOOK);
	mmEachInit(TaglineFile, "taglines");
	mmEachInit(ColorFile, "colors");
}
