/*
 * MultiMail offline mail reader
 * miscellaneous routines (global)

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "mmail.h"

// get a little-endian short, return an int
unsigned getshort(const unsigned char *x)
{
	return ((unsigned) x[1] << 8) + (unsigned) x[0];
}

// get a little-endian long
unsigned long getlong(const unsigned char *x)
{
	return ((unsigned long) x[3] << 24) + ((unsigned long) x[2] << 16) +
		((unsigned long) x[1] << 8) + (unsigned long) x[0];
}

// get a big-endian long
unsigned long getblong(const unsigned char *x)
{
	return ((unsigned long) x[0] << 24) + ((unsigned long) x[1] << 16) +
		((unsigned long) x[2] << 8) + (unsigned long) x[3];
}

// put an int into a little-endian short
void putshort(unsigned char *dest, unsigned source)
{
	dest[0] = source & 0xff;
	dest[1] = (source & 0xff00) >> 8;
}

// put a long into a little-endian long
void putlong(unsigned char *dest, unsigned long source)
{
	dest[0] = source & 0xff;
	dest[1] = (source & 0xff00) >> 8;
	dest[2] = (source & 0xff0000) >> 16;
	dest[3] = (source & 0xff000000) >> 24;
}

// put a long into a big-endian long
void putblong(unsigned char *dest, unsigned long source)
{
	dest[0] = (source & 0xff000000) >> 24;
	dest[1] = (source & 0xff0000) >> 16;
	dest[2] = (source & 0xff00) >> 8;
	dest[3] = source & 0xff;
}

// takes off the spaces from the end of a string
char *cropesp(char *st)
{
	char *p;

	for (p = st + strlen(st) - 1; (p > st) && (*p == ' '); p--);
	p[1] = '\0';
	return st;
}

// converts spaces to underline characters
char *unspace(char *source)
{
	for (unsigned c = 0; c < strlen(source); c++)
		if (source[c] == ' ')
			source[c] = '_';
	return source;
}

char *strdupplus(const char *original)
{
	char *tmp;

	if (original) {
		tmp = new char[strlen(original) + 1];
		strcpy(tmp, original);
	} else
		tmp = 0;

	return tmp;
};

const char *findBaseName(const char *fileName)
{
	int c, d;
	static char tmp[13];

	for (c = 0; (fileName[c] != '.') && (fileName[c]); c++);

	for (d = 0; d < c; d++)
		tmp[d] = tolower(fileName[d]);
	tmp[d] = '\0';

	return tmp;
};

const char *stripre(const char *subject)
{
	while (!strncasecmp(subject, "re: ", 4))
		subject += 4;
	return subject;
}

// basically the equivalent of "strcasestr()", if there were such a thing
const char *searchstr(const char *source, const char *item)
{
	const char *s;
	char first[3];
	int ilen = strlen(item) - 1;
	bool found = false;

	first[0] = tolower(*item);
	first[1] = toupper(*item);
	first[2] = '\0';

	item++;

	do {
		s = strpbrk(source, first);
		if (s) {
			source = s + 1;
			found = !strncasecmp(source, item, ilen);
		}
	} while (s && !found && *source);

	return found ? s : 0;
}

// Find the address in "Foo <foo@bar.baz>" or "foo@bar.baz (Foo)"
const char *fromAddr(const char *source)
{
	static char tmp[100];
	const char *end = 0, *index = source;

	while (*index) {
		if (*index == '"')
			do
				index++;
			while (*index && (*index != '"'));
		if ((*index == '<') || (*index == '('))
			break;
		if (*index)
			index++;
	}

	if (*index == '<') {
		index++;
		end = strchr(index, '>');
	} else {
		end = index - (*index == '(');
		index = source;
	}

	if (end) {
		int len = end - index;
		if (len > 99)
			len = 99;
		strncpy(tmp, index, len);
		tmp[len] = '\0';
		return tmp;
	}
	return source;
}

// Find the name portion of the address
const char *fromName(const char *source)
{
	static char tmp[100];
	const char *end = 0, *fr = source;

	while (*fr) {
		if (*fr == '"') {
			fr++;
			end = strchr(fr, '"');
			break;
		}
		if (*fr == '(') {
			fr++;
			end = strchr(fr, ')');
			break;
		}
		if ((*fr == '<') && (fr > source)) {
			end = fr - 1;
			fr = source;
			break;
		}
		if (*fr)
			fr++;
	}

	if (end) {
		int len = end - fr;
		if (len > 99)
			len = 99;
		strncpy(tmp, fr, len);
		tmp[len] = '\0';
		return tmp;
	}
	return source;
}

// Should a name be quoted in an address?
bool quoteIt(const char *s)
{
	bool flag = false;

	while (*s) {
		int c = toupper(*s++);
		if (!(((c >= 'A') && (c <= 'Z')) || (c == ' '))) {
			flag = true;
			break;
		}
	}
	return flag;
}
