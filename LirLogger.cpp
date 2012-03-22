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
#include <PvBuffer.h>
#include "Spyder3FrameGrabber.h"
#include "FrameWriteBuffer.h"

#define BUF_SIZE 32
#define CONFIGFILE "config.xml"
#define LOGDIR "/deployments"

volatile sig_atomic_t daemonrunning=1;

void catch_term(int sig){
    syslog(LOG_DAEMON|LOG_INFO,"SIGTERM Caught");
    daemonrunning = 0;
}

int main(void){
    openlog("LIRLOGGER",LOG_PID,0);
    if(daemon(0,0)){
        syslog(LOG_DAEMON|LOG_ERR,"Error becoming daemon.  Error: %d  Exiting.",errno);
        exit(1);
    }
    syslog(LOG_DAEMON|LOG_INFO,"Daemon running");

    signal(SIGTERM, catch_term); 

    // Create shared buffer
    FrameWriteBuffer *buffer = new FrameWriteBuffer(BUF_SIZE);

    // Create and start spyder3 frame grabber manager.  Pass it shared buffer.
    

    // Create and start frame encoder.  Pass it shared frame buffer


    closelog();
}
