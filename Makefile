SRC_CPPS = \
           LirTest.cpp \

EXEC     = LirTest

#FFMPEG_LIBS=libavdevice libavformat libavfilter libavcodec libswscale libavutil
#CFLAGS+=$(shell pkg-config  --cflags $(FFMPEG_LIBS))
#CPPFLAGS+=$(shell pkg-config  --cflags $(FFMPEG_LIBS))
#LDFLAGS+=$(shell pkg-config --libs $(FFMPEG_LIBS))
LDFLAGS+=-ltiff
CPPFLAGS+=-D__STDC_CONSTANT_MACROS

include sample.Makefile
