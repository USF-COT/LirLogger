
#ifndef ISENSOR_HPP
#define ISENSOR_HPP

#include <string.h>
#include <vector>
#include <iostream>

#include "ISensorListener.hpp"

using namespace std;

typedef struct{
    unsigned int id;
    bool isNum;
    string name;
    string units;
}FieldDescriptor;

class ISensor{
public:
    virtual ~ISensor(){}
    virtual bool Connect() = 0;
    virtual bool Disconnect() = 0;
    virtual void addListener(ISensorListener *l) = 0;
    virtual void clearListeners() = 0;
    virtual bool isRunning() = 0;
    virtual unsigned int getID() = 0;
    virtual string getName() = 0;
    virtual vector<FieldDescriptor> getFieldDescriptors() = 0;
};

#endif
