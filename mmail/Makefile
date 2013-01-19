CXXFLAGS = $(OPTS) -DMM_MAJOR=$(MM_MAJOR) -DMM_MINOR=$(MM_MINOR)

OBJS = misc.o resource.o mmail.o driverl.o filelist.o area.o letter.o \
read.o compress.o pktbase.o bw.o qwk.o omen.o soup.o opx.o

include depend

dep:
	$(CXX) -MM *.cc > depend

clean:
	$(RM) *.o

mm-main: $(OBJS)
