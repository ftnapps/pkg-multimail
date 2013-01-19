/*
 * MultiMail offline mail reader
 * file_header and file_list

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "mmail.h"

// --------------------------------------------------------------------------
// file_header methods
// --------------------------------------------------------------------------
file_header::file_header(const char *nameA, time_t dateA, off_t sizeA)
{
	name = strdupplus(nameA);
	date = dateA;
	size = sizeA;
	next = 0;
}

file_header::~file_header()
{
	delete[] name;
}

const char *file_header::getName() const
{
	return name;
}

time_t file_header::getDate() const
{
	return date;
}

off_t file_header::getSize() const
{
	return size;
}

// ------------------------------------------------------------------------
// file_list methods
// ------------------------------------------------------------------------

file_list::file_list(const char *FileDir, bool sorttypeA, bool dirlistA)
{
	sorttype = sorttypeA;
	dirlist = dirlistA;

	dirlen = strlen(FileDir);
	DirName = new char[dirlen + 81];
	strcpy(DirName, FileDir);

	initFiles();
}

file_list::~file_list()
{
	while (noOfFiles)
		delete files[--noOfFiles];
	delete[] files;
	delete[] DirName;
}

void file_list::initFiles()
{
	if (!myopendir(DirName))
		fatalError("There is no Packet Dir!");

	noOfFiles = 0;

	file_header head("", 0, 0);
	file_header *filept = &head;

	const char *fname;
	mystat st;

	while ((fname = myreaddir()))
	    if (st.init(fname))
		if (dirlist ^ !st.isdir)
		    if (strcmp(fname, "."))
			if (readable(fname)) {
				filept->next = new file_header(fname,
					st.date, st.size);
				filept = filept->next;
				noOfFiles++;
			}

	files = new file_header *[noOfFiles];
	int c = 0;
	filept = head.next;
	while (filept) {
		files[c++] = filept;
		filept = filept->next;
	}
	sort();
}

void file_list::sort()
{
	if (noOfFiles > 1)
		qsort(files, noOfFiles, sizeof(file_header *), sorttype ?
			ftimecomp : fnamecomp);
}

void file_list::resort()
{
	sorttype = !sorttype;
	sort();
}

int fnamecomp(const void *a, const void *b)
{
	int d;

	const char *p = (*((file_header **) a))->getName();
	const char *q = (*((file_header **) b))->getName();

	d = strcasecmp(p, q);
	if (!d)
		d = strcmp(q, p);

	return d;
}

int ftimecomp(const void *a, const void *b)
{
	return (*((file_header **) b))->getDate() -
		(*((file_header **) a))->getDate();
}

const char *file_list::getDirName() const
{
	DirName[dirlen] = '\0';
	return DirName;
}

int file_list::getNoOfFiles() const
{
	return noOfFiles;
}

void file_list::gotoFile(int fileNo)
{
	if (fileNo < noOfFiles)
		activeFile = fileNo;
}

const char *file_list::changeDir(const char *newpath)
{
	static char newdir[256];

	if (dirlist) {
		if (!newpath)
			newpath = getName();
		mychdir(DirName);
		mychdir(newpath);
		mygetcwd(newdir);

		char *p = newdir + strlen(newdir) - 1;
		if ((*p == '\\') || (*p == '/')) {
			p[1] = '.';
			p[2] = '\0';
		}
	}
	return newdir;
}

int file_list::changeName(const char *newname)
{
	mychdir(DirName);
	return rename(getName(), newname);
}

const char *file_list::getName() const
{
	return files[activeFile]->getName();
}

time_t file_list::getDate() const
{
	return files[activeFile]->getDate();
}

off_t file_list::getSize() const
{
	return files[activeFile]->getSize();
}

const char *file_list::getNext(const char *fname)
{
	int c, len;
	const char *p, *q;
	bool isExt;

	if (fname) {
		isExt = (*fname == '.');

		for (c = activeFile + 1; c < noOfFiles; c++) {
			q = files[c]->getName();

			if (isExt) {
				len = strlen(q);
				if (len > 5) {
				p = q + len - 4;
					if (!strcasecmp(p, fname)) {
						activeFile = c;
						return q;
					}
				}
			} else
				if (!strncasecmp(q, fname, strlen(fname))) {
					activeFile = c;
					return q;
				}
        	}
	}
        return 0;
}

file_header *file_list::getNextF(const char *fname)
{
	return getNext(fname) ? files[activeFile] : 0;
}

const char *file_list::exists(const char *fname)
{
	gotoFile(-1);
	return getNext(fname);
}

file_header *file_list::existsF(const char *fname)
{
	return exists(fname) ? files[activeFile] : 0;
}

void file_list::addItem(file_header **list, const char *q, int &filecount)
{
	file_header *p;
	int x;

	gotoFile(-1);
	while ((p = getNextF(q))) {
		for (x = 0; x < filecount; x++)
			if (list[x] == p)
				break;
		if (x == filecount) {
			list[x] = p;
			filecount++;
		}
	}
}

const char *file_list::expandName(const char *fname)
{
	sprintf(DirName + dirlen, "/%.79s", fname);
	return DirName;
}

FILE *file_list::ftryopen(const char *fname, const char *mode)
{
	const char *p = exists(fname);
	return p ? fopen(expandName(p), mode) : 0;
}

void file_list::kill()
{
	remove(expandName(getName()));

	delete files[activeFile];
	noOfFiles--;
	for (int i = activeFile; i < noOfFiles; i++)
		files[i] = files[i + 1];
}
