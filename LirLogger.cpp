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
#include <signal.h>

#include "LirCommand.hpp"

volatile sig_atomic_t daemonrunning=1;

void catch_term(int sig){
    daemonrunning=0;
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
        zmq::message_t requestMessage;

        // Receive Request
        socket.recv(&requestMessage);
        std::string request((char *)requestMessage.data(), requestMessage.size());
        std::string response = com->parseCommand(request);

        // Send Response
        zmq::message_t responseMessage(response.size());
        memcpy((void*)responseMessage.data(), response.data(), response.size());
        socket.send(responseMessage);
    }

    // Housekeeping
    com->stopLogger(); // Just in case it was left running
    syslog(LOG_DAEMON|LOG_INFO,"Daemon Exited Successfully.");
    closelog();
    exit(EXIT_SUCCESS);
}
