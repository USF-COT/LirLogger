# Parameters
# SRC_CS: The source C files to compie
# SRC_CPPS: The source CPP files to compile
# EXEC: The executable name

ifeq ($(SRC_CS) $(SRC_CPPS),)
  $(error No source files specified)
endif

ifeq ($(EXEC),)
  $(error No executable file specified)
endif

CC=gcc
CPP=g++
LD=$(CC)

PUREGEV_ROOT ?= ../../..

INC_DIR         = $(PUREGEV_ROOT)/include
LIB_DIR         = $(PUREGEV_ROOT)/lib
QT_LIB_DIR      = $(PUREGEV_ROOT)/lib/qt/lib

ifneq ($(shell uname -m),x86_64)
GENICAM_LIB_DIR = $(LIB_DIR)/genicam/bin/Linux32_i86
else
GENICAM_LIB_DIR = $(LIB_DIR)/genicam/bin/Linux64_x64
endif


LD_LIBRARY_PATH=$(LIB_DIR):$(GENICAM_LIB_DIR):$(QT_LIB_DIR)
export LD_LIBRARY_PATH

ifdef _DEBUG
CFLAGS    += -g -D_DEBUG
CPPFLAGS  += -g -D_DEBUG
else
CFLAGS    += -O3
CPPFLAGS  += -O3
endif

CFLAGS    += -I$(INC_DIR) -D_UNIX_ -D_LINUX_
CPPFLAGS  += -I$(INC_DIR) -D_UNIX_ -D_LINUX_
LDFLAGS   += -L$(LIB_DIR)         \
             -L$(GENICAM_LIB_DIR) \
             -L/usr/local/lib     \
             -lPvBase             \
             -lPvDevice           \
             -lPvBuffer           \
             -lPvGUI              \
             -lPvPersistence      \
             -lPvGenICam          \
             -lPvStreamRaw        \
             -lPvStream           \
             -lPvTransmitterRaw   \
             -lPvVirtualDevice

OBJS      += $(SRC_CPPS:%.cpp=%.o)
OBJS      += $(SRC_CS:%.c=%.o)

all: $(EXEC)

$(EXEC): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o: %.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -rf $(OBJS) $(EXEC)

.PHONY: all clean
