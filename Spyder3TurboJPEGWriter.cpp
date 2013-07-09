
#include "Spyder3TurboJPEGWriter.hpp"

#include <stdio.h>

#include <string>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <limits.h>

Spyder3TurboJPEGWriter::Spyder3TurboJPEGWriter(string outputFolderPath):super(outputFolderPath, 1000){
    compressor = tjInitCompress();
    jpegBufferSize = 8388608;
    jpegBuffer = tjAlloc(8388608);
}

Spyder3TurboJPEGWriter::~Spyder3TurboJPEGWriter(){
    tjDestroy(compressor);
    tjFree(jpegBuffer);
}

void Spyder3TurboJPEGWriter::processFrame(unsigned long cameraID, PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer){
    super::processFrame(cameraID, lWidth, lHeight, lBuffer);

    FILE * outfile;

    unsigned char* buffer = (unsigned char*)lBuffer->GetDataPointer();

    int compressSuccess = tjCompress2(compressor,
                                      buffer, lWidth, lWidth, lHeight,
                                      TJPF_GRAY, &jpegBuffer, &jpegBufferSize,
                                      TJSAMP_GRAY, 100, 0);

    if(compressSuccess == 0){
        string path = this->getNextImagePath("jpg");
        if((outfile = fopen(path.c_str(), "wb")) != NULL){
            fwrite(jpegBuffer, 1, jpegBufferSize, outfile);
            fclose(outfile);
            //this->updateStatsListeners(jpegBufferSize);
        } else {
            syslog(LOG_DAEMON|LOG_ERR, "Can't open file @ %s", path.c_str());
        }
    } else {
        syslog(LOG_DAEMON|LOG_INFO, "TurboJPEG compression unsuccessful: %s", tjGetErrorStr());
    }
}
