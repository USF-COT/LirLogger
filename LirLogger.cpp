/*
 * LirLogger
 *
 * A daemon that grabs, encodes, and logs frames in relation
 * to environmental sensors.
 * 
 */ 

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

/*
volatile sig_atomic_t daemonrunning=1;

void catch_term(int sig){
    daemonrunning=0;
    return;
}
*/

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
    syslog(LOG_DAEMON|LOG_INFO,"Loading configuration.");
    com->loadConfig("/etc/LirLogger/config.xml");
    syslog(LOG_DAEMON|LOG_INFO,"Config loaded.");

    // Setup SIGTERM Handler
    // signal(SIGTERM,catch_term); 

    // Setup server socket
    try{
        boost::asio::io_service io_service;
        LirTCPServer server(io_service);
        io_service.run();
    } catch (std::exception& e){
        syslog(LOG_DAEMON|LOG_ERR,e.what());
    }
    com->stopLogger(); // Just in case it was left running
    syslog(LOG_DAEMON|LOG_INFO,"Daemon Exited Successfully.");
    closelog();
    exit(EXIT_SUCCESS);
}
