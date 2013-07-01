
#ifndef ISPYDER3IMAGEWRITERLISTENER_HPP
#define ISPYDER3IMAGEWRITERLISTENER_HPP

#include <time.h>

typedef struct{
    time_t time;
    unsigned long cameraID;
    unsigned long folderID;
    unsigned long frameID;
    unsigned long numBytes;
}Spyder3ImageWriterStats;

class ISpyder3ImageWriterListener{
    public:
        virtual ~ISpyder3ImageWriterListener(){}
        virtual void processStats(const Spyder3ImageWriterStats& stats) = 0;
};

#endif
