#define STRingize(x) #x
#define STR(x) STRingize(x)

#define MM_NAME "MultiMail"
#define MM_VERNUM STR(MM_MAJOR) "." STR(MM_MINOR)
#define MM_TOPHEADER MM_NAME "/%.16s v" MM_VERNUM

#define USE_SHADOWS     // "Shadowed" windows
#define VANITY_PLATE    // Author info -- undefine for longer packet list

/* With the default tar/gz compress command, leave this on. It causes the
   entire file to be rearchived when using that format, instead of just
   the .red file. If you replace the tar/gz command with something 
   smarter (e.g., via a script), you can disable the kludge.
*/
#define TAR_KLUDGE

#define HAS_BOOL
#define HAS_OFFT
#define HAS_HOME

/* ----- Platform-specific defines ----- */

/* This could be improved on. Currently, it's only written to deal with
   GCC, DJGPP, EMX, RSX/NT, BCC 5.5, Turbo C++ 3.0, MinGW and Watcom.
*/

/* HAS_UNAME determines whether the "System" part of the tearline is taken
   from the uname() function, or is hardwired. Turbo/Borland C++ and
   Watcom don't have it, and it's broken in RSX/NT.
*/
#if !defined(__RSXNT__) && !defined(__TURBOC__) && !defined(__MINGW32__) && !defined(__WATCOMC__)
# define HAS_UNAME
#endif

/* MS-DOS and DOS-like systems. */

#if defined(__MSDOS__) || defined(__WIN32__) || defined(__OS2__)
# define DOSCHARS
# define DOSNAMES
# define USE_SHELL
# undef HAS_HOME

/* Assume the use of PDCurses on these platforms (can't check it
   explicitly until curses.h is included), and so:

   In PDCurses, the A_REVERSE attribute resets the colors, so I avoid
   using it.
*/
# define NOREVERSE

/* Some characters are unprintable on standard Unix terminals, even with
   A_ALTCHARSET set. But PDCurses will handle them.
*/
# define ALLCHARSOK

/* For the ugly cursor-toggling routines */

# ifndef __WIN32__
#  define PDCURSKLUDGE
# endif

#else

/* Not a DOS-like system -- enable "Transparency" keyword */

# define HAS_TRANS

#endif

/* One remaining PDCurses platform not covered by the above */

#ifdef XCURSES
# define NOREVERSE
#endif

/* Also, see the NCURSES_SIGWINCH definition in interfac/interfac.h -- it
   should be fine as is, but may need manual adjustment in some cases.
*/

/* I use strcasecmp() and strncasecmp() throughout, but some systems call
   these functions stricmp() and strincmp().
*/
#if defined(__EMX__) || defined(__TURBOC__) || defined(__WATCOMC__)
# define USE_STRICMP
#endif

/* unistd.h is the POSIX header file. Borland/Turbo C doesn't have it.
   The sleep() function is also defined there.
*/
#if !defined(__TURBOC__) && !defined(__MINGW32__) && !defined(__WATCOMC__)
# define HAS_UNISTD
# define HAS_SLEEP
#endif

/* For the 16-bit MS-DOS version compiled with Turbo C++ 3.0, I've added
   the SPAWNO library by Ralf Brown to get more memory when shelling.
   Also, this version lacks the "bool" and "off_t" types. (Check should be
   more restrictive.)
*/
#if defined(__TURBOC__) && defined(__MSDOS__)
# define USE_SPAWNO
# define LIMIT_MEM
# define MAXBLOCK 0x0FFE0L
# undef HAS_BOOL
# undef HAS_OFFT
#else
# define MAXBLOCK 0x07FFFFFE0L
#endif

/* Watcom (all DOSish platforms) and Turbo C++ need an extra call to get
   the drive letter changed, when changing directories.
*/
#if defined(__WATCOMC__) || (defined(__TURBOC__) && defined(__MSDOS__))
# define USE_SETDISK
#endif

/* Borland C++ 5.5 barfs on time_t = 0, which appears as the timestamp of
   the top-level directory. Also, utime(), though implemented, doesn't work
   right.
*/
#if defined(__TURBOC__) && defined(__WIN32__)
# define TIMEKLUDGE
# define USE_SETFTIME
#endif

/* Some lines in the code serve no purpose but to supress the GCC warning
   "might be used uninitialized in this function". Borland C++ 5.5, on the
   other hand, complains "is assigned a value that is never used" if I
   leave these lines _in_.
*/
#ifndef __TURBOC__
# define BOGUS_WARNING
#endif

/* In Borland/Turbo C++ and in DJGPP, using findfirst()/findnext() is
   faster than using readdir()/stat(). Probably in Watcom and MinGW too, 
   but they have a different interface.
*/
#if defined(__MSDOS__) || defined(__TURBOC__)
# define USE_DIRH
# define USE_FINDFIRST
#endif

/* ----- End of platform-specific defines ----- */

#ifndef HAS_BOOL
typedef unsigned char bool;
# define true 1
# define false 0
#endif

#ifndef HAS_OFFT
typedef long off_t;
#endif
