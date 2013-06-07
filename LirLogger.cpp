/*
 * LirLogger
 *
 * A daemon that grabs, encodes, and logs frames in relation
 * to environmental sensors.
 * 
 */ 

#include <zmq.hpp>
#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include "LirCommand.hpp"
#include "Spyder3Camera.hpp"
#include "LirTCPServer.hpp"

volatile sig_atomic_t daemonrunning=1;
boost::asio::io_service io_service;

void catch_term(int sig){
    daemonrunning=0;
    io_service.stop();
    return;
}

int main(){
    printf("Starting Lir Logger.  All subsequent messages will be appended to system log.\n");

    openlog("LIRLOGGER",LOG_PID,0);

    if(daemon(1,0)){
        syslog(LOG_DAEMON|LOG_ERR,"Error becoming daemon.  Error: %d  Exiting.",errno);
        exit(EXIT_FAILURE);
    }
    syslog(LOG_DAEMON|LOG_INFO,"Daemon running");

    // Initialize Lir Command 
    syslog(LOG_DAEMON|LOG_INFO,"Creating command instance.");
    LirCommand* com = LirCommand::Instance();

    // Setup SIGTERM Handler
    signal(SIGTERM,catch_term); 

    // Setup ZMQ RESP Server
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:5555");

    while(daemonrunning){
        zmq::message_t request;

        socket.recv(&request);
    }

    // Housekeeping
    com->stopLogger(); // Just in case it was left running
    syslog(LOG_DAEMON|LOG_INFO,"Daemon Exited Successfully.");
    closelog();
    exit(EXIT_SUCCESS);
}
