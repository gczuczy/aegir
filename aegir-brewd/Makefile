INCPATH=-I/usr/local/include -I ../contrib/
CXXFLAGS=-Werror -pedantic -std=c++11 $(INCPATH)
LDFLAGS=-pthread -L/usr/local/lib -lboost_program_options -lgpio -lyaml-cpp -lzmq

OBJS=main.o GPIO.o Config.o Exception.o ThreadManager.o IOHandler.o ZMQ.o \
	Message.o
OUT=../bin/aegir-brewd

.SUFFIXES : .o .cc .hh .hpp
.OBJDIR   : obj/
.PATH.cc  : $(.CURDIR)/src/
.PATH.hh  : $(.CURDIR)/src/
.PATH.hpp  : $(.CURDIR)/contrib/

.sinclude ".depend"

.PHONY = clean brewd depend .depend

brewd: .depend $(OUT)

depend: .depend

.depend: ../Makefile
	mkdep $(INCPATH) ../src/*.cc ../src/*.hh ../contrib/*.hpp

clean:
	rm -f .depend $(OUT) $(OBJS)
	rm -f ../src/*~

$(OUT): $(OBJS)
	$(CXX) -o $@ $(.ALLSRC) $(LDFLAGS)

.cc.o:
	$(CXX) -c -pipe -o $@ $< $(CXXFLAGS)
