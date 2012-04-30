SRC_CPPS = \
           LirLogger.cpp \
           LirCommand.cpp \
           Spyder3Camera.cpp \

EXEC     = LirLogger

#FFMPEG_LIBS=libavdevice libavformat libavfilter libavcodec libswscale libavutil
#CFLAGS+=$(shell pkg-config  --cflags $(FFMPEG_LIBS))
#CPPFLAGS+=$(shell pkg-config  --cflags $(FFMPEG_LIBS))
#LDFLAGS+=$(shell pkg-config --libs $(FFMPEG_LIBS))
LDFLAGS+=-ltiff -lxerces-c -L/usr/src/boost_1_49_0/bin.v2/libs/thread/build/gcc-4.1.2/release/address-model-64/threading-multi/ -lboost_thread
CPPFLAGS+=-D__STDC_CONSTANT_MACROS -I/usr/src/boost_1_49_0

include sample.Makefile
