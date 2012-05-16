SRC_CPPS = \
           LirLogger.cpp \
           LirCommand.cpp \
           Spyder3Camera.cpp \
           Spyder3TiffWriter.cpp \
           EthSensor.cpp \
           LirSQLiteWriter.cpp \
           LirTCPServer.cpp \

EXEC     = LirLogger

#FFMPEG_LIBS=libavdevice libavformat libavfilter libavcodec libswscale libavutil
#CFLAGS+=$(shell pkg-config  --cflags $(FFMPEG_LIBS))
#CPPFLAGS+=$(shell pkg-config  --cflags $(FFMPEG_LIBS))
#LDFLAGS+=$(shell pkg-config --libs $(FFMPEG_LIBS))
LDFLAGS+=-ltiff -lxerces-c -lboost_thread -lboost_system -lsqlite3
CPPFLAGS+=-D__STDC_CONSTANT_MACROS 

include sample.Makefile
