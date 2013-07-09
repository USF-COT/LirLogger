
#ifndef ISPYDER3LISTENER_HPP
#define ISPYDER3LISTENER_HPP

#include <PvTypes.h>
#include <PvBuffer.h>

class ISpyder3Listener{
    public:
        virtual ~ISpyder3Listener(){}
        virtual void processFrame(unsigned long cameraID, PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer) = 0;
};

#endif
