
#include "Spyder3TiffWriter.hpp"

extern "C"{
#include <tiffio.h>
}

#include <string>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <limits.h>

#include <boost/filesystem.hpp>

#define MAXFILESPERFOLDER 1000 // Files to write to a folder before rolling over to the next folder

bool Spyder3TiffWriter::setNextFolderPath(){
    string fullPath;

    bool created;

    idMutex.lock();
    frameID=0;
    do{
        stringstream path;
        path << outputFolderPath << "/" << ++folderID;
        fullPath = path.str();
        syslog(LOG_DAEMON|LOG_INFO,"Trying %s folder.",fullPath.c_str());
        created = boost::filesystem::create_directories(fullPath);
    } while(!created && folderID < ULONG_MAX);
    syslog(LOG_DAEMON|LOG_INFO,"Using %s folder",fullPath.c_str());
    idMutex.unlock();    

    if(folderID == ULONG_MAX){
        return false;
    }

    return true;
}

Spyder3TiffWriter::Spyder3TiffWriter(string _outputFolder) : outputFolderPath(_outputFolder){
    folderID = 0;
    frameID = 0;

    setNextFolderPath();
}

Spyder3TiffWriter::~Spyder3TiffWriter(){
}

void Spyder3TiffWriter::changeFolder(string folderPath){
    idMutex.lock();
    outputFolderPath = folderPath;
    this->folderID = 0;
    this->frameID = 0;
    idMutex.unlock();

    setNextFolderPath();
}

std::pair<unsigned long, unsigned long> Spyder3TiffWriter::getFolderFrameIDs(){
    std::pair<unsigned long, unsigned long> retVal;
    idMutex.lock();
    retVal.first = folderID;
    retVal.second = frameID;
    idMutex.unlock();
    return retVal;
}

void Spyder3TiffWriter::processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer){
    if(frameID >= MAXFILESPERFOLDER){
        if(!setNextFolderPath()){
            syslog(LOG_DAEMON|LOG_ERR, "SEVERE: Cannot log any more frames, maximum number of folders and frames reached.");
            return;
        }
    }

    stringstream path;
    idMutex.lock();
    path << outputFolderPath << "/" << folderID << "/" << folderID << "-" << ++frameID << ".tif";
    idMutex.unlock();
    //syslog(LOG_DAEMON|LOG_INFO,"Writing Tiff to File @ %s.",path.str().c_str());

    TIFF *out = TIFFOpen(path.str().c_str(),"w");
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, lWidth);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, lHeight);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

    TIFFWriteEncodedStrip(out,0,(void*)lBuffer->GetDataPointer(),lWidth*lHeight);
    TIFFClose(out);
}
