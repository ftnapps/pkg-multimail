#-----------------------------------------------
# MultiMail Makefile (top) for Borland/Turbo C++
#-----------------------------------------------

!include version

#--------------------------------------------------------------
# Turbo C++ 3.0:

CURS_INC = \\"/tc/pdcurs24/curses.h\\"
CURS_DIR = \tc\pdcurs24
CURS_LIB = .
LIBS = spawnl.lib e:\tc\pdcurs24\dos\pdcurses.lib
#LIBS = spawnh.lib e:\tc\pdcurs24\dos\cursesh.lib
RM = del
LIST = tclist

#--------------------------------------------------------------
#--------------------------------------------------------------

all:	mm

mm-main:
	cd mmail
	$(MAKE) -fMakefile.bcc -DMM_MAJOR=$(MM_MAJOR) \
		-DMM_MINOR=$(MM_MINOR) mm-main
	cd ..

intrfc:
	cd interfac
	$(MAKE) -fMakefile.bcc -DMM_MAJOR=$(MM_MAJOR) \
		-DMM_MINOR=$(MM_MINOR) -DOPTS="-I$(CURS_DIR)" \
		-DCURS_INC="$(CURS_INC)" intrfc
	cd ..

mm:	mm-main intrfc
	$(CC) -emm @$(LIST) $(LIBS)

clean:
	cd interfac
	$(MAKE) -fMakefile.bcc -DRM="$(RM)" clean
	cd ..
	cd mmail
	$(MAKE) -fMakefile.bcc -DRM="$(RM)" clean
	cd ..
	$(RM) mm.exe

modclean:
	cd mmail
	$(MAKE) -fMakefile.bcc -DRM="$(RM)" modclean
	cd ..
