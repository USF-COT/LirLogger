
#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <string.h>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "ISensorListener.hpp"

using namespace std;

typedef struct{
    unsigned int id;
    bool isNum;
    string name;
    string units;
}FieldDescriptor;

class Sensor{    

private:
    unsigned int sensorID;

    string name;
    string lineEnd;
    string delimeter;
    vector<FieldDescriptor> fields;
    string startChars;
    string endChars;
    vector<ISensorListener *> listeners;

    boost::mutex runMutex;
    boost::mutex listenersMutex;
    bool running;

    // Socket Variables
    boost::asio::io_service ios;
    boost::asio::streambuf buf;

    boost::thread* readThread;

    void setRunning(bool);
    void parseLine(const boost::system::error_code& ec, size_t bytes_transferred);

public:
    EthSensor(const unsigned int _sensorID, const string IP, const unsigned int port, const string _name, const string _lineEnd, const string _delimeter, const vector<FieldDescriptor> _fields, const string _startChars, const string _endChars);
    ~EthSensor();
    bool Connect();
    bool Disconnect();
    void addListener(IEthSensorListener *l);
    void clearListeners();
    bool isRunning();
    void operator() ();
    unsigned int getID();
    string getName();
    vector<FieldDescriptor> getFieldDescriptors();
};

#endif
