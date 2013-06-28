
#include "Spyder3JPEGWriter.hpp"

#include <stdio.h>

extern "C"{
#include <jpeglib.h>
}

#include <string>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <limits.h>

#include <boost/filesystem.hpp>

#define MAXFILESPERFOLDER 1000 // Files to write to a folder before rolling over to the next folder

bool Spyder3JPEGWriter::setNextFolderPath(){
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

Spyder3JPEGWriter::Spyder3JPEGWriter(string _outputFolder) : outputFolderPath(_outputFolder){
    folderID = 0;
    frameID = 0;

    setNextFolderPath();
}

Spyder3JPEGWriter::~Spyder3JPEGWriter(){
}

void Spyder3JPEGWriter::changeFolder(string folderPath){
    idMutex.lock();
    outputFolderPath = folderPath;
    this->folderID = 0;
    this->frameID = 0;
    idMutex.unlock();

    setNextFolderPath();
}

std::pair<unsigned long, unsigned long> Spyder3JPEGWriter::getFolderFrameIDs(){
    std::pair<unsigned long, unsigned long> retVal;
    idMutex.lock();
    retVal.first = folderID;
    retVal.second = frameID;
    idMutex.unlock();
    return retVal;
}

void Spyder3JPEGWriter::processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer){
    if(frameID >= MAXFILESPERFOLDER){
        if(!setNextFolderPath()){
            syslog(LOG_DAEMON|LOG_ERR, "SEVERE: Cannot log any more frames, maximum number of folders and frames reached.");
            return;
        }
    }

    stringstream path;
    idMutex.lock();
    path << outputFolderPath << "/" << folderID << "/" << folderID << "-" << ++frameID << ".jpg";
    idMutex.unlock();

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE* outfile;
    JSAMPROW row_pointer[1];
    int row_stride;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);

    if((outfile = fopen(path.str().c_str(), "wb")) == NULL){
        syslog(LOG_DAEMON|LOG_ERR, "Unable to open jpeg file at %s", path.str().c_str());
        return;
    }
    
    jpeg_stdio_dest(&cinfo, outfile);
    cinfo.image_width = lWidth;
    cinfo.image_height = lHeight;
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 80, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    row_stride = lWidth;

    const void* image_buffer = lBuffer->GetDataPointer();

    while(cinfo.next_scanline < cinfo.image_height){
        row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
}
