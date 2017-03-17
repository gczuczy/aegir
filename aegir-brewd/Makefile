INCPATH=-I/usr/local/include
CXXFLAGS=-Werror -pedantic -std=c++11 $(INCPATH)
LDFLAGS=-pthread -L/usr/local/lib -lboost_program_options -lgpio -lyaml-cpp

OBJS=main.o GPIO.o Config.o Exception.o ThreadManager.o
OUT=../bin/aegir-brewd

.SUFFIXES : .o .cc .hh
.OBJDIR   : obj/
.PATH.cc  : $(.CURDIR)/src/
.PATH.hh  : $(.CURDIR)/src/

.sinclude ".depend"

.PHONY = clean brewd depend .depend

brewd: .depend $(OUT)

depend: .depend

.depend:
	mkdep $(INCPATH) ../src/*.cc ../src/*.hh

clean:
	rm -f .depend $(OUT) $(OBJS)
	rm -f ../src/*~

$(OUT): $(OBJS)
	$(CXX) -o $@ $(.ALLSRC) $(LDFLAGS)

.cc.o:
	$(CXX) -c -pipe -o $@ $< $(CXXFLAGS)
