SRC_CPPS = \
           ConfigREST.cpp \
           LirLogger.cpp \
           LirCommand.cpp \
           Spyder3Camera.cpp \
           Spyder3TiffWriter.cpp \
           EthSensor.cpp \
           LirSQLiteWriter.cpp \
           MemoryCameraStatsListener.cpp \
           MemoryEthSensorListener.cpp \
           ZMQEthSensorPublisher.cpp \
           ZMQCameraStatsPublisher.cpp \

#SRC_CPPS = \
#           LirTest.cpp

EXEC     = LirLogger

#FFMPEG_LIBS=libavdevice libavformat libavfilter libavcodec libswscale libavutil
#CFLAGS+=$(shell pkg-config  --cflags $(FFMPEG_LIBS))
#CPPFLAGS+=$(shell pkg-config  --cflags $(FFMPEG_LIBS))
#LDFLAGS+=$(shell pkg-config --libs $(FFMPEG_LIBS))
LDFLAGS+=-g -lcurl -ljson -ltiff -lzmq -lboost_thread -lboost_system -lboost_filesystem -lsqlite3
CPPFLAGS+=-D__STDC_CONSTANT_MACROS 

include sample.Makefile
