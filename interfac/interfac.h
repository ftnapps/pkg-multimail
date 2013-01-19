/*
 * MultiMail offline mail reader
 * most class definitions for the interface

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef INTERFACE_H
#define INTERFACE_H

#define USE_SHADOWS	// "Shadowed" windows
#define MM_TOPHEADER "%s offline reader v%d.%d"

#include "../mmail/mmail.h"

#include <csignal>

extern "C" {
#include CURS_INC
}

#include "mmcolor.h"
#include "isoconv.h"

enum direction {UP, DOWN, PGUP, PGDN, HOME, END};
enum statetype {nostate, packetlist, arealist, letterlist, letter,
		letter_help, littlearealist, address, tagwin, ansiwin,
		ansi_help};
enum searchret {False, True, Abort};

enum {s_fulltext = 1, s_headers, s_arealist, s_pktlist};

#if defined (SIGWINCH) && !defined (XCURSES)
void sigwinchHandler(int);
#endif

#define TAGLINE_LENGTH 76

/* Include Keypad keys for PDCurses */

#ifdef __PDCURSES__
# define MM_PLUS '+': case PADPLUS
# define MM_MINUS '-': case PADMINUS
# define MM_ENTER '\r': case '\n': case PADENTER
# define MM_SLASH '/': case PADSLASH
#else
# define MM_PLUS '+'
# define MM_MINUS '-'
# define MM_ENTER '\r': case '\n'
# define MM_SLASH '/'
#endif

class tagline
{
 public:
 	tagline(const char * = 0);
 	tagline *next;
 	char text[TAGLINE_LENGTH + 1];
};

class ColorClass : public baseconfig
{
	static chtype allcolors[];
	static const char *col_names[], *col_intro[], *col_comments[];
	static const chtype mapped[];

	chtype colorparse(const char *);
	void processOne(int, const char *);
	const char *configLineOut(int);
	const char *findcol(chtype);
	const char *decompose(chtype);
 public:
	void Init();
};

class Win
{
 protected:
	WINDOW *win;
 public:
	Win(int, int, int, chtype);
	~Win();
	void Clear(chtype);
	void put(int, int, chtype);
	void put(int, int, char);
	void put(int, int, const chtype *, int = 0);
	int put(int, int, const char *, int = 0);
	void attrib(chtype);
	void horizline(int, int);
	void update();
	void delay_update();
	void wtouch();
	void wscroll(int);
	void cursor_on();
	void cursor_off();
	int keypressed();
	int inkey();
	void boxtitle(chtype, const char *, chtype);
};

class ShadowedWin : public Win
{
#ifdef USE_SHADOWS
	WINDOW *shadow;
#endif
 public:
	ShadowedWin(int, int, int, chtype, const char * = 0, chtype = 0);
	~ShadowedWin();
	void touch();
	int getstring(int, int, char *, int, chtype, chtype);
};

class InfoWin : public ShadowedWin
{
	Win *info;
 public:
	char *lineBuf;

	InfoWin(int, int, int, chtype, const char * = 0,
		chtype = 0, int = 3, int = 2);
	~InfoWin();
	void irefresh();
	void touch();
	void oneline(int, chtype);
	void iscrl(int);
};

class ListWindow
{
 private:
	bool lynxNav;	//use Lynx-like navigation?
	bool useScrollBars;
	int oldPos;	//position at last Draw()
	int oldActive;	//active at last Draw()
	int oldHigh;	//location of highlight bar at last Draw()

	void checkPos(int);
	chtype setHighlight(chtype);
 protected:
	InfoWin *list;
  	int list_max_y, list_max_x, top_offset;
	int position;	//the first element in the window
	int active;	//this is the highlited	

	chtype borderCol;

	void Draw();		//refreshes the window
	void DrawOne(int, chtype);
	void DrawAll();
	virtual int NumOfItems() = 0;
	virtual void oneLine(int) = 0;
	virtual searchret oneSearch(int, const char *, int) = 0;
	virtual bool extrakeys(int) = 0;
 public:
 	ListWindow();
	virtual ~ListWindow();
	void Move(direction);		//scrolloz
	void setActive(int);
	int getActive();
	searchret search(const char *, int);
	bool KeyHandle(int);
	virtual void Delete() = 0;
	virtual void Prev();
	virtual void Next();
};

class Welcome
{
 	ShadowedWin *window;
 public:
 	void MakeActive();
 	void Delete();
};

class Person
{
	void Init(const char *);
 public:
 	Person *next;
 	char name[45];
 	net_address netmail_addr;

	Person(const char * = 0, const char * = 0);
	Person(const char *, net_address &);
	void dump(FILE *);
};

class AddressBook : public ListWindow
{
	Person head, *curr, *highlighted, **people;
	const char *addfname;
	int NumOfPersons;
	bool NoEnter, inletter;

  	int NumOfItems();
	void oneLine(int);
	searchret oneSearch(int, const char *, int);
	bool extrakeys(int);

	void Add(const char *, net_address &);
	void GetAddress();
	int HeaderLine(ShadowedWin &, char *, int, int, int);
	int Edit(Person &);
	void NewAddress();
	void ChangeAddress();
 	void ReadFile();
 	void DestroyChain();
	friend int perscomp(const void *, const void *);
	void MakeChain();
	void ReChain();
 	void SetLetterThings();
	void WriteFile();
	void kill();
public:
	AddressBook();
	~AddressBook();
	void MakeActive(bool);
	void Delete();
	void Init();
};

class TaglineWindow : public ListWindow
{
	tagline head, *curr, *highlighted, **taglist;
	const char *tagname;
  	int NumOfTaglines;
	bool nodraw;

 	int NumOfItems();
	void oneLine(int);
	searchret oneSearch(int, const char *, int);
	bool extrakeys(int);

	void kill();
	bool ReadFile();
	void WriteFile(bool);
	void DestroyChain();
	void MakeChain();
	void RandomTagline();
 public:
 	TaglineWindow();
	~TaglineWindow();
	void MakeActive();
	void Delete();
 	void EnterTagline(const char * = 0);
	void EditTagline();
	void Init();
	const char *getCurrent();
};

class LittleAreaListWindow : public ListWindow
{
	int disp, areanum;

	int NumOfItems();
	void oneLine(int);
	searchret oneSearch(int, const char *, int);
	bool extrakeys(int);
	void Select();
 public:
	void init();
 	void MakeActive();
 	void Delete();
	int getArea();
};

class PacketListWindow : public ListWindow
{
	Welcome welcome;
	file_list *dirList, *packetList;
	const char *currDir, *origDir;
	time_t currTime;
	int noDirs, noFiles;
	bool sorttype;

	int NumOfItems();
	void oneLine(int);
	searchret oneSearch(int, const char *, int);
	bool extrakeys(int);

	void newList();
	void error();
	void gotoDir();
	void renamePacket();
	void killPacket();
 public:
	PacketListWindow();
	~PacketListWindow();
 	void MakeActive();
 	void Delete();
	void Select();
	pktstatus OpenPacket();
	bool back();
};

class AreaListWindow : public ListWindow
{
	char format[40], format2[40];
	bool mode;

 	int NumOfItems();
	void oneLine(int);
	searchret oneSearch(int, const char *, int);
	bool extrakeys(int);

 public:
	void ResetActive();
 	void MakeActive();
 	void Delete();
	void Select();
	void ReDraw();
	void FirstUnread();
	void Prev();
	void Next();
};

class LetterListWindow : public ListWindow
{
	char format[50];

	int NumOfItems();
	void oneLine(int);
	searchret oneSearch(int, const char *, int);
	bool extrakeys(int);

	void listSave();
 public:
	LetterListWindow();
	void ResetActive();
	void MakeActive();
	void Delete();
	void Select();
	void FirstUnread();
	void Prev();
	void Next();
};

class Line
{
 public:
	Line *next;
	char *text;
	unsigned length;
	bool quoted, hidden, tearline, tagline, origin, sigline;

 	Line(int);
	~Line();
};

class LetterWindow
{
	Win *headbar, *header, *text, *statbar;
	Line **linelist;
	char key, tagline1[TAGLINE_LENGTH + 1], To[30];
	int letter_in_chain;	//0 = no letter in chain
	int position;		//which row is the first in the text window
	int NumOfLines;
	int y;			//height of the window, set by MakeActive
	int replyto_area, stripCR, beepPers;
	bool rot13, hidden, lynxNav;
	net_address NM;

	void lineCount();
	void oneLine(int);
	void Move(int);
	char *netAdd(char *);
	int HeaderLine(ShadowedWin &, char *, int, int, int);
	int EnterHeader(char *, char *, char *, bool &);
	void QuoteText(FILE *);
	void DestroyChain();
	void setToFrom(char *, char *);
	void forward_header(FILE *, const char *, const char *,
		const char *, int, bool);
	void EditLetter(bool);
	bool SplitLetter(int = 0);
	long reconvert(const char *);
	void write_header_to_file(FILE *);
	void write_to_file(FILE *);
	void GetTagline();
	bool Previous();
	void NextDown();
	void MakeChain(int, bool = false);
 	void DrawHeader();
 	void DrawBody();
	void DrawStat();
	bool EditOriginal();
 public:
	LetterWindow();
 	void MakeActive(bool);
 	void Delete();
	bool Next();
 	void Draw(bool = false);
	void ReDraw();
	bool Save(int);
	void EnterLetter();
	void StatToggle(int);
	net_address &PickNetAddr();
	void set_Letter_Params(int, char);
	void set_Letter_Params(net_address &, const char *);
	void setPos(int);
	int getPos();
	searchret search(const char *);
	void SplitAll(int);
	void KeyHandle(int);
};

class HelpWindow
{
 	Win *menu;
	int midpos, endpos, base, items;

	void newHelpMenu(const char **, const char **, int);
	void h_packetlist();
	void h_arealist();
	void h_letterlist();
	void h_letter(bool);
 public:
	HelpWindow();
	void MakeActive();
	void Delete();
	void baseNext();
	void baseReset();
};

class AnsiLine
{
 private:
	AnsiLine *prev, *next;
 public:
	AnsiLine *getprev();
	AnsiLine *getnext(int = 0);
	chtype *text;
	unsigned length;
 	AnsiLine(int = 0, AnsiLine * = 0);
	~AnsiLine();
};

class AnsiWindow
{
	bool colorsused[64];
	char escparm[256];	//temp copy of ESC sequence parameters
	const char *title;
	file_header *fromFile;
	const unsigned char *source;
	Win *header, *text, *statbar, *animtext;
	AnsiLine *head, *curr, **linelist;
	int position;		//which row is the first in the window
	int NumOfLines;
	int x, y;		//dimensions of the window
	int cpx, cpy, lpy;	//ANSI positions
	int spx, spy;		//stored ANSI positions
	int ccf, ccb, cfl, cbr, crv;	//colors and attributes
	int oldcolorx, oldcolory;
	int baseline;		//base for positions in non-anim mode
	bool anim;		//animate mode?
	bool ansiAbort, sourceDel;
	chtype attrib;		//current attribute
	bool isLatin;

	void oneLine(int);
	void lineCount();
	void DrawBody();
	int getparm();
	void colreset();
	void colorset();
	const unsigned char *escfig(const unsigned char *);
	void posreset();
	void checkpos();
	void update(unsigned char);
	void ResetChain();
	void MakeChain(const unsigned char *);
	void DestroyChain();
	void animate();
	void statupdate(const char * = 0);
	void getFile();
	void Save();
 public:
	void set(const char *, const char *, bool);
	void setFile(file_header *, const char *, bool);
	void MakeActive();
	void Delete();
	void setPos(int);
	int getPos();
	searchret search(const char *);
	void KeyHandle(int);
};

class Interface
{
#if defined (__MSDOS__) || defined (__EMX__)
	Shell shell;
#endif
	PacketListWindow packets;
	AddressBook addresses;
	HelpWindow helpwindow;
	LittleAreaListWindow littleareas;

	Win *screen;
	ListWindow *currList;
 	statetype state, prevstate, searchstate;
	const char *searchItem, *cmdpktname;
	file_header *newFiles, **bulletins;
	int Key, searchmode, s_oldpos;
	bool unsaved_reply, any_read, addrparm, commandline, abortNow,
		dontSetAsRead, lynxNav;
#ifdef SIGWINCH
	bool resized;

	void sigwinch();
#endif
	void init_colors();
	void oldstate(statetype);
	void helpreset(statetype);
	void newstate(statetype);
	void alive();
	void screen_init();
	const char *pkterrmsg(pktstatus);
	void newpacket();
	void create_reply_packet();
	void save_read();
	int ansiCommon();
	void searchNext();
	void searchSet();
	void KeyHandle();
 public:
	ColorClass colorlist;
	AreaListWindow areas;
	LetterListWindow letters;
	LetterWindow letterwindow;
	TaglineWindow taglines;
	AnsiWindow ansiwindow;

 	Interface();
	~Interface();
	void init();
	void main();
 	int WarningWindow(const char *, const char ** = 0, int = 2);
	int savePrompt(const char *, char *);
	void nonFatalError(const char *);
	void changestate(statetype);
	void redraw();
 	bool select();
 	bool back();		//returns true if we have to quit
	statetype active();
	statetype prevactive();
#ifdef SIGWINCH
	void setResized();
#endif
	void addressbook(bool = false);
	bool Tagwin();
	int ansiLoop(const char *, const char *, bool);
	int ansiFile(file_header *, const char *, bool);
	int areaMenu();
	void kill_letter();
	void setUnsaved();
	void setUnsavedNoAuto();
	void setAnyRead();
	void setKey(int);
	bool dontRead();
	bool fromCommandLine(const char *);
};

extern mmail mm;
extern Interface *interface;

#if defined (__PDCURSES__) && !defined (XCURSES)
extern "C" {
int PDC_get_cursor_mode();
int PDC_set_cursor_mode(int, int);
}
extern int curs_start, curs_end;
#endif

#endif
