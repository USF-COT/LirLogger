/*
 * LirLogger
 *
 * A daemon that grabs, encodes, and logs frames to /deployments
 * 
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <PvSystem.h>
#include <PvInterface.h>
#include <PvDevice.h>
#include <PvBuffer.h>

extern "C" {
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/dict.h"
}

#define BUFSIZE 32
#define DEVMAC "00-11-1c-f0-0f-09"
#define LOGDIR "/deployments"

volatile sig_atomic_t daemonrunning=1;

int main(int argc, char** argv){
    if(argc != 2){
        printf("Incorrect usage.\nEXAMPLE: LirLogger \"<Camera MAC Address>\"");
    }

    char* MACAddress = argv[1];

    printf("Starting Lir Logger.  All subsequent messages will be appended to system log.\n");

    openlog("LIRLOGGER",LOG_PID,0);
    if(daemon(0,0)){
        syslog(LOG_DAEMON|LOG_ERR,"Error becoming daemon.  Error: %d  Exiting.",errno);
        exit(1);
    }
    syslog(LOG_DAEMON|LOG_INFO,"Daemon running");
    
    closelog();
}
