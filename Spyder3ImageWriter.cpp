
#include "Spyder3ImageWriter.hpp"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <limits.h>

#include <boost/filesystem.hpp>

#define MAXFILESPERFOLDER 1000 // Files to write to a folder before rolling over to the next folder

bool Spyder3ImageWriter::setNextFolderPath(){
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

Spyder3ImageWriter::Spyder3ImageWriter(string _outputFolder, unsigned long _numImagesPerFolder) : outputFolderPath(_outputFolder), numImagesPerFolder(_numImagesPerFolder){
    cameraID = 0;
    folderID = 0;
    frameID = 0;

    setNextFolderPath();
}

Spyder3ImageWriter::~Spyder3ImageWriter(){
}

void Spyder3ImageWriter::changeFolder(string folderPath){
    idMutex.lock();
    outputFolderPath = folderPath;
    this->folderID = 0;
    this->frameID = 0;
    idMutex.unlock();

    setNextFolderPath();
}

unsigned long Spyder3ImageWriter::getFolderID(){
    unsigned long retVal;
    idMutex.lock();
    retVal = this->folderID;
    idMutex.unlock();
    return retVal;
}

unsigned long Spyder3ImageWriter::getFrameID(){
    unsigned long retVal;
    idMutex.lock();
    retVal = this->frameID;
    idMutex.unlock();
    return retVal;
}

string Spyder3ImageWriter::getNextImagePath(const string extension){
    stringstream path;
    idMutex.lock();
    path << outputFolderPath << "/" << folderID << "/" << folderID << "-" << ++frameID << "." << extension;
    this->nextImagePath = path.str();
    idMutex.unlock();

    return path.str();
}

void Spyder3ImageWriter::updateStatsListeners(unsigned long bytesWritten){
    Spyder3ImageWriterStats stats;
    stats.time = time(NULL);
    stats.cameraID = this->cameraID;
    idMutex.lock();
    stats.folderID = folderID;
    stats.frameID = frameID;
    idMutex.unlock();
    stats.numBytes = bytesWritten;

    vector<ISpyder3ImageWriterListener*>::iterator it;
    listenerMutex.lock();
    for(it=statsListeners.begin(); it != statsListeners.end(); ++it){
        (*it)->processStats(stats);
    }
    listenerMutex.unlock();
}

void Spyder3ImageWriter::processFrame(unsigned long cameraID, PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer){
    if(this->cameraID != cameraID)
        this->cameraID = cameraID;

    if(frameID >= this->numImagesPerFolder){
        if(!setNextFolderPath()){
            syslog(LOG_DAEMON|LOG_ERR, "SEVERE: Cannot log any more frames, maximum number of folders and frames reached.");
            return;
        }
    }
}

void Spyder3ImageWriter::addStatsListener(ISpyder3ImageWriterListener* l){
    listenerMutex.lock();
    statsListeners.push_back(l);
    listenerMutex.unlock();
}

void Spyder3ImageWriter::clearStatsListeners(){
    listenerMutex.lock();
    statsListeners.clear();
    listenerMutex.unlock();
}
