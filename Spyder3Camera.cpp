
#include "Spyder3Camera.hpp"

#include <stdlib.h>

#include <PvSystem.h>
#include <PvInterface.h>
#include <PvDeviceFinderWnd.h>
#include <PvDevice.h>
#include <PvPipeline.h>
#include <PvBuffer.h>
#include <PvStream.h>
#include <PvStreamRaw.h>
#include <PvBufferWriter.h>
#include <PvPixelType.h>
#include <syslog.h>

Spyder3Camera::Spyder3Camera(const char* _MAC, unsigned int _pipelineBufferMax) : pipelineBufferMax(_pipelineBufferMax){
    MAC = (char*) malloc(sizeof(char)*(strlen(_MAC)+1));
    strncpy(MAC,_MAC,strlen(_MAC)+1);
    camThread = NULL;
}

Spyder3Camera::~Spyder3Camera(){
    free(MAC);
}

bool Spyder3Camera::start(){
    if(camThread == NULL){
        isRunning = true;
        syslog(LOG_DAEMON|LOG_INFO,"Starting camera thread.");
        camThread = new boost::thread(boost::ref(*this));
    }
}

bool Spyder3Camera::stop(){
    if(camThread != NULL){
        runMutex.lock();
        isRunning = false;
        runMutex.unlock();
        
        syslog(LOG_DAEMON|LOG_INFO,"Joining camera thread.");
        if(camThread) camThread->join();
        camThread = NULL;
    }
}

void Spyder3Camera::operator() (){
    bool running;

    // Setup code
    syslog(LOG_DAEMON|LOG_INFO,"Camera thread running.");

    // Pipeline loop 
    do{

        runMutex.lock();
        running = isRunning;
        runMutex.unlock();

    }while(running); 

    // Housekeeping!
    syslog(LOG_DAEMON|LOG_INFO,"Camera thread stopped.");
}
