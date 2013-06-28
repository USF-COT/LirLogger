
#ifndef ISPYDER3STATSLISTENER_HPP
#define ISPYDER3STATSLISTENER_HPP

#include <PvTypes.h>
#include <string>

typedef struct{
    time_t time;
    unsigned int cameraID;
    PvInt64 imageCount;
    PvInt64 framesDropped;
    double frameRate;
}Spyder3Stats;

class ISpyder3StatsListener{
    public:
        virtual ~ISpyder3StatsListener(){}
        virtual void processStats(const Spyder3Stats& stats) = 0;
};

#endif
