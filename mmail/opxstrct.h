/* OPX (Silver Xpress) packet structures, in C
   Reverse engineered by William McBrine <wmcbrine@clark.net>
   Version 1.0 of this document, Oct. 27, 1999
   Placed in the Public Domain

   To the best of my knowledge, this format has never been publicly 
   documented until now, and has only previously been implemented in
   Santronics products. There are many parts of the structures that I
   haven't figured out yet, but what's here is enough on which to build
   basic reader functionality.

   Why use OPX? Because the doors are widely available, and the format
   offers more than standard QWK: long subject lines, Fido netmail, and
   Internet email. And in some cases (most notably with Wildcat boards),
   it's the only alternative to QWK.

   I focus here on BRDINFO.DAT, which contains the list of areas, along
   with things like sysop and BBS names; and MAIL.DAT, which holds the
   actual messages. (These correspond roughly to CONTROL.DAT and
   MESSAGES.DAT, respectively, in QWK.) Every OPX packet also has two
   index files, MAIL.FDX and MAIL.IDX; but to the extent I've decoded them
   thus far, they seem to contain data that's redundant with the .DAT
   files. MAIL.FDX is also used by the Silver Xpress reader to store its
   last-read markers. The new files list is NEWFILES.TXT, and a variety of
   bulletin files may be present.

   It appears that Silver Xpress was originally written in Borland Pascal,
   as the structures make extensive use of the BP style of strings. Here's
   a macro to help define the strings in C terms:
*/

#define pstring(y,x) struct {unsigned char len; char data[x];} y

/* Little-endian shorts (16-bit) and longs (32-bit), similar to the tWORD
   and tDWORD defintions in the Blue Wave specs, except that I don't
   bother drawing a distinction between signed and unsigned here, and I
   use only the portable (byte-by-byte) definition:
*/

typedef unsigned char pshort[2];
typedef unsigned char plong[4];
typedef unsigned char pbyte;

/* ### BRDINFO.DAT structures ### */

/* The Header */

typedef struct {
	char unknown1[15];
	pstring(doorid,20);	/* ID of the door that made this */
	pstring(bbsid,8);	/* Like BBSID in QWK; used for replies */
	pstring(bbsname,60);	/* BBS name */
	pstring(sysopname,50);	/* Sysop's name */
	char unknown2[81];
	pstring(zone,6);	/* Fidonet zone number of BBS, in ASCII */
	pstring(net,6);		/* Net number */
	pstring(node,6);	/* Node number */
	char unknown3[252];
	pstring(doorver,10);	/* Version number of door */
	char unknown4[2];
	pstring(phoneno,26);	/* Phone number of BBS */
	char unknown5[4];
	pstring(bbstype,123);	/* BBS software name and version -- not
				   the right length, I'm sure */
	pstring(username,35);	/* User's name */
	char unknown6[21];
	pshort numofareas;	/* Number of conferences */
	char unknown7[4];
	pbyte readerfiles;	/* Number of readerfiles */
} brdHeader;

/* This is followed by a 13-byte record (a Pascal string[12]) for each
   readerfile, as specified in brdHeader.readerfiles. Then there seems to
   be a single byte before the board records begin, but I don't know its
   function. For now, I've decided to ignore this and assume that the
   board record starts one byte earlier.
*/

/* Each Area */

typedef struct {
	char unknown1[3];
	pbyte conflow;		/* Low byte of conf. number (obsolete) */
	pstring(name,70);	/* Name of conference on BBS */
	pshort confnum;		/* Number of conference on BBS */
	char unknown2[3];
	pbyte attrib;		/* Area attributes (bitflags) */
	char unknown3[3];
	pbyte attrib2;		/* More area attributes */
	char unknown4;
} brdRec;

/* Some of the flags in attrib appear to be: */

#define OPX_NETMAIL	1
#define OPX_PRIVONLY	4
#define OPX_PUBONLY	8

/* And in attrib2: */

#define OPX_INTERNET	64
#define OPX_USENET	128

/* After all the areas comes some extra data which I'm ignoring for now.
   It appears to be a list of the message number, conference number, and
   attribute (?) for each message in the packet.
*/

/* ### MAIL.DAT structures ### */

/* Each message consists of the header, in a fixed format shown below, and
   the message text, whose length is specified in the header. Messages for
   all areas are concatenated.

   The header is based on Fidonet packet structures. Consequently, it
   deviates from the Borland Pascal style used elsewhere, and uses null-
   terminated (C-style) strings.

   The biggest oddity is the length field, which specifies the length of
   the entire message, including the header -- but minus 14 bytes. Since
   the header size is fixed (AFAIK), a more useful interpretation of this
   field is as the length of the text plus 0xBE bytes.
*/

/* Message Header */

typedef struct {
	pshort msgnum;		/* Message number on BBS */
	pshort confnum;		/* Conference number */
	pshort length;		/* Length of text + 0xBE */
	char unknown[8];
	char from[36];		/* From: (null-terminated string) */
	char to[36];		/* To: */
	char subject[72];	/* Subject: */
	char date[20];		/* Date in ASCII form -- don't use this, 
				   use the binary form below */
	pshort dest_zone;	/* Fido zone number of destination */
	pshort dest_node;	/* Node of dest. */
	pshort orig_node;	/* Node number of originating system */
	pshort orig_zone;	/* Zone of orig. */
	pshort orig_net;	/* Net of orig. */
	pshort dest_net;	/* Net of dest. */
	plong date_written;	/* Date in packed MS-DOS format */
	plong date_arrived;	/* Date the message arrived on the BBS, in
				   packed MS-DOS format */
	pshort reply;		/* Number of message that this replies to,
				   if applicable */
	pshort attr;		/* Attributes */
	pshort up;		/* Number of message that replies to this
				   one, if applicable */
} msgHead;

/* "attr" is a bitfield with the following values: */

#define OPX_PRIVATE	1	/* Private message */
#define OPX_CRASH	2	/* Fido crashmail */
#define OPX_READ	4	/* Read by addressee */
#define OPX_SENT	8
#define OPX_FILE	16
#define OPX_FWD		32
#define OPX_ORPHAN	64
#define OPX_KILL	128
#define OPX_LOCAL	256	/* SX reader sets this on every reply? */
#define OPX_HOLD	512
#define OPX_JUNKMAIL	1024
#define OPX_FRQ		2048
#define OPX_RRQ		4096
#define OPX_CPT		8192
#define OPX_SXFORM	16384
#define OPX_URQ		32768

/* "Packed MS-DOS format" dates are the format used by MS-DOS in some of
   its time/date routines, and in the FAT file system. They can cover
   dates from 1980 through 2107 -- better than a signed 32-bit time_t, but
   still limited. (The ASCII date field is deprecated.)

   bits 00-04 = day of month
   bits 05-08 = month
   bits 09-15 = year - 1980

   bits 16-20 = second / 2
   bits 21-26 = minute
   bits 27-31 = hour

   On some systems, the message text consists of lines delimited by LF
   characters, with CR characters optional (the reverse of Blue Wave).
   On others, the "soft CR" character (0x8D) is misused in place of hard
   CRs, with neither LF nor CR present. I'm not sure whether this covers
   all the possibile forms. Various Fido-style "hidden lines" may be
   present in the text, including "INETORIG <address>" on Internet email.
*/

/* ### REPLIES ### */

/* Reply packets are named in the form <BBSID>.REP. Inside each packet is
   one file per message, in a form similar but not identical to that used
   in MAIL.DAT, with a header followed by the text; along with a text file
   named <BBSID>.ID.
*/

/* Reply header */

typedef struct {
	char from[36];		/* From: (null-terminated string) */
	char to[36];		/* To: */
	char subject[72];	/* Subject: */
	char date[20];		/* Date in ASCII (not authoritative) */
	pshort dest_zone;	/* Fido zone number of destination -- set
                                   this to 0 for non-netmail */
	pshort dest_node;	/* Node of dest. */
	pshort orig_node;	/* Node number of originating system */
	pshort orig_zone;	/* Zone of orig. */
	pshort orig_net;	/* Net of orig. */
	pshort dest_net;	/* Net of dest. */
	plong date_written;	/* Date in packed MS-DOS format */
	plong date_arrived;	/* Same (meaningless here) */
	pshort reply;		/* Number of message that this replies to,
				   if applicable */
	pshort attr;		/* Attributes */
	pshort up;		/* Number of message that replies to this
				   one (meaningless here) */
} repHead;

/* You'll notice that some crucial information is missing from the header:
   the conference number! Instead, the conference number is encoded in the
   filename of the message. The names come in two forms. Messages which
   are not replies to existing messages are named as:

   !N<x>.<yyy>

   where <x> is the serial number of the message (this is ignored, AFAIK),
   in decimal, with no leading zeroes; and <yyy> is the destination area,
   in _base 36_, with leading zeroes if needed to pad it out to three
   characters. Messages which are replies are named:

   !R<x>.<yyy>

   In this case, <x> is the number of the message to which this is a
   reply. (This is redundant with the reply field in the header.) <x> is
   again a decimal number with no leading zeroes, and <yyy> is the same in
   both forms. Note that this system means there can only be one reply to
   a given message in a single reply packet, and the Silver Xpress reader
   enforces this. (The limit can be circumvented by not using the R form
   for subsequent replies.)

   After the header comes the message text. The length is determined by
   the file length, since there's no length field in the header. The text
   is CRLF-delimited paragraphs. I'm not sure whether the lines should be
   forced to wrap (like QWK) or not (like Blue Wave); when I tested this
   with the SX reader, it gave inconsistent results.

   Several Fido-style "hidden lines" (beginning with ctrl-A) may be
   present; the SX reader typically includes a PID and MSGID, and adds an
   INTL for netmail. I'm not sure how necessary it is for the reader to
   generate these. Internet email is indicated by an "INETDEST" kludge
   line (ctrl-A, "INETDEST", a space (no colon), and then the address).

   The text of messages generated by the SX reader ends in a zero byte,
   but this doesn't appear to be necessary.
*/

/* <BBSID>.ID */

/* A CRLF-delimited text file with seven lines. I don't really understand
   the purpose of this file, but I can give some description of it.

   Line 1: A textual representation of a boolean, either "TRUE" or
           "FALSE". All the replies I've seen so far say "FALSE". I don't
           know what it means.
   Line 2: Blank, in all the replies I've seen.
   Line 3: Version number of the SX reader, in ASCII.
   Line 4: Another boolean; always "TRUE" in my limited experience.
   Line 5: The date the packet was created, in packed MS-DOS format,
           treated as a signed number and written out as ASCII.
   Line 6: Another number; always 0 here.
   Line 7: Another number; always "-598939720" here, which I can't guess
           the meaning of. (It doesn't decode as a valid date/time.)

   So, lines 3 and 5 are the only ones I fully understand. Also, the file
   is given a weird time stamp (Dec 31, 2001). Perhaps these are security/
   validation features, but they don't seem to be required by the WINS
   version of the door. (Any others?)
*/
