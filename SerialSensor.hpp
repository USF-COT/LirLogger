
#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <string.h>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "ISensor.hpp"
#include "ISensorListener.hpp"

using namespace std;

class SerialSensor : public ISensor{

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
    string port_string;
    int baud;
    boost::asio::serial_port* serial;
    boost::asio::io_service ios;
    boost::asio::streambuf buf;

    boost::thread* readThread;

    void setRunning(bool);
    void parseLine(const boost::system::error_code& ec, size_t bytes_transferred);

public:
    SerialSensor(const unsigned int _sensorID, const string _port, const int _baud, const string _name, const string _lineEnd, const string _delimeter, const vector<FieldDescriptor> _fields, const string _startChars, const string _endChars);
    ~SerialSensor();
    virtual bool Connect();
    virtual bool Disconnect();
    virtual void addListener(ISensorListener *l);
    virtual void clearListeners();
    virtual bool isRunning();
    void operator() ();
    virtual unsigned int getID();
    virtual string getName();
    virtual vector<FieldDescriptor> getFieldDescriptors();
};

#endif
