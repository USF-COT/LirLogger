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

#ifdef _UNIX_

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

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

#define _getch getchar

#endif // _UNIX_


#include <PvDeviceFinderWnd.h>
#include <PvDevice.h>
#include <PvPipeline.h>
#include <PvBuffer.h>
#include <PvStream.h>
#include <PvStreamRaw.h>

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
}

//
// Shows how to use a PvPipeline object to acquire images from a device
//

bool AcquireImages()
{
    // Create a GEV Device finder dialog
    PvDeviceFinderWnd lDeviceFinderWnd;

    // Prompt the user to select a GEV Device
    lDeviceFinderWnd.ShowModal();

    // Get the connectivity information for the selected GEV Device
    PvDeviceInfo* lDeviceInfo = lDeviceFinderWnd.GetSelected();

    // If no device is selected, abort
    if( lDeviceInfo == NULL )
    {
        printf( "No device selected.\n" );
        return false;
    }

    // Connect to the GEV Device
    PvDevice lDevice;
    printf( "Connecting to %s\n", lDeviceInfo->GetMACAddress().GetAscii() );
    if ( !lDevice.Connect( lDeviceInfo ).IsOK() )
    {
        printf( "Unable to connect to %s\n", lDeviceInfo->GetMACAddress().GetAscii() );
        return false;
    }
    printf( "Successfully connected to %s\n", lDeviceInfo->GetMACAddress().GetAscii() );
    printf( "\n" );

    // Get device parameters need to control streaming
    PvGenParameterArray *lDeviceParams = lDevice.GetGenParameters();

    // Negotiate streaming packet size
    lDevice.NegotiatePacketSize();

    // Create the PvStream object
    PvStream lStream;

    // Open stream - have the PvDevice do it for us
    printf( "Opening stream to device\n" );
    lStream.Open( lDeviceInfo->GetIPAddress() );

    // Create the PvPipeline object
    PvPipeline lPipeline( &lStream );
    
    // Reading payload size from device
    PvInt64 lSize = 0;
	lDeviceParams->GetIntegerValue( "PayloadSize", lSize );

    // Set the Buffer size and the Buffer count
    lPipeline.SetBufferSize( static_cast<PvUInt32>( lSize ) );
    lPipeline.SetBufferCount( 16 ); // Increase for high frame rate without missing block IDs

    // Have to set the Device IP destination to the Stream
    lDevice.SetStreamDestination( lStream.GetLocalIPAddress(), lStream.GetLocalPort() );

    // IMPORTANT: the pipeline needs to be "armed", or started before 
    // we instruct the device to send us images
    printf( "Starting pipeline\n" );
    lPipeline.Start();

    // Get stream parameters/stats
    PvGenParameterArray *lStreamParams = lStream.GetParameters();

    // TLParamsLocked is optional but when present, it MUST be set to 1
    // before sending the AcquisitionStart command
	lDeviceParams->SetIntegerValue( "TLParamsLocked", 1 );

    printf( "Resetting timestamp counter...\n" );
	lDeviceParams->ExecuteCommand( "GevTimestampControlReset" );

    // The pipeline is already "armed", we just have to tell the device
    // to start sending us images
    printf( "Sending StartAcquisition command to device\n" );
	lDeviceParams->ExecuteCommand( "AcquisitionStart" );

    char lDoodle[] = "|\\-|-/";
    int lDoodleIndex = 0;
    PvInt64 lImageCountVal = 0;
    double lFrameRateVal = 0.0;
    double lBandwidthVal = 0.0;

    // Setup FFMPEG Encoder
    AVCodec *codec;
    codec = avcodec_find_encoder(CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    // Acquire images until the user instructs us to stop
    printf( "\n<press the enter key to stop streaming>\n" );
    while ( !_kbhit() )
    {
        // Retrieve next buffer		
        PvBuffer *lBuffer = NULL;
        PvResult  lOperationResult;
        PvResult lResult = lPipeline.RetrieveNextBuffer( &lBuffer, 1000, &lOperationResult );
        
        if ( lResult.IsOK() )
        {
            if ( lOperationResult.IsOK() )
            {
				lStreamParams->GetIntegerValue( "ImagesCount", lImageCountVal );
				lStreamParams->GetFloatValue( "AcquisitionRateAverage", lFrameRateVal );
				lStreamParams->GetFloatValue( "BandwidthAverage", lBandwidthVal );
            
				// If the buffer contains an image, display width and height
				PvUInt32 lWidth = 0, lHeight = 0;
				if ( lBuffer->GetPayloadType() == PvPayloadTypeImage )
				{
					// Get image specific buffer interface
					PvImage *lImage = lBuffer->GetImage();

					// Read width, height
					lWidth = lBuffer->GetImage()->GetWidth();
					lHeight = lBuffer->GetImage()->GetHeight();
				}

				printf( "%c Timestamp: %016llX BlockID: %04X W: %i H: %i %.01f FPS %.01f Mb/s\r", 
                    lDoodle[ lDoodleIndex ],
                    lBuffer->GetTimestamp(),
                    lBuffer->GetBlockID(),
					lWidth,
					lHeight,
                    lFrameRateVal,
                    lBandwidthVal / 1000000.0 ); 
            }
            // We have an image - do some processing (...) and VERY IMPORTANT,
            // release the buffer back to the pipeline
            lPipeline.ReleaseBuffer( lBuffer );
        }
        else
        {
            // Timeout
            printf( "%c Timeout\r", lDoodle[ lDoodleIndex ] );
        }

        ++lDoodleIndex %= 6;
    }

    _getch(); // Flush key buffer for next stop
    printf( "\n\n" );

    // Tell the device to stop sending images
    printf( "Sending AcquisitionStop command to the device\n" );
	lDeviceParams->ExecuteCommand( "AcquisitionStop" );

    // If present reset TLParamsLocked to 0. Must be done AFTER the 
    // streaming has been stopped
	lDeviceParams->SetIntegerValue( "TLParamsLocked", 0 );

    // We stop the pipeline - letting the object lapse out of 
    // scope would have had the destructor do the same, but we do it anyway
    printf( "Stop pipeline\n" );
    lPipeline.Stop();

    // Now close the stream. Also optionnal but nice to have
    printf( "Closing stream\n" );
    lStream.Close();

    // Finally disconnect the device. Optional, still nice to have
    printf( "Disconnecting device\n" );
    lDevice.Disconnect();

    return true;
}


//
// Main function
//

int main()
{
    // PvPipeline used to acquire images from a device
    printf( "1. PvPipeline sample - image acquisition from a device\n\n" );
    AcquireImages();

    printf( "\n<press the enter key to exit>\n" );
    while ( !_kbhit() );

    return 0;
}
