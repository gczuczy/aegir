INCPATH=
CXXFLAGS=-Werror -pedantic -std=c++11 $(INCPATH)
LDFLAGS=-lpthread

OBJS=main.o
OUT=../bin/aegir-brewd

.SUFFIXES : .o .c .cc .hh .h
.OBJDIR   : obj/
.PATH.cc  : $(.CURDIR)/src/ $(.CURDIR)/../common-ng/src/
.PATH.hh  : $(.CURDIR)/src/ $(.CURDIR)/../common-ng/src/

.sinclude ".depend"

.PHONY = clean aegir-brewd depend .depend
.DEFAULT = aegir-brewd
.MAIN = aegir-brewd

aegir-brewd: .depend $(OUT)

depend: .depend

.depend:
	mkdep $(INCPATH) ../src/*.cc

clean:
	rm -f .depend $(OUT) $(OBJS)
	rm -f ../src/*.cc~

$(OUT): $(OBJS)
	$(CXX) -o $@ $(.ALLSRC) $(LDFLAGS)

.cc.o:
	$(CXX) -c -pipe -o $@ $< $(CXXFLAGS)
