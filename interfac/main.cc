/*
 * MultiMail offline mail reader
 * main, error

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "error.h"
#include "interfac.h"

#include <new>

Interface *interface = 0;
const chtype *ColorArray = 0;
ErrorType error;
mmail mm;
#ifdef __PDCURSES__
int curs_start, curs_end;
#endif

void memError();
void fatalError(const char *description);

ErrorType::ErrorType()
{
	set_new_handler(memError);
	mygetcwd(origdir);
}

ErrorType::~ErrorType()
{
	mychdir(origdir);
}

const char *ErrorType::getOrigDir()
{
	return origdir;
}

#if defined (SIGWINCH) && !defined (XCURSES)
void sigwinchHandler(int sig)
{
	if (sig == SIGWINCH)
		interface->setResized();
	signal(SIGWINCH, sigwinchHandler);
}
#endif

void fatalError(const char *description)
{
	delete interface;
	fprintf(stderr, "\n\n%s\n\n", description);
	exit(EXIT_FAILURE);
};

void memError()
{
	fatalError("Out of memory");
}

int main(int argc, char **argv)
{
	interface = new Interface();
	interface->init();
	if (argc > 1)
		for (int i = 1; (i < argc) &&
			interface->fromCommandLine(argv[i]); i++);
	else
		interface->main();
	delete interface;
	return EXIT_SUCCESS;
}
