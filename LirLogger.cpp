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
#include <pthread.h>

#include "LirCommand.hpp"
#include "Spyder3Camera.hpp"

#define NUM_THREADS 10
#define MAXBUF 1024 
#define IMGBUFSIZE 32

volatile sig_atomic_t daemonrunning=1;
volatile sig_atomic_t numClients=0;

int list_s; // Listening socket

void catch_term(int sig){
    daemonrunning=0;
    close(list_s);
    return;
}

typedef enum {UNAVAILABLE, AVAILABLE}AVAILABILITY;
pthread_t thread_bin[NUM_THREADS];
AVAILABILITY thread_bin_available[NUM_THREADS];

typedef struct THREADINFO{
    unsigned short thread_bin_index;
    int socket_connection;
} threadInfo;

void* handleConnection(void* info){
    int* connection = &((threadInfo*)info)->socket_connection;
    char buffer[MAXBUF+1];
    int numBytesReceived, offset;

    numBytesReceived = recv(*connection,buffer,MAXBUF,0);
    while(numBytesReceived > 0 && daemonrunning){
        // Terminate buffer
        buffer[numBytesReceived] = '\0';
        offset = 0;
        do{
            //offset += parseCommand(*connection,buffer,offset,MAXBUF);
        }while(offset <= MAXBUF && buffer[offset] != '\0' && daemonrunning);
        numBytesReceived = recv(*connection,buffer,MAXBUF,0);
    }

    if ( close(*connection) < 0 ) {
        syslog(LOG_DAEMON|LOG_ERR,"(Thread %i)Error calling close() on connection socket. Daemon Terminated.",((threadInfo*)info)->thread_bin_index);
        exit(EXIT_FAILURE);
    }

    syslog(LOG_DAEMON|LOG_INFO,"(Thread %i)Connected Socket Closed.",((threadInfo*)info)->thread_bin_index);
    numClients--;
    thread_bin_available[((threadInfo*)info)->thread_bin_index] = AVAILABLE;
    free(info);
    pthread_exit(NULL);
}

int main(){
    int conn_s;
    short int port = 8045;
    struct sockaddr_in servaddr;
    int i, availableThreadID;
    uint8_t numClients = 0;
    threadInfo* info;

    printf("Starting Lir Logger.  All subsequent messages will be appended to system log.\n");

    openlog("LIRLOGGER",LOG_PID,0);
    if(daemon(1,0)){
        syslog(LOG_DAEMON|LOG_ERR,"Error becoming daemon.  Error: %d  Exiting.",errno);
        exit(EXIT_FAILURE);
    }
    syslog(LOG_DAEMON|LOG_INFO,"Daemon running");

    // Initialize the Spyder 3 Camera
    syslog(LOG_DAEMON|LOG_INFO,"Creating command instance.");
    LirCommand* com = LirCommand::Instance();
    syslog(LOG_DAEMON|LOG_INFO,"Loading configuration.");
    com->loadConfig("config.xml");
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

    for(i=0; i < NUM_THREADS; i++)
        thread_bin_available[i] = AVAILABLE;

    while(daemonrunning){
        syslog(LOG_DAEMON|LOG_INFO,"Listening for connection on port %i", port);
        /* Wait for TCP/IP Connection */
        conn_s = accept(list_s, NULL, NULL);
        if ( conn_s < 0 ) {
            syslog(LOG_DAEMON|LOG_ERR,"Unable to call accept() on socket.");
            break;
        }

        /* Spawn a POSIX Server Thread to Handle Connected Socket */
        for(i=0; i < NUM_THREADS; i++){
            if(thread_bin_available[i]){
                thread_bin_available[i] = UNAVAILABLE;
                syslog(LOG_DAEMON|LOG_INFO,"Handling new connection on port %i",port);
                numClients++;
                info = (threadInfo*)malloc(sizeof(threadInfo));
                info->socket_connection = conn_s;
                info->thread_bin_index = i;
                pthread_create(&thread_bin[i],NULL,handleConnection, (void*)info);
                break;
            }
        }

        if(i > NUM_THREADS){
            syslog(LOG_DAEMON|LOG_ERR,"Unable to create thread to handle connection.  Continuing...");
        }

    }
    syslog(LOG_DAEMON|LOG_INFO,"Daemon Exited Successfully.");
    closelog();
    exit(EXIT_SUCCESS);
}
