/*
 * MultiMail offline mail reader
 * compress and decompress packets

 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#include "compress.h"

enum atype {A_ARJ, A_ZIP, A_LHA, A_RAR, A_UNKNOWN, A_UNEXIST};

atype lastAType = A_UNKNOWN;	// saves last atype for compress routine

atype getArchiveType(const char *fname)
{
	FILE *f;
	unsigned magic;
	atype tip = A_UNKNOWN;

	if (!(f = fopen(fname, "rb")))
		return A_UNEXIST;
	magic = fgetc(f) << 8;
	magic += fgetc(f);

	switch (magic) {
	case 0x60EA:
		tip = A_ARJ;
		break;
	case 0x504B:		// PK - check for ZIP
		if (3 == fgetc(f))
			if (4 == fgetc(f))
				tip = A_ZIP;
		break;
	case 0x5261:		// Ra - chech for RAR
		if ('r' == fgetc(f))
			if ('!' == fgetc(f))
				tip = A_RAR;
		break;
	default:		// can be LHA - check 3. and 4. bytes
		if ('-' == fgetc(f))
			if ('l' == fgetc(f))
				tip = A_LHA;
	// we should check another byte (l/z) but i'm lazy
	}
	fclose(f);

	return tip;
}

// clears the working directory and uncompresses the packet into it.
pktstatus uncompressFile(resource *ro, const char *fname,
	const char *todir, bool setAType)
{
	char command[255];
	const char *cm;
	atype at;

	clearDirectory(todir);
	at = getArchiveType(fname);
	if (at == A_UNEXIST)
		return PKT_UNFOUND;
	if (setAType)
		lastAType = at;

	switch (at) {
	case A_ARJ:
		cm = ro->get(arjUncompressCommand);
		break;
	case A_ZIP:
		cm = ro->get(zipUncompressCommand);
		break;
	case A_LHA:
		cm = ro->get(lhaUncompressCommand);
		break;
	case A_RAR:
		cm = ro->get(rarUncompressCommand);
		break;
	default:
		cm = ro->get(unknownUncompressCommand);
	}

	sprintf(command, "%s %s", cm, canonize(fname));

	return mysystem(command) ? UNCOMP_FAIL : PKT_OK;
}

int compressAddFile(resource *ro, const char *arcdir, const char *arcfile, 
			const char *addfname)
{
	char tmp[256], tmp2[256];
	const char *cm;
	int result;

	switch (lastAType) {
	case A_ARJ:
		cm = ro->get(arjCompressCommand);
		break;
	case A_ZIP:
		cm = ro->get(zipCompressCommand);
		break;
	case A_LHA:
		cm = ro->get(lhaCompressCommand);
		break;
	case A_RAR:
		cm = ro->get(rarCompressCommand);
		break;
	default:
		cm = ro->get(unknownCompressCommand);
	}

	sprintf(tmp2, "%s/%s", arcdir, arcfile);
	sprintf(tmp, "%s %s %s", cm, canonize(tmp2), addfname);
	result = mysystem(tmp);

	if (lastAType == A_LHA) {	// then the fixup
		strcpy(tmp2, arcfile);
		strtok(tmp2, ".");
		sprintf(tmp, "%s/%s.bak", arcdir, tmp2);
		remove(tmp);
	}
	return result;
}

// mainly for use with OMEN replies
const char *defExtent()
{
	switch (lastAType) {
	case A_ARJ:
		return "arj";
	case A_ZIP:
		return "zip";
	case A_LHA:
		return "lzh";
	case A_RAR:
		return "rar";
	default:
		return "";
	}
}
