/*
 * MultiMail offline mail reader
 * error-reporting class

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

class ErrorType
{
	char origdir[256];
 public:
	ErrorType();
	~ErrorType();
	const char *getOrigDir();
};

extern ErrorType error;
