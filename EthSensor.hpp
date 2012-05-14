
#ifndef IETHSENSOR_HPP
#define IETHSENSOR_HPP

#include <string.h>
#include <vector>
#include <boost/thread.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "IEthSensorListener.hpp"

using namespace std;

typedef struct{
    bool isNumeric;
    string name;
}FieldDescriptor;

class EthSensor{    

private:
    string IP;
    unsigned int port;
    string name;
    string lineEnd;
    string delimeter;
    vector<FieldDescriptor> fields;
    string startChars;
    string endChars;
    vector<IEthSensorListener *> listeners;

    boost::mutex runMutex;
    boost::mutex listenersMutex;
    bool running;

    // Socket Variables
    boost::asio::io_service ios;
    boost::asio::ip::tcp::socket* readSock;
    boost::asio::streambuf buf;

    boost::thread* readThread;

    void parseLine(const boost::system::error_code& ec, size_t bytes_transferred);

public:
    EthSensor(const string IP, const unsigned int port, const string _name, const string _lineEnd, const string _delimeter, const vector<FieldDescriptor> _fields, const string _startChars, const string _endChars);
    ~EthSensor();
    bool Connect();
    bool Disconnect();
    void addListener(IEthSensorListener *l);
    bool isRunning();
    void operator() ();
    vector<FieldDescriptor> getFieldDescriptors();
};

#endif
