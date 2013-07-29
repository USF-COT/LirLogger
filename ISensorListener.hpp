
#ifndef ISENSORLISTENER_HPP
#define ISENSORLISTENER_HPP

#include <map>
#include <string>
#include <time.h>

using namespace std;

typedef struct{
    unsigned int fieldID;
    string field;
    bool isNum;
    string text;
    double num;
}SensorReading;

typedef struct{
    time_t time;
    unsigned int sensorID;
    string sensorName;
    vector<SensorReading> readings;
}SensorReadingSet;

class ISensorListener{
    public:
        virtual ~ISensorListener(){}
        virtual void sensorStarting() = 0; // Allows the listener to setup anything necessary on io thread
        virtual void processReading(const SensorReadingSet& set) = 0; // readings are passed from io thred to this function
        virtual void sensorStopping() = 0; // Allows the listener to close anything setup on the io thread
};

#endif
