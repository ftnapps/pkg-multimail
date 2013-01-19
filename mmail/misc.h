/*
 * MultiMail offline mail reader
 * miscellaneous routines (global)

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef MISC_H
#define MISC_H

unsigned getshort(const unsigned char *);
unsigned long getlong(const unsigned char *);
unsigned long getblong(const unsigned char *);
void putshort(unsigned char *, unsigned);
void putlong(unsigned char *, unsigned long);
void putblong(unsigned char *, unsigned long);

char *cropesp(char *);
char *unspace(char *);
char *strdupplus(const char *);
const char *findBaseName(const char *);
const char *stripre(const char *);
const char *searchstr(const char *, const char *);
const char *fromAddr(const char *);
const char *fromName(const char *);
bool quoteIt(const char *);

void fatalError(const char *);	// actually in ../interfac/main.cc!

#endif
