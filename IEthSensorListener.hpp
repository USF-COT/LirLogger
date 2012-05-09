
#ifndef IETHSENSORLISTENER_HPP
#define IETHSENSORLISTENER_HPP

#include <vector>
#include <string.h>

using namespace std;

typedef struct{
    const string field;
    const bool isNum;
    string text;
    double num;
}EthSensorReading;

class IEthSensorListener{
    public:
        virtual ~IEthSensorListener(){}
        virtual void processReading(const std::vector<EthSensorReading>* readings) = 0;
};

#endif
