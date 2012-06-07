// *****************************************************************************
//
//     Copyright (c) 2008, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

//
// To receive images using PvPipeline
//

#ifdef WIN32
#include <windows.h>
#include <conio.h>
#include <process.h>
#endif // WIN32

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _UNIX_

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>

#define MAXFILEPATH 1024

int _kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

typedef struct CAMINFO{
    char* MACAddress;
    char* filename;
    char* prefix;
}CamInfo;

volatile sig_atomic_t running = 1;

#define _getch getchar

#endif // _UNIX_

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

/*
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 37888000

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
}
 */

extern "C"
{
#include <tiffio.h>
}

//
// Shows how to use a PvPipeline object to acquire images from a device
//

bool AcquireImages(CamInfo* cams, int numCams)
{
    int i;
    PvResult result;

    // Initialize Camera System
    PvSystem system;
    system.SetDetectionTimeout(2000);
    result = system.Find();    
    if(!result.IsOK()){
        printf("PvSystem::Find Error: %s", result.GetCodeString().GetAscii());
        return false;
    }

    PvDevice lDevice[numCams];
    PvGenParameterArray *lDeviceParams[numCams];
    PvStream lStream[numCams];
    PvPipeline *lPipeline[numCams];
    for(i=0; i < numCams; i++){
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
                if(strlen(cams[i].MACAddress) == strlen(tempInfo->GetMACAddress().GetAscii()) && strncmp(cams[i].MACAddress,tempInfo->GetMACAddress().GetAscii(),strlen(cams[i].MACAddress)) == 0){
                    lDeviceInfo = tempInfo;
                    break;
                }
            }
        }

        // If no device is selected, abort
        if( lDeviceInfo == NULL )
        {
            printf( "No device selected.\n" );
            return false;
        }

        // Connect to the GEV Device
        printf( "Connecting to %s\n", lDeviceInfo->GetMACAddress().GetAscii() );
        if ( !lDevice[i].Connect( lDeviceInfo ).IsOK() )
        {
            printf( "Unable to connect to %s\n", lDeviceInfo->GetMACAddress().GetAscii() );
            return false;
        }
        printf( "Successfully connected to %s\n", lDeviceInfo->GetMACAddress().GetAscii() );
        printf( "\n" );

        // Get device parameters need to control streaming
        lDeviceParams[i] = lDevice[i].GetGenParameters();

        // Negotiate streaming packet size
        lDevice[i].NegotiatePacketSize();

        // Open stream - have the PvDevice do it for us
        printf( "Opening stream to device\n" );
        lStream[i].Open( lDeviceInfo->GetIPAddress() );

        // Create the PvPipeline object
        lPipeline[i] = new PvPipeline( &lStream[i] );

        // Reading payload size from device
        PvInt64 lSize = 0;
        lDeviceParams[i]->GetIntegerValue( "PayloadSize", lSize );

        // Set the Buffer size and the Buffer count
        lPipeline[i]->SetBufferSize( static_cast<PvUInt32>( lSize ) );
        lPipeline[i]->SetBufferCount( 16 ); // Increase for high frame rate without missing block IDs

        // Have to set the Device IP destination to the Stream
        lDevice[i].SetStreamDestination( lStream[i].GetLocalIPAddress(), lStream[i].GetLocalPort() );
    }

    PvGenParameterArray *lStreamParams[numCams];
    for(i=0; i < numCams; i++){
        // IMPORTANT: the pipeline needs to be "armed", or started before
        // we instruct the device to send us images
        printf( "Starting pipeline %d\n",i);
        lPipeline[i]->Start();

        // Get stream parameters/stats
        lStreamParams[i] = lStream[i].GetParameters();

        // TLParamsLocked is optional but when present, it MUST be set to 1
        // before sending the AcquisitionStart command
        lDeviceParams[i]->SetIntegerValue( "TLParamsLocked", 1 );

        printf( "Resetting timestamp counter...\n" );
        lDeviceParams[i]->ExecuteCommand( "GevTimestampControlReset" );

        // The pipeline is already "armed", we just have to tell the device
        // to start sending us images
        printf( "Sending StartAcquisition command to device\n" );
        lDeviceParams[i]->ExecuteCommand( "AcquisitionStart" );
    }

    char lDoodle[] = "|\\-|-/";
    int lDoodleIndex = 0;
    PvInt64 lImageCountVal = 0;
    double lFrameRateVal = 0.0;
    double lBandwidthVal = 0.0;
    PvInt64 lPipelineBlocksDropped = 0;

    // Acquire images until the user instructs us to stop
    printf( "\n<press the enter key to stop streaming>\n" );
    //PvBufferWriter writer;
    char filePath[MAXFILEPATH];
    PvUInt32 lWidth = 4096, lHeight = 9250;
    while ( running )
    {
        for(i=0; i < numCams; i++){
            // Retrieve next buffer		
            PvBuffer *lBuffer = NULL;
            PvResult  lOperationResult;
            PvResult lResult = lPipeline[i]->RetrieveNextBuffer( &lBuffer, 1000, &lOperationResult );

            if ( lResult.IsOK() )
            {
                if ( lOperationResult.IsOK() )
                {
                    lStreamParams[i]->GetIntegerValue( "ImagesCount", lImageCountVal );
                    lStreamParams[i]->GetFloatValue( "AcquisitionRateAverage", lFrameRateVal );
                    lStreamParams[i]->GetFloatValue( "BandwidthAverage", lBandwidthVal );
                    lStreamParams[i]->GetIntegerValue("PipelineBlocksDropped", lPipelineBlocksDropped);

                    filePath[0] = '\0';
                    sprintf(filePath,"%s/%s%04X.tif",cams[i].filename,cams[i].prefix,lBuffer->GetBlockID());
                    TIFF *out = TIFFOpen(filePath,"w");
                    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, lWidth);
                    TIFFSetField(out, TIFFTAG_IMAGELENGTH, lHeight);
                    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
                    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
                    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
                    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
                    TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

                    TIFFWriteEncodedStrip(out,0,lBuffer->GetDataPointer(),lWidth*lHeight);
                    TIFFClose(out);

                    printf( "%d:%s%c Timestamp: %016llX BlockID: %04X %.01f FPS %d DROP %.01f Mb/s\r\n",i,
                            cams[i].prefix,
                            lDoodle[ lDoodleIndex ],
                            lBuffer->GetTimestamp(),
                            lBuffer->GetBlockID(),
                            lFrameRateVal,
                            lPipelineBlocksDropped,
                            lBandwidthVal / 1000000.0 );
                } else {
                    printf("%d: ERROR Code: %s, Description: %s\r\n",i,lOperationResult.GetCodeString().GetAscii(),lOperationResult.GetDescription().GetAscii());
                }
                // We have an image - do some processing (...)
                // VERY IMPORTANT:
                // release the buffer back to the pipeline
                lPipeline[i]->ReleaseBuffer( lBuffer );
                usleep(200);
            }
            else
            {
                // Timeout
                printf( "%d:%s%c Timeout\r\n",i, cams[i].prefix,lDoodle[ lDoodleIndex ]);
            }
        }
        ++lDoodleIndex %= 6;
    }

    //_getch(); // Flush key buffer for next stop
    printf( "\n\n" );

    for(i=0; i < numCams; i++){
        // Tell the device to stop sending images
        printf( "Sending AcquisitionStop command to the device\n" );
        lDeviceParams[i]->ExecuteCommand( "AcquisitionStop" );

        // If present reset TLParamsLocked to 0. Must be done AFTER the 
        // streaming has been stopped
        lDeviceParams[i]->SetIntegerValue( "TLParamsLocked", 0 );

        // We stop the pipeline - letting the object lapse out of 
        // scope would have had the destructor do the same, but we do it anyway
        printf( "Stop pipeline\n" );
        lPipeline[i]->Stop();
        delete lPipeline[i];

        // Now close the stream. Also optionnal but nice to have
        printf( "Closing stream\n" );
        lStream[i].Close();

        // Finally disconnect the device. Optional, still nice to have
        printf( "Disconnecting device\n" );
        lDevice[i].Disconnect();
    }

    return true;
}

void *RunCam(void* tInfo){
    CamInfo* info = (CamInfo*)tInfo;

    //    AcquireImages(info->MACAddress,info->filename,info->prefix);
}


//
// Main function
//

int main(int argc, char** argv)
{
    int i;

    if(argc != 5){
        fprintf(stderr,"Incorrect number of arguments passed.\nEXAMPLE: ./LirTest \"<Camera 1 MAC Address>\" \"<encoder 1 output file path>\" \"<Camera 2 MAC Address>\" \"<encoder 2 output file path>\"\n");
        exit(EXIT_FAILURE);
    }

    pthread_t thread[2];
    pthread_attr_t attr;
    CamInfo camInfo[2];
    running = 1;

    camInfo[0].MACAddress = argv[1];
    camInfo[0].filename = argv[2];
    camInfo[0].prefix = "CAM1-";
    camInfo[1].MACAddress = argv[3];
    camInfo[1].filename = argv[4];
    camInfo[1].prefix = "CAM2-";

    /*
       pthread_attr_init(&attr);
       pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

       for(i=0; i < 2; i++){
       pthread_create(&thread[i],&attr,RunCam,(void*)&camInfo[i]);
       }
     */

    AcquireImages(camInfo,2);

    printf( "\n<press the enter key to exit>\n" );
    while ( !_kbhit() );

    running = 0;

    /*
       pthread_attr_destroy(&attr);
       for(i=0; i < 2; i++){
       pthread_join(thread[i],NULL);
       }
     */
    return 0;
}

