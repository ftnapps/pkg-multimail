#--------------------------
# MultiMail Makefile (top)
#--------------------------

include version

# General options (passed to mmail/Makefile and interfac/Makefile):

# With debug:
#OPTS = -g -Wall -Wextra -pedantic -Wno-deprecated -Wno-char-subscripts

# Optimized, no debug:
# OPTS = -O2 -Wall -pedantic -Wno-deprecated -Wno-char-subscripts

# Edited for Debian - build with debugging symbols; make it possible to
# build without optimization
OPTS = -g -Wall -pedantic -Wno-deprecated -Wno-char-subscripts
ifneq (,$(findstring noopt,$(DEB_BUILD_OPTIONS)))
OPTS += -O0
else
OPTS += -O2
endif

# Edited for Debian - make it possible to install unstripped binaries
ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTIONS)))
INSTALL_OPTS = -s
else
INSTALL_OPTS =
endif

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
# Defaults are for the standard curses setup:

# CURS_DIR specifies the directory with your curses header file, if it's 
# not /usr/include/curses.h:

CURS_DIR = .

# CURS_LIB specifies the directory where the curses libraries can be found,
# if they're not in the standard search path:

CURS_LIB = .

# LIBS lists any "extra" libraries that need to be linked in:

LIBS = -lcurses

#--------------------------------------------------------------
# With PDCurses for X11:

#CURS_DIR = /usr/local/include/xcurses
#LIBS = -lXCurses

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
		MM_MINOR="$(MM_MINOR)" OPTS="$(OPTS)" \
		CURS_DIR="$(CURS_DIR)" intrfc $(SEP) cd ..

mm:	mm-main intrfc
	$(CXX) -o mm mmail/*.o interfac/*.o -L$(CURS_LIB) $(LIBS)
	$(POST)

dep:
	cd interfac $(SEP) $(MAKE) CURS_DIR="$(CURS_DIR)" dep $(SEP) cd ..
	cd mmail $(SEP) $(MAKE) dep $(SEP) cd ..

clean:
	cd interfac $(SEP) $(MAKE) RM="$(RM)" clean $(SEP) cd ..
	cd mmail $(SEP) $(MAKE) RM="$(RM)" clean $(SEP) cd ..
	$(RM) mm

modclean:
	cd mmail $(SEP) $(MAKE) RM="$(RM)" modclean $(SEP) cd ..

install::
	install -d $(PREFIX)/bin $(HELPDIR) $(DOCDIR)
	install -c $(INSTALL_OPTS) mm $(PREFIX)/bin
	install -c -m 644 mm.1 $(HELPDIR)
	$(RM) $(HELPDIR)/mmail.1
#	ln $(HELPDIR)/mm.1 $(HELPDIR)/mmail.1
	install -m 644 README $(DOCDIR)
	install -d $(DOCDIR)/colors
	cp -a colors/* $(DOCDIR)/colors
