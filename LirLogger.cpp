/*
 * LirLogger
 *
 * A daemon that grabs, encodes, and logs frames in relation
 * to environmental sensors.
 * 
 */ 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <boost/thread/thread.hpp>

#include "LirCommand.hpp"
#include "Spyder3Camera.hpp"

#define NUM_THREADS 10
#define MAXBUF 1024 
#define IMGBUFSIZE 32

LirCommand* com;
volatile sig_atomic_t daemonrunning=1;

int list_s; // Listening socket

void catch_term(int sig){
    daemonrunning=0;
    close(list_s);
    return;
}

void* handleConnection(int connection){
    char buffer[MAXBUF+1];
    int numBytesReceived;

    do{
        numBytesReceived = recv(connection,buffer,MAXBUF,0);
        if(numBytesReceived > 0){
            // Terminate buffer
            buffer[numBytesReceived] = '\0';
            
            // Check for exit
            std::string bufString(buffer);
            if(bufString.substr(0,bufString.find_first_of(" \r\n",0)).compare("exit") == 0) break;

            syslog(LOG_DAEMON|LOG_INFO,"Received: %s", buffer);
            com->parseCommand(connection,buffer);
        }
    }while(numBytesReceived >= 0 && daemonrunning);

    if ( close(connection) < 0 ) {
        syslog(LOG_DAEMON|LOG_ERR,"(Connection %d)Error calling close() on connection socket. Daemon Terminated.",connection);
        exit(EXIT_FAILURE);
    }

    syslog(LOG_DAEMON|LOG_INFO,"(Connection %d)Connected Socket Closed.",connection);
}

int main(){
    int conn_s;
    short int port = 8045;
    struct sockaddr_in servaddr;
    int i;
    unsigned int numClients = 0;
    boost::thread_group *tgroup = new boost::thread_group();
    std::vector<int> clients;

    printf("Starting Lir Logger.  All subsequent messages will be appended to system log.\n");

    openlog("LIRLOGGER",LOG_PID,0);
    if(daemon(1,0)){
        syslog(LOG_DAEMON|LOG_ERR,"Error becoming daemon.  Error: %d  Exiting.",errno);
        exit(EXIT_FAILURE);
    }
    syslog(LOG_DAEMON|LOG_INFO,"Daemon running");

    // Initialize the Spyder 3 Camera
    syslog(LOG_DAEMON|LOG_INFO,"Creating command instance.");
    com = LirCommand::Instance();
    syslog(LOG_DAEMON|LOG_INFO,"Loading configuration.");
    com->loadConfig("/etc/LirLogger/config.xml");
    syslog(LOG_DAEMON|LOG_INFO,"Config loaded.");

    // Setup SIGTERM Handler
    signal(SIGTERM,catch_term); 

    // Setup server socket
    /* Setup TCP/IP Socket */
    if((list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        syslog(LOG_DAEMON|LOG_ERR,"Unable to create socket. Daemon Terminated.");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);    
    /*  Bind our socket addresss to the
        listening socket, and call listen()  */

    if ( bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
        syslog(LOG_DAEMON|LOG_ERR,"Unable to bind socket. Daemon Terminated.");
        exit(EXIT_FAILURE);
    }

    if ( listen(list_s, NUM_THREADS) < 0 ) {
        syslog(LOG_DAEMON|LOG_ERR,"Unable to listen on socket. Daemon Terminated.");
        exit(EXIT_FAILURE);
    }

    while(daemonrunning){
        syslog(LOG_DAEMON|LOG_INFO,"Listening for connection on port %i", port);
        // Wait for TCP/IP Connection
        conn_s = accept(list_s, NULL, NULL);
        if ( conn_s < 0 ) {
            syslog(LOG_DAEMON|LOG_ERR,"Unable to call accept() on socket.");
            break;
        }

        // Spawn a Server Thread to Handle Connected Socket
        syslog(LOG_DAEMON|LOG_INFO,"Handling new connection on port %i",port);
        clients.push_back(conn_s);
        tgroup->add_thread(new boost::thread(handleConnection,conn_s));
        //pthread_create(&thread_bin[i],NULL,handleConnection, (void*)info);
    }
    tgroup->join_all();
    com->stopLogger();
    syslog(LOG_DAEMON|LOG_INFO,"Daemon Exited Successfully.");
    closelog();
    exit(EXIT_SUCCESS);
}
