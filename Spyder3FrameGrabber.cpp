/* Spyder3 Frame Grabber
 * - Wraps pleora eBUS SDK drivers into a smaller library that has just a few, Lir-Specific calls.
 *
 * By: Michael Lindemuth
 */

#include "Spyder3FrameGrabber.h"

/*
struct Sp3GrabThreadParams{
    volatile sig_atomic_t running;
    volatile sig_atomic_t stopped;
    std::string configPath;
    PvDevice device;
    FrameWriteBuffer* writeBuffer;
};

class Spyder3FrameGrabber{
    private:
        Sp3GrabThreadParams runParams;
        pthread_t runner;

    public:
        Spyder3FrameGrabber(std::string path, FrameWriteBuffer *frameQueue);
        bool start();
        bool stop();
        ~Spyder3FrameGrabber();
        void ReQueueBuffer(PvBuffer* buffer);
};
*/

void initStream(Sp3GrabThreadParams *params){
    pthread_mutex_lock(&params->streamMutex);

    // Negotiate streaming packet size
    params->device.NegotiatePacketSize();

    // Open stream - have the PvDevice do it for us
    printf( "Opening stream to device\n" );
    params->stream.Open( params->devInfo->GetIPAddress() );

    // Reading payload size from device
    PvGenParameterArray *lDeviceParams = params->device.GetGenParameters();
    PvGenInteger *lPayloadSize = dynamic_cast<PvGenInteger *>( lDeviceParams->Get( "PayloadSize" ) );
    PvInt64 lSize = 0;
    lPayloadSize->GetValue( lSize );

    // Use buffer count from Frame Write Queue 
    PvUInt32 lBufferCount = ( params->stream.GetQueuedBufferMaximum() < params->writeBuffer->getLimit() ) ?
        params->stream.GetQueuedBufferMaximum() :
        params->writeBuffer->getLimit();

    // Create, alloc buffers
    PvBuffer *lBuffers = new PvBuffer[ lBufferCount ];
    for ( PvUInt32 i = 0; i < lBufferCount; i++ )
    {
        lBuffers[ i ].Alloc( static_cast<PvUInt32>( lSize ) );
    }

    // Have to set the Device IP destination to the Stream
    params->device.SetStreamDestination( params->stream.GetLocalIPAddress(), params->stream.GetLocalPort() );

    // Get stream parameters/stats
    PvGenParameterArray *lStreamParams = params->stream.GetParameters();
    params->imageCount = dynamic_cast<PvGenInteger *>( lStreamParams->Get( "ImagesCount" ) );
    params->frameRate = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "AcquisitionRateAverage" ) );
    params->bandwidth = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "BandwidthAverage" ) );

    // Queue all buffers in the stream
    for ( PvUInt32 i = 0; i < lBufferCount; i++ )
    {
        params->stream.QueueBuffer( lBuffers + i );
    }

    pthread_mutex_unlock(&params->streamMutex);
}

void *grabFrames(void* runParams){
    Sp3GrabThreadParams *params = (Sp3GrabThreadParams *) runParams;
    params->running = true;
    params->stopped = false;

    // Initialize PvStream
    initStream(params);

    // Get device parameters need to control streaming
    PvGenParameterArray *lDeviceParams = params->device.GetGenParameters();
    PvGenInteger *lTLLocked = dynamic_cast<PvGenInteger *>( lDeviceParams->Get( "TLParamsLocked" ) );
    PvGenCommand *lStart = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
    PvGenCommand *lStop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );

    // TLParamsLocked is optional but when present, it MUST be set to 1
    // before sending the AcquisitionStart command
    if ( lTLLocked != NULL )
    {
        lTLLocked->SetValue( 1 );
    }

    PvGenCommand *lResetTimestamp = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "GevTimestampControlReset" ) );
    lResetTimestamp->Execute();

    // The buffers are queued in the stream, we just have to tell the device
    // to start sending us images
    PvResult lResult = lStart->Execute();
    
    while(!params->stopped){
        PvBuffer *lBuffer = NULL;
        PvResult lOperationResult;

        // Retrieve next buffer
        PvResult lResult = params->stream.RetrieveBuffer( &lBuffer, &lOperationResult, 1000 );
        if ( lResult.IsOK() )
        {
                // Queue image for thread safe write shared buffer 
                pthread_mutex_lock(&(params->streamMutex));
                params->writeBuffer->enqueue(lBuffer);
                pthread_mutex_unlock(&(params->streamMutex));
        }
    }

    // Tell the device to stop sending images
    lStop->Execute();

    // If present reset TLParamsLocked to 0. Must be done AFTER the
    // streaming has been stopped
    if ( lTLLocked != NULL )
    {
        lTLLocked->SetValue( 0 );
    }

    // Abort all buffers from the stream, unqueue
    pthread_mutex_lock(&(params->streamMutex));
    params->stream.AbortQueuedBuffers();
    while ( params->stream.GetQueuedBufferCount() > 0 )
    {
        PvBuffer *lBuffer = NULL;
        PvResult lOperationResult;

        params->stream.RetrieveBuffer( &lBuffer, &lOperationResult );
    }

    // Release buffers
    delete []lBuffers;

    // Now close the stream. Also optionnal but nice to have
    params->stream.Close();
    pthread_mutex_unlock(&(params->streamMutex));
    params->running = false;
}

bool findCamera(char* MACAddress, Sp3GrabThreadParams *params){
        PvResult lResult;
        PvDeviceInfo *lDeviceInfo = 0;

        // Create an GEV system and an interface.
        PvSystem lSystem;

        // Find all GEV Devices on the network.
        lSystem.SetDetectionTimeout( 2000 );
        lResult = lSystem.Find();
        if( !lResult.IsOK() )
        {
                printf( "PvSystem::Find Error: %s", lResult.GetCodeString().GetAscii() );
                return -1;
        }

        // Get the number of GEV Interfaces that were found using GetInterfaceCount.
        PvUInt32 lInterfaceCount = lSystem.GetInterfaceCount();

        // For each interface, check MAC Address against passed address 
        for( PvUInt32 x = 0; x < lInterfaceCount; x++ )
        {
                // get pointer to each of interface
                PvInterface * lInterface = lSystem.GetInterface( x );

                // Get the number of GEV devices that were found using GetDeviceCount.
                PvUInt32 lDeviceCount = lInterface->GetDeviceCount();

                for( PvUInt32 y = 0; y < lDeviceCount ; y++ )
                {
                        lDeviceInfo = lInterface->GetDeviceInfo( y );
                        if(strncmp(MACAddress,lDeviceInfo->GetMACAddress().GetAscii(),strlen(MACAddress)) == 0){
                            params->devInfo = lDeviceInfo;
                            lResult = params->device.Connect(lDeviceInfo);
                            if(lResult.IsOK())
                                return true;
                            else
                                return false;
                        }
                }
        }

        return false;

}

Spyder3FrameGrabber::Spyder3FrameGrabber(){
    pthread_mutex_init(&(runParams.streamMutex),NULL); 
    runParams.stopped = true;
    runParams.running = false;
}

bool Spyder3FrameGrabber::connect(char* MACAddress){
    return findCamera(MACAddress,&runParams);
}

bool Spyder3FrameGrabber::startStream(FrameWriteBuffer* frameQueue){
    if(!runParams.running){
        runParams.writeBuffer = frameQueue; 
        if(pthread_create(&runner,NULL,grabFrames, (void *)&runParams)){
            return true;
        } 
    }
    return false;
}

bool Spyder3FrameGrabber::stopStream(){
    int waitIntervals = 10;
    if(runParams.running){
        runParams.stopped = true;
    }
    // Try to wait for device to disconnect gracefully
    while(waitIntervals-- > 0 && runParams.running){
        sleep(1);
    }
    
    return waitIntervals > 0;
}

void Spyder3FrameGrabber::disconnect(){
    stopStream();
    runParams.device.Disconnect();
}

Spyder3FrameGrabber::~Spyder3FrameGrabber(){
    disconnect();
}

void Spyder3FrameGrabber::ReQueueBuffer(PvBuffer* buffer){
    pthread_mutex_lock(&(runParams.streamMutex));
    runParams.stream.QueueBuffer(buffer); 
    pthread_mutex_unlock(&(runParams.streamMutex));      
}

