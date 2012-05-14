
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
        virtual void processReading(time_t time, const vector<EthSensorReading>& readings) = 0;
};

#endif
