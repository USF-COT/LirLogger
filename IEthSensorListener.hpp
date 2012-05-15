
#ifndef IETHSENSORLISTENER_HPP
#define IETHSENSORLISTENER_HPP

#include <vector>
#include <string>
#include <time.h>

using namespace std;

typedef struct{
    string field;
    bool isNum;
    string text;
    double num;
}EthSensorReading;

class IEthSensorListener{
    public:
        virtual ~IEthSensorListener(){}
        virtual void sensorStarting() = 0; // Allows the listener to setup anything necessary on io thread
        virtual void processReading(time_t time, const vector<EthSensorReading>& readings) = 0; // readings are passed from io thred to this function
        virtual void sensorStopping() = 0; // Allows the listener to close anything setup on the io thread
};

#endif
