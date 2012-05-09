
#include "Spyder3TiffWriter.hpp"

extern "C"{
#include <tiffio.h>
}

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <limits.h>

#define MAXFILESPERFOLDER 1200 // Files to write to a folder before rolling over to the next folder

Spyder3TiffWriter::Spyder3TiffWriter(char* _outputFolder){
    outputFolderPath = (char*) malloc(sizeof(char)*(strlen(_outputFolder)+1));
    strncpy(outputFolderPath,_outputFolder,strlen(_outputFolder)+1);

    fullPath = (char*) malloc(sizeof(char)*(strlen(_outputFolder)+1024));

    folderID = 0;
    frameID = 0;

    do{
        sprintf(fullPath,"%s/%d",outputFolderPath,++folderID);
    } while(mkdir(fullPath,0775) != 0 && folderID < ULONG_MAX);

    if(folderID == ULONG_MAX)
        syslog(LOG_DAEMON|LOG_ERR, "Unable to log in this output folder because %d folders already found.",ULONG_MAX);
}

Spyder3TiffWriter::~Spyder3TiffWriter(){
    free(outputFolderPath);
    free(fullPath);
}

void Spyder3TiffWriter::processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer){
    if(frameID >= MAXFILESPERFOLDER){
        frameID=0;

        unsigned long lastFolderID = folderID-1;
        do{
            sprintf(fullPath,"%s/%d",outputFolderPath,++folderID);
        } while(mkdir(fullPath,0775) != 0 && folderID < ULONG_MAX);

        if(folderID == ULONG_MAX){
            syslog(LOG_DAEMON|LOG_ERR, "Unable to log in this output folder because %d folders already found.",ULONG_MAX);
            return;
        }
    }

    fullPath[0] = '\0';
    sprintf(fullPath,"%s/%d/%d-%d.tif",outputFolderPath,folderID,folderID,++frameID);
    syslog(LOG_DAEMON|LOG_INFO,"Writing Tiff to File @ %s.",fullPath);

    TIFF *out = TIFFOpen(fullPath,"w");
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, lWidth);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, lHeight);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

    TIFFWriteEncodedStrip(out,0,(void*)lBuffer->GetDataPointer(),lWidth*lHeight);
    TIFFClose(out);
}
