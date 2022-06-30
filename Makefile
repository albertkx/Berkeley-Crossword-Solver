CC = clang++
C  = gcc

OS := $(shell uname)

ifeq ($(OS),Linux)
CFLAGS = -g -O3 -std=c++11 -pthread -Wall -Wno-c++11-extensions
# CFLAGS = -g -std=c++11 -pthread -Wall -Wno-c++11-extensions
else
CFLAGS = -g -O3 -std=c++11 -Wall -Wno-c++11-extensions
# CFLAGS = -g -O3 -std=c++11 -Wall -Wno-c++11-extensions -fsanitize=address
# CFLAGS = -g -std=c++11 -Wall -Wno-c++11-extensions
endif

TARGET = xw

SRCS = xw.cpp bits.cpp compress.cpp data.cpp utils.cpp pos.cpp score.cpp \
	display.cpp puz.cpp word.cpp solve.cpp xref.cpp clue.cpp \
	fill.cpp berkeley.cpp trans.cpp rebus.cpp multiword.cpp dump.cpp \
	task.cpp flamingo.cpp theme.cpp lev.cpp t2p.c fitb.cpp geom.cpp tune.cpp

OTHER_SRCS = xwg.cpp gui.cpp

OBJS = $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRCS))) \
	$(patsubst %.c,%.o,$(filter %.c,$(SRCS)))

# OBJS = $(SRCS:.cpp=.o) t2p.o

ifeq ($(OS),Linux)
CXX_EXTRA = `wx-config --cxxflags` -DWX_PRECOMP 
LINK_EXTRA = -lm `wx-config --libs`
else
CXX_EXTRA = `wx-config --cxxflags` -DWX_PRECOMP 
LINK_EXTRA = `wx-config --libs` -stdlib=libc++ -lm
endif

$(TARGET): $(OBJS)
#	 @ $(MAKE) -C flite
	   $(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(FLIBS) $(T2PLIBS)

FLAMINGO_EXTRA = -I./flamingo/src/topk/src -I./flamingo/src
FTOPK = ./flamingo/src/topk/CMakeFiles/topk-lib.dir/src
FCOMMON = ./flamingo/src/common/CMakeFiles/common-lib.dir/src
FLIBS = $(FTOPK)/topkindex.cc.o $(FTOPK)/topkheap.cc.o $(FCOMMON)/gramgen.cc.o \
	$(FCOMMON)/simmetric.cc.o $(FCOMMON)/filtertypes.cc.o

ifeq ($(OS),Linux)
FLITEDIR = x86_64-linux-gnu
else
FLITEDIR = x86_64-darwin15.6.0
endif

T2PLIBS =  -Lflite/build/$(FLITEDIR)/lib -lflite_cmu_us_kal -lflite_cmu_time_awb -lflite_cmu_us_kal16 -lflite_cmu_us_awb -lflite_cmu_us_rms -lflite_cmu_us_slt -lflite_usenglish -lflite_cmulex -lflite -lm

flamingo.o: flamingo.cpp
	$(CC) $(CFLAGS) $(FLAMINGO_EXTRA) -c flamingo.cpp

xwg: xwg.o gui.o
	$(CC) $(CFLAGS) -o xwg xwg.o gui.o -lm $(LINK_EXTRA)

app:
	mkdir -p xwg.app/Contents
	mkdir -p xwg.app/Contents/MacOS
	mkdir -p xwg.app/Contents/Resources
#	sed -e "s/IDENTIFIER/`echo $(srcdir) | sed -e 's,\.\./,,g' | sed -e 's,/,.,g'`/" \
#	-e "s/EXECUTABLE/xwg/" \
#	-e "s/VERSION/$(WX_VERSION)/" \
#	Info.plist.in >xwg.app/Contents/Info.plist
	echo -n "APPL????" >xwg.app/Contents/PkgInfo
	ln -f xwg$(EXEEXT) xwg.app/Contents/MacOS/xwg
	cp -f wxmac.icns xwg.app/Contents/Resources/wxmac.icns

app:  xwg Info.plist.in wxmac.icns

bt: bt.o bits.o
	$(CC) -o bt bt.o bits.o

bt.o: bt.cpp
	$(CC) $(CXX_EXTRA) $(CFLAGS) -c $<

gui.o:
	$(CC) $(CXX_EXTRA) $(CFLAGS) -c $< 

xwg.o:
	$(CC) $(CXX_EXTRA) $(CFLAGS) -c $< 

ifeq ($(OS),Linux)
t2p.o: t2p.c
	@ $(MAKE) -C flite
	$(C) -g -O2 -Wall -Iflite/include -c -o t2p.o t2p.c
else
t2p.o: t2p.c
	@ $(MAKE) -C flite
	$(C) -g -O2 -Wall -no-cpp-precomp -Iflite/include -c -o t2p.o t2p.c
endif

tester: tester.o
	$(CC) $(CFLAGS) -o tester tester.o

.cpp.o:
	$(CC) $(CFLAGS) $(CXX_EXTRA) -c $< 

clean:	
	rm -f $(TARGET) *.o

deps: 
	$(CC) $(CFLAGS) $(CXX_EXTRA) $(FLAMINGO_EXTRA) \
	-Iflite/include -MM $(SRCS) $(OTHER_SRCS) $<

xw.o: xw.cpp xw.h score.h bits.h solvergui.h
bits.o: bits.cpp bits.h
compress.o: compress.cpp score.h bits.h solvergui.h
data.o: data.cpp score.h bits.h solvergui.h fitb.h
utils.o: utils.cpp xw.h score.h bits.h solvergui.h
pos.o: pos.cpp score.h bits.h solvergui.h
score.o: score.cpp xw.h score.h bits.h solvergui.h
display.o: display.cpp xw.h score.h bits.h solvergui.h
puz.o: puz.cpp xw.h score.h bits.h solvergui.h wide.h
word.o: word.cpp xw.h score.h bits.h solvergui.h
solve.o: solve.cpp xw.h score.h bits.h solvergui.h
xref.o: xref.cpp xw.h score.h bits.h solvergui.h
clue.o: clue.cpp xw.h score.h bits.h solvergui.h
fill.o: fill.cpp xw.h score.h bits.h solvergui.h
berkeley.o: berkeley.cpp xw.h score.h bits.h
trans.o: trans.cpp xw.h score.h bits.h solvergui.h
rebus.o: rebus.cpp xw.h score.h bits.h solvergui.h
multiword.o: multiword.cpp xw.h score.h bits.h solvergui.h
dump.o: dump.cpp score.h bits.h solvergui.h
task.o: task.cpp xw.h score.h bits.h solvergui.h
flamingo.o: flamingo.cpp score.h bits.h solvergui.h
theme.o: theme.cpp xw.h score.h bits.h solvergui.h
lev.o: lev.cpp score.h bits.h solvergui.h
t2p.o: t2p.c
fitb.o: fitb.cpp score.h bits.h solvergui.h fitb.h
geom.o: geom.cpp xw.h score.h bits.h solvergui.h
tune.o: tune.cpp xw.h score.h bits.h solvergui.h
xwg.o: xwg.cpp xwg.h solvergui.h
gui.o: gui.cpp xwg.h solvergui.h
