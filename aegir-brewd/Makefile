INCPATH=-I/usr/local/include -I../contrib/include
CXXFLAGS=-Werror -pedantic -std=c++14 $(INCPATH)
LDFLAGS=-pthread -L/usr/local/lib -lboost_program_options -lgpio -lyaml-cpp -lzmq -ljsoncpp

OBJS=main.o GPIO.o Config.o Exception.o ThreadManager.o IOHandler.o ZMQ.o \
	Message.o Controller.o SPI.o DirectSelect.o MAX31856.o PRThread.o \
	Program.o JSONMessage.o ProcessState.o PINTracker.o
OUT=../bin/aegir-brewd

.if defined(DEBUG)
CXXFLAGS+= -O0 -glldb
.else
CXXFLAGS+= -O2
.endif
.ifdef defined(DEBUG_AEGIR)
CXXFLAGS+=  -DMAX31856_DEBUG -DGPIO_DEBUG -DAEGIR_DEBUG
.endif

.SUFFIXES : .o .cc .hh
.OBJDIR   : obj/
.PATH.cc  : $(.CURDIR)/src/
.PATH.hh  : $(.CURDIR)/src/

.PHONY = clean brewd depend .depend

brewd: .depend $(OUT)

depend: .depend

.depend: ../Makefile
	mkdep $(INCPATH) ../src/*.cc ../src/*.hh

clean:
	rm -f .depend $(OUT) $(OBJS)
	rm -f ../src/*~

$(OUT): $(OBJS)
	$(CXX) -o $@ $(.ALLSRC) $(LDFLAGS)

.cc.o:
	$(CXX) -c -pipe -o $@ $< $(CXXFLAGS)

.sinclude ".depend"
