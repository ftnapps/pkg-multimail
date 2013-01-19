/*
 * MultiMail offline mail reader
 * protos for mysystem.cc

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net> 

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef MYSYSTEM_H
#define MYSYSTEM_H

#include <ctime>

extern "C" {
#include <sys/types.h>
}

char *myfgets(char *, size_t, FILE *);
int mysystem(const char *);
void mytmpnam(char *);
void edit(const char *);
int mychdir(const char *);
int mymkdir(const char *);
void myrmdir(const char *);
void mygetcwd(char *);
bool readable(const char *);
bool writeable(const char *);
const char *sysname();
bool myopendir(const char *);
const char *myreaddir();
void clearDirectory(const char *);

#if defined (__MSDOS__) || defined (__EMX__)
const char *canonize(const char *);

class Shell
{
	char *prompt;
 public:
	Shell();
	~Shell();
	void out();
};

#else
# define canonize(x) x
#endif

class mystat
{
 public:
	off_t size;
	time_t date;
	bool isdir;

	bool init(const char *);
};

// Some of the functions normally used by MultiMail don't exist in EMX,
// but are available under other names:

#ifdef __EMX__
# define strcasecmp stricmp
# define strncasecmp strnicmp
#endif

#endif
