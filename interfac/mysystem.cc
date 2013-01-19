/*
 * MultiMail offline mail reader
 * some low-level routines common to both sides

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

/* Most non-ANSI, non-curses stuff is here. */

#include "interfac.h"
#include "error.h"

extern "C" {
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#ifdef __EMX__
int _chdir2(const char *);
char *_getcwd2(char *, int);
#endif
}

static DIR *Dir;
static dirent *entry;

void fatalError(const char *description);

char *myfgets(char *s, size_t size, FILE *stream)
{
	char *end = 0;

	if (!feof(stream) && fgets(s, size, stream)) {

		end = s + strlen(s) - 1;

		// Skip any leftovers:
		if (*end != '\n')
			while (!feof(stream) && (fgetc(stream) != '\n'));

	}
	return end;
}

int mysystem(const char *cmd)
{
	if (interface) {
		if (!isendwin())
			endwin();
#if defined (__PDCURSES__) && !defined (XCURSES)
		// Restore original cursor
		PDC_set_cursor_mode(curs_start, curs_end);
#endif
	}

	int result = system(cmd);

	// Non-zero result = error; pause so it can (maybe) be read
	if (result)
		sleep(2);

	if (interface) {
		keypad(stdscr, TRUE);
#if defined (__PDCURSES__) && defined (__RSXNT__)
		typeahead(-1);
#endif
	}

	return result;
}

void mytmpnam(char *name)
{
/* EMX doesn't return an absolute pathname from tmpnam(), so we create one
   ourselves. Otherwise, use the system's version, and make sure it hasn't
   run out of names.
*/
#ifdef __EMX__
	const char *tmp = getenv("TEMP");

	if (!tmp) {
		tmp = getenv("TMP");
		if (!tmp)
			tmp = mm.resourceObject->get(mmHomeDir);
	}
	sprintf(name, "%s/%s", tmp, tmpnam(0));
#else
	if (!tmpnam(name))
		fatalError("Out of temporary filenames");
#endif
}

void edit(const char *reply_filename)
{
        char command[512];

        sprintf(command, "%.255s %.255s", mm.resourceObject->get(editor),
                canonize(reply_filename));
        mysystem(command);
}

int mychdir(const char *pathname)
{
	return
#ifdef __EMX__
		_chdir2(pathname);
#else
		chdir(pathname);
#endif
}

int mymkdir(const char *pathname)
{
	return mkdir(pathname, S_IRWXU);
}

void myrmdir(const char *pathname)
{
	rmdir(pathname);
}

void mygetcwd(char *pathname)
{
#ifdef __EMX__
	_getcwd2(pathname, 255);
#else
	getcwd(pathname, 255);
#endif
}

bool readable(const char *filename)
{
	return !access(filename, R_OK);
}

bool writeable(const char *filename)
{
	return !access(filename, R_OK | W_OK);
}

// system name -- results of uname()
const char *sysname()
{
	static struct utsname buf;

	if (!buf.sysname[0])
#if defined(__WIN32__) && defined(__RSXNT__)
		// uname() returns "MS-DOS" in RSXNT, so hard-wire it here
		strcpy(buf.sysname, "Win32");
#else
		uname(&buf);
#endif
	return buf.sysname;
}

bool myopendir(const char *dirname)
{
	return (Dir = opendir(dirname)) ? !mychdir(dirname) : false;
}

const char *myreaddir()
{
	entry = readdir(Dir);
	if (!entry) {
		closedir(Dir);
		return 0;
	} else
		return entry->d_name;
}

void clearDirectory(const char *DirName)
{
	const char *fname;

	if (myopendir(DirName))
		while ((fname = myreaddir()))
			if (fname[0] != '.')
				remove(fname);
}

#if defined (__MSDOS__) || defined (__EMX__)

/* Convert pathnames to "canonical" form (change slashes to backslashes).
   The "nospace" stuff leaves any parameters unconverted.
   Don't call this twice in a row without first copying the result! D'oh!
*/

const char *canonize(const char *sinner)
{
	static char saint[256];
	int i;
	bool nospace = true;

	for (i = 0; sinner[i]; i++) {
		saint[i] = (nospace && (sinner[i] == '/')) ?
			'\\' : sinner[i];
		if (nospace && (saint[i] == ' '))
			nospace = false;
	}
	saint[i] = '\0';
	return saint;
}

/* Command shell routine -- currently only used in the DOSish ports */

Shell::Shell()
{
	const char *oldprompt = getenv("PROMPT");
	if (!oldprompt)
		oldprompt = "$p$g";

	int len = strlen(oldprompt) + 13;
	prompt = new char[len];

	sprintf(prompt, "PROMPT=%s[MM] ", oldprompt);
	putenv(prompt);
}

Shell::~Shell()
{
	delete[] prompt;
}

void Shell::out()
{
	mychdir(error.getOrigDir());
	touchwin(stdscr);
	refresh();
	mysystem(getenv("COMSPEC"));

	interface->redraw();
}

#endif

bool mystat::init(const char *fname)
{
	struct stat fileStat;
	bool retval = !stat(fname, &fileStat);
	size = retval ? fileStat.st_size : -1;
	date = retval ? fileStat.st_mtime : -1;
	isdir = !(!S_ISDIR(fileStat.st_mode));
	return retval;
}
