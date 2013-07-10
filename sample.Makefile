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

CC                  = gcc
CPP                 = g++
LD                  = $(CC)
SRC_MOC             =
MOC			        =
RCC					=

PUREGEV_ROOT        ?= ../../..
CFLAGS              += -I$(PUREGEV_ROOT)/include
CPPFLAGS            += -I$(PUREGEV_ROOT)/include
LDFLAGS             += -L$(PUREGEV_ROOT)/lib	\
						-lPvBase             	\
             			-lPvDevice          	\
             			-lPvBuffer          	\
             			-lPvGUIUtils         	\
             			-lPvGUI              	\
             			-lPvPersistence      	\
             			-lPvGenICam          	\
             			-lPvStreamRaw        	\
             			-lPvStream           	\
             			-lPvTransmitterRaw   	\
             			-lPvVirtualDevice
PV_LIBRARY_PATH      = $(PUREGEV_ROOT)/lib

ifneq ($(shell uname -m),x86_64)
	LDFLAGS            += -L$(PUREGEV_ROOT)/lib/genicam/bin/Linux32_i86
	GEN_LIB_PATH        =  $(PUREGEV_ROOT)/lib/genicam/bin/Linux32_i86
else
	LDFLAGS            += -L$(PUREGEV_ROOT)/lib/genicam/bin/Linux64_x64
	GEN_LIB_PATH        =  $(PUREGEV_ROOT)/lib/genicam/bin/Linux64_x64
endif

LD_LIBRARY_PATH        = $(GEN_LIB_PATH):$(OPENCV_LIBRARY_PATH):$(QT_LIBRARY_PATH):$(PV_LIBRARY_PATH)
export LD_LIBRARY_PATH

ifdef _DEBUG
    CFLAGS    += -g -D_DEBUG
    CPPFLAGS  += -g -D_DEBUG
else
    CFLAGS    += -O3
    CPPFLAGS  += -O3
endif

CFLAGS    += -D_UNIX_ -D_LINUX_
CPPFLAGS  += -D_UNIX_ -D_LINUX_

OBJS      += $(SRC_CPPS:%.cpp=%.o)
OBJS      += $(SRC_CS:%.c=%.o)

all: $(EXEC)
	sudo cp LirLogger /usr/local/bin

clean:
	rm -rf $(OBJS) $(EXEC) $(SRC_MOC) $(SRC_QRC)

moc_%.cxx: %.h
	$(MOC) $< -o $@ 

qrc_%.cxx: %.qrc
	$(RCC) $< -o $@

%.o: %.cxx
	$(CPP) -c $(CPPFLAGS) -o $@ $<

%.o: %.cpp
	$(CPP) -c $(CPPFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(EXEC): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

.PHONY: all clean
