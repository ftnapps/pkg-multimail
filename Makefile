#--------------------------
# MultiMail Makefile (top)
#--------------------------

include version

# General options (passed to mmail/Makefile and interfac/Makefile):

# With debug:
#OPTS = -g -Wall -pedantic

# Optimized, no debug:
OPTS = -O2 -Wall -pedantic

# Edited for Debian
DESTDIR =

# PREFIX is the base directory under which to install the binary and man 
# page; generally either /usr/local or /usr (or perhaps /opt...):

PREFIX = $(DESTDIR)/usr

# Delete command ("rm" or "del", as appropriate):

RM = rm -f

# The separator for multi-statement lines... some systems require ";",
# while others need "&&":

SEP = ;

# Any post-processing that needs doing:

POST =

#--------------------------------------------------------------
# Defaults are for Linux, with ncurses; also NetBSD, others:

# CURS_INC specifies the location of your curses header file. Broken
# brackets (<, >) should be preceded by backslashes. Quotes (") should
# be preceded by *three* backslashes:

CURS_INC = \<ncurses/curses.h\>

# CURS_DIR may also be necessary in some cases:

CURS_DIR = /usr/include/ncurses

# CURS_LIB specifies the directory where the curses libraries can be found,
# if they're not in the standard search path:

CURS_LIB = /usr/local/lib

# LIBS lists any "extra" libraries that need to be linked in:

LIBS = -lncurses
#LIBS = /usr/lib/libncurses.a

#--------------------------------------------------------------
# Solaris and others, with standard curses:

#CURS_INC = \<curses.h\>
#CURS_DIR = .
#CURS_LIB = .
#LIBS = -lcurses

#--------------------------------------------------------------
# With ncurses installed in the user's home directory:

# Example with quotes (relative pathnames start from ./interfac):
#CURS_INC = \\\"../../ncurses-4.2/include/curses.h\\\"
#CURS_DIR = .
#CURS_LIB = ../ncurses-4.2/lib

#--------------------------------------------------------------
# DJGPP (MSDOS), with PDCurses 2.4 beta:

#CURS_INC = \<curses.h\>
#CURS_DIR = /djgpp/contrib/pdcurs24
#CURS_LIB = .
#LIBS = ../contrib/pdcurs24/dos/pdcurses.a
#RM = del
# Optional; attach pmode stub:
#POST = $(RM) mm.exe $(SEP) strip mm $(SEP) copy /b \
#	..\pmode\pmodstub.exe+mm mm.exe $(SEP) $(RM) mm

#--------------------------------------------------------------
# EMX (OS/2), with GNU make, and PDCurses 2.3:
# Note: If you get "g++: Command not found", then type "set cxx=gcc"
# before running make.

# For some reason, it wants twice as many slashes:
#CURS_INC = \\\\\\"../../PDCurses-2.3/curses.h\\\\\\"
#CURS_DIR = /emx/PDCurses-2.3
#CURS_LIB = .
#LIBS = /emx/PDCurses-2.3/os2/pdcurses.a -Zcrtdll -lwrap
#RM = del /n
#SEP = &&
# Remove "emxbind -s mm" for a debug executable:
#POST = emxbind mm $(SEP) $(RM) mm $(SEP) emxbind -s mm

#--------------------------------------------------------------
# With XCurses (PDCurses 2.3) in my home directory:

#CURS_INC = \\\"../../../PDCurses-2.4b/curses.h\\\"
# Sneak some extra defines in through the back door:
#CURS_DIR = /home/wmcbrine/source/PDCurses-2.4b -DXCURSES -DHAVE_PROTO
#CURS_LIB = /home/wmcbrine/source/PDCurses-2.4b/pdcurses
#LIBS = -L/usr/X11R6/lib -lXCurses -lXaw -lXmu -lXt -lX11 -lSM -lICE -lXext

#--------------------------------------------------------------
#--------------------------------------------------------------

HELPDIR = $(PREFIX)/share/man/man1
DOCDIR = $(PREFIX)/share/doc/multimail

all:	mm

mm-main:
	cd mmail $(SEP) $(MAKE) MM_MAJOR="$(MM_MAJOR)" \
		MM_MINOR="$(MM_MINOR)" OPTS="$(OPTS)" mm-main $(SEP) cd ..

intrfc:
	cd interfac $(SEP) $(MAKE) MM_MAJOR="$(MM_MAJOR)" \
		MM_MINOR="$(MM_MINOR)" OPTS="$(OPTS) -I$(CURS_DIR)" \
		CURS_INC="$(CURS_INC)" intrfc $(SEP) cd ..

mm:	mm-main intrfc
	$(CXX) -o mm mmail/*.o interfac/*.o -L$(CURS_LIB) $(LIBS)
	$(POST)

dep:
	cd interfac $(SEP) $(MAKE) CURS_INC="$(CURS_INC)" dep $(SEP) cd ..
	cd mmail $(SEP) $(MAKE) dep $(SEP) cd ..

clean:
	cd interfac $(SEP) $(MAKE) RM="$(RM)" clean $(SEP) cd ..
	cd mmail $(SEP) $(MAKE) RM="$(RM)" clean $(SEP) cd ..
	$(RM) mm

install:
	install -d $(PREFIX)/bin $(HELPDIR) $(DOCDIR)
	install -c -s mm $(PREFIX)/bin
	install -c -m 644 mm.1 $(HELPDIR)
	$(RM) $(HELPDIR)/mmail.1
#	ln $(HELPDIR)/mm.1 $(HELPDIR)/mmail.1
	install -m 644 README $(DOCDIR)
	install -m 644 TODO $(DOCDIR)
	install -m 644 mm_faq.txt $(DOCDIR)
	install -d $(DOCDIR)/colors
	cp -a colors/* $(DOCDIR)/colors
