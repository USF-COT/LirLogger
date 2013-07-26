
#ifndef IFLOWMETERLISTENER_HPP
#define IFLOWMETERLISTENER_HPP

#include <map>
#include <string>
#include <time.h>

using namespace std;

class IFlowMeterListener{
    public:
        virtual ~IFlowMeterListener(){}
        virtual void flowMeterStarting() = 0; // Allows the listener to setup anything necessary on io thread
        virtual void processFlowReading(time_t timestamp, int count, double flowRate) = 0; // readings are passed from io thread to this function
        virtual void flowMeterStopping() = 0; // Allows the listener to close anything setup on the io thread
};

#endif
