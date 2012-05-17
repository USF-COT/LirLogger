
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
    isRunning = false;
}

Spyder3Camera::~Spyder3Camera(){
    free(MAC);
}

string Spyder3Camera::getMAC(){
    return string(this->MAC);
}

unsigned int Spyder3Camera::getNumBuffers(){
    return this->pipelineBufferMax;
}

bool Spyder3Camera::start(){
    bool running;

    runMutex.lock();
    running = isRunning;
    runMutex.unlock();

    if(!running){
        isRunning = true;
        syslog(LOG_DAEMON|LOG_INFO,"Starting camera thread.");
        camThread = new boost::thread(boost::ref(*this));
    } else {
        syslog(LOG_DAEMON|LOG_INFO,"Camera thread currently running.");
    }
}

bool Spyder3Camera::stop(){
    bool running;

    runMutex.lock();
    running = isRunning;
    runMutex.unlock();

    if(running){
        runMutex.lock();
        isRunning = false;
        runMutex.unlock();
        
        if(camThread) camThread->join();
        syslog(LOG_DAEMON|LOG_INFO,"Camera thread stopped.");
    }
}

void Spyder3Camera::addListener(ISpyder3Listener *l){
    listenersMutex.lock();
    listeners.push_back(l);
    listenersMutex.unlock();
}

void Spyder3Camera::addStatsListener(ISpyder3StatsListener *l){
    statsListenerMutex.lock();
    statsListeners.push_back(l);
    statsListenerMutex.unlock();
}

void Spyder3Camera::operator() (){
    bool running;

    syslog(LOG_DAEMON|LOG_INFO,"Starting camera thread for camera @ %s with %d buffers",MAC,pipelineBufferMax);

    // Setup code
    PvResult result;

    // Initialize Camera System
    PvSystem system;
    system.SetDetectionTimeout(2000);
    result = system.Find();
    if(!result.IsOK()){
        syslog(LOG_DAEMON|LOG_ERR,"PvSystem::Find Error: %s", result.GetCodeString().GetAscii());
        return;
    }

    PvDeviceInfo* lDeviceInfo = NULL;
    PvDeviceInfo* tempInfo;

    // Get the number of GEV Interfaces that were found using GetInterfaceCount.
    PvUInt32 lInterfaceCount = system.GetInterfaceCount();

    // For each interface, check MAC Address against passed address
    for( PvUInt32 x = 0; x < lInterfaceCount; x++ )
    {
        // get pointer to each of interface
        PvInterface * lInterface = system.GetInterface( x );

        // Get the number of GEV devices that were found using GetDeviceCount.
        PvUInt32 lDeviceCount = lInterface->GetDeviceCount();

        for( PvUInt32 y = 0; y < lDeviceCount ; y++ )
        {
            tempInfo = lInterface->GetDeviceInfo( y );
            if(strlen(MAC) == strlen(tempInfo->GetMACAddress().GetAscii()) && strncmp(MAC,tempInfo->GetMACAddress().GetAscii(),strlen(MAC)) == 0){
                lDeviceInfo = tempInfo;
                break;
            }
        }
    }

    // If no device is selected, abort
    if( lDeviceInfo == NULL )
    {
        syslog(LOG_DAEMON|LOG_ERR,"No device selected." );
        return;
    }

    // Connect to the GEV Device
    PvDevice lDevice;
    printf( "Connecting to %s\n", lDeviceInfo->GetMACAddress().GetAscii() );
    if ( !lDevice.Connect( lDeviceInfo ).IsOK() )
    {
        syslog(LOG_DAEMON|LOG_ERR,"Unable to connect to %s", lDeviceInfo->GetMACAddress().GetAscii() );
        return;
    }
    syslog(LOG_DAEMON|LOG_INFO,"Successfully connected to %s", lDeviceInfo->GetMACAddress().GetAscii() );

     // Get device parameters need to control streaming
    PvGenParameterArray *lDeviceParams = lDevice.GetGenParameters();

    // Negotiate streaming packet size
    lDevice.NegotiatePacketSize();

    // Create the PvStream object
    PvStream lStream;

    // Open stream - have the PvDevice do it for us
    syslog(LOG_DAEMON|LOG_INFO,"Opening stream to device" );
    lStream.Open( lDeviceInfo->GetIPAddress() );

    // Create the PvPipeline object
    PvPipeline lPipeline( &lStream );

    // Reading payload size from device
    PvInt64 lSize = 0;
    lDeviceParams->GetIntegerValue( "PayloadSize", lSize );

    // Set the Buffer size and the Buffer count
    lPipeline.SetBufferSize( static_cast<PvUInt32>( lSize ) );
    lPipeline.SetBufferCount(pipelineBufferMax); // Increase for high frame rate without missing block IDs

    // Have to set the Device IP destination to the Stream
    lDevice.SetStreamDestination( lStream.GetLocalIPAddress(), lStream.GetLocalPort() );

    // IMPORTANT: the pipeline needs to be "armed", or started before
    // we instruct the device to send us images
    syslog(LOG_DAEMON|LOG_INFO,"Starting pipeline" );
    lPipeline.Start();

    // Get stream parameters/stats
    PvGenParameterArray *lStreamParams = lStream.GetParameters();

    // TLParamsLocked is optional but when present, it MUST be set to 1
    // before sending the AcquisitionStart command
    lDeviceParams->SetIntegerValue( "TLParamsLocked", 1 );

    syslog(LOG_DAEMON|LOG_INFO,"Resetting timestamp counter..." );
    lDeviceParams->ExecuteCommand( "GevTimestampControlReset" );

    // The pipeline is already "armed", we just have to tell the device
    // to start sending us images
    syslog(LOG_DAEMON|LOG_INFO,"Sending StartAcquisition command to device");
    lDeviceParams->ExecuteCommand( "AcquisitionStart" );

    PvInt64 lImageCountVal = 0;
    double lFrameRateVal = 0.0;
    double lBandwidthVal = 0.0;
    PvInt64 lPipelineBlocksDropped = 0;

    // Pipeline loop
    PvUInt32 lWidth=0, lHeight=0;
    do{
        // Retrieve next buffer
        PvBuffer *lBuffer = NULL;
        PvResult  lOperationResult;
        PvResult lResult = lPipeline.RetrieveNextBuffer( &lBuffer, 1000, &lOperationResult );

        if ( lResult.IsOK() && lOperationResult.IsOK() )
        {
            // Update Image Resolution In Case It Changes
            if(lBuffer->GetPayloadType() == PvPayloadTypeImage){
                PvImage *lImage = lBuffer->GetImage();
                lWidth = lImage->GetWidth();
                lHeight = lImage->GetHeight();
            }

            if(lWidth > 0 && lHeight > 0){
                listenersMutex.lock();
                // Pass buffer to each listener in the vector
                for(unsigned int i=0; i < listeners.size(); ++i){
                    listeners[i]->processFrame(lWidth, lHeight, lBuffer);
                }
                listenersMutex.unlock();
            }

            // If there are stats listeners, send them the current statistics
            if(statsListeners.size() > 0){
                Spyder3Stats stats;
                lStreamParams->GetIntegerValue( "ImagesCount", stats.imageCount);
                lStreamParams->GetFloatValue( "AcquisitionRateAverage",stats.frameRate);
                lStreamParams->GetFloatValue( "BandwidthAverage", stats.bandwidth);
                lStreamParams->GetIntegerValue("PipelineBlocksDropped", stats.framesDropped);

                statsListenerMutex.lock();
                for(unsigned int i=0; i < statsListeners.size(); ++i){
                    statsListeners[i]->processStats(&stats);
                }
                statsListenerMutex.unlock();
            }
        }

        lPipeline.ReleaseBuffer( lBuffer );
        runMutex.lock();
        running = isRunning;
        runMutex.unlock();
    }while(running); 

    // Housekeeping!
    // Tell the device to stop sending images
    syslog(LOG_DAEMON|LOG_INFO,"Sending AcquisitionStop command to the device");
    lDeviceParams->ExecuteCommand( "AcquisitionStop" );

    // If present reset TLParamsLocked to 0. Must be done AFTER the
    // streaming has been stopped
    lDeviceParams->SetIntegerValue( "TLParamsLocked", 0 );

    // We stop the pipeline - letting the object lapse out of
    // scope would have had the destructor do the same, but we do it anyway
    syslog(LOG_DAEMON|LOG_INFO,"Stop pipeline");
    lPipeline.Stop();

    // Now close the stream. Also optionnal but nice to have
    syslog(LOG_DAEMON|LOG_INFO,"Closing stream");
    lStream.Close();

    // Finally disconnect the device. Optional, still nice to have
    syslog(LOG_DAEMON|LOG_INFO,"Disconnecting device");
    lDevice.Disconnect();
}
