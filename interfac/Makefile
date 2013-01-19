CXXFLAGS = $(OPTS) -DCURS_INC=$(CURS_INC) -DMM_MAJOR=$(MM_MAJOR) \
-DMM_MINOR=$(MM_MINOR)

OBJS = mmcolor.o mysystem.o isoconv.o basic.o interfac.o packet.o \
arealist.o letterl.o letterw.o lettpost.o ansiview.o addrbook.o tagline.o \
help.o main.o

include depend

dep:
	$(CXX) -DCURS_INC=$(CURS_INC) -MM *.cc > depend

clean:
	$(RM) *.o

intrfc: $(OBJS)
