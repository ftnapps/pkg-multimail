/*
 * MultiMail offline mail reader
 * color pairs #define'd here

 Copyright (c) 1996 John Zero <john@graphisoft.hu>
 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef MMCOLOR_H
#define MMCOLOR_H

#define COL(f, b) COLOR_PAIR(((f) << 3) + (b))

// In PDCurses, the A_REVERSE attribute resets the colors, so I avoid
// using it here:

#ifdef __PDCURSES__
# define REVERSE(f, b) (COL((b), (f)))
#else
# define REVERSE(f, b) ((COL((f), (b))) | (A_REVERSE))
#endif

#define C_ANSIBACK	COL(COLOR_WHITE, COLOR_BLACK)

#define C_SBACK		ColorArray[0]	//Start screen/backgnd
#define C_SBORDER	ColorArray[1]	//Start/bdr
#define C_SSEPBOTT	ColorArray[2]	//Start screen/bottom 
#define C_HELP1		ColorArray[3]	//Help desc.
#define C_HELP2		ColorArray[4]	//Help keys
#define C_HELP3		ColorArray[5]	//Help 2 bdr
#define C_HELP4		ColorArray[6]	//Help 2 text
#define C_WBORDER	ColorArray[7]	//Welcome border
#define C_WELCOME1	ColorArray[8]	//Welcome prog name
#define C_WELCOME2	ColorArray[9]	//Welcome auth names
#define C_ADDR1		ColorArray[10]	//Add. backgnd
#define C_ADDR2		ColorArray[11]	//Add. headers
#define C_ADDR3		ColorArray[12]	//Address book/text
#define C_WTEXT		ColorArray[13]	//Warn/text
#define C_WTEXTHI	ColorArray[14]	//Warn/hilight
#define C_LTEXT		ColorArray[15]	//Letter/text
#define C_LQTEXT	ColorArray[16]	//Letter/quoted text
#define C_LTAGLINE	ColorArray[17]	//Letter/tagline
#define C_LTEAR		ColorArray[18]	//Letter/tear
#define C_LHIDDEN	ColorArray[19]	//Letter/hidden
#define C_LORIGIN	ColorArray[20]	//Letter/origin
#define C_LBOTTSTAT	ColorArray[21]	//Letter/bottom statline
#define C_LHEADTEXT	ColorArray[22]	//Letter/header text
#define C_LHMSGNUM	ColorArray[23]	//msgnum 
#define C_LHFROM	ColorArray[24]	//from
#define C_LHTO		ColorArray[25]	//to
#define C_LHSUBJ	ColorArray[26]	//subject
#define C_LHDATE	ColorArray[27]	//date
#define C_LHFLAGSHI	ColorArray[28]	//flags high
#define C_LHFLAGS	ColorArray[29]	//flags
#define C_PBBACK	ColorArray[30]	//Packet/header 
#define C_PHEADTEXT	ColorArray[31]	//line text
#define C_PLINES	ColorArray[32]	//Packet/lines
#define C_LALBTEXT	ColorArray[33]	//Little area 
#define C_LALLINES	ColorArray[34]	//line text
#define C_ALREPLINE	ColorArray[35]	//Area list/reply area
#define C_ALPACKETLINE	ColorArray[36]	//Area list/normal
#define C_ALINFOTEXT	ColorArray[37]	//info win
#define C_ALINFOTEXT2	ColorArray[38]	//filled text
#define C_ALBTEXT	ColorArray[39]	//border text
#define C_ALBORDER	ColorArray[40]	//border
#define C_ALHEADTEXT	ColorArray[41]	//header text
#define C_LETEXT	ColorArray[42]	//Letter text
#define C_LEGET1	ColorArray[43]	//Letter/enter get1
#define C_LEGET2	ColorArray[44]	//get2
#define C_LLSAVEBORD	ColorArray[45]	//Letter/save border
#define C_LLSAVETEXT	ColorArray[46]	//Letter/save
#define C_LLSAVEGET	ColorArray[47]	//get
#define C_LISTWIN	ColorArray[48]	//Letter list/top text1
#define C_LLPERSONAL	ColorArray[49]	//Letter list/personal
#define C_LLBBORD	ColorArray[50]	//Letter list
#define C_LLTOPTEXT1	ColorArray[51]	//top text1
#define C_LLTOPTEXT2	ColorArray[52]	//areaname
#define C_LLHEAD	ColorArray[53]	//headers
#define C_TBBACK	ColorArray[54]	//Tagline
#define C_TTEXT 	ColorArray[55]	//Tagline/text
#define C_TKEYSTEXT	ColorArray[56]	//key select
#define C_TENTER 	ColorArray[57]	//Tagline/enter
#define C_TENTERGET	ColorArray[58]	//enter get
#define C_TLINES	ColorArray[59]	//lines
#define C_SHADOW	ColorArray[60]	//All black!

chtype emph(chtype);
chtype noemph(chtype);

extern const chtype *ColorArray;

#endif
