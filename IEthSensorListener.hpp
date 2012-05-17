
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

typedef struct{
    time_t time;
    string sensorName;
    vector <EthSensorReading> readings;
}EthSensorReadingSet;

class IEthSensorListener{
    public:
        virtual ~IEthSensorListener(){}
        virtual void sensorStarting() = 0; // Allows the listener to setup anything necessary on io thread
        virtual void processReading(const EthSensorReadingSet set) = 0; // readings are passed from io thred to this function
        virtual void sensorStopping() = 0; // Allows the listener to close anything setup on the io thread
};

#endif
