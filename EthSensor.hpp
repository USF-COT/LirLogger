
#ifndef IETHSENSOR_HPP
#define IETHSENSOR_HPP

#include <string.h>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "ISensor.hpp"
#include "ISensorListener.hpp"

using namespace std;

class EthSensor : public ISensor{

private:
    unsigned int sensorID;
    string IP;
    unsigned int port;
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
    boost::asio::ip::tcp::socket* readSock;
    boost::asio::streambuf buf;

    boost::thread* readThread;

    void setRunning(bool);
    void parseLine(const boost::system::error_code& ec, size_t bytes_transferred);

public:
    EthSensor(const unsigned int _sensorID, const string IP, const unsigned int port, const string _name, const string _lineEnd, const string _delimeter, const vector<FieldDescriptor> _fields, const string _startChars, const string _endChars);
    ~EthSensor();
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
