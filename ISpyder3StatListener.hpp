
#ifndef ISPYDER3STATLISTENER_HPP
#define ISPYDER3STATLISTENER_HPP

#include <PvBuffer.h>

class ISpyder3StatListener{
    public:
        virtual ~ISpyder3StatListener(){}
        virtual void processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer) = 0;
};

#endif
