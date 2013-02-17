
#ifndef IETHSENSOR_HPP
#define IETHSENSOR_HPP

#include <string.h>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "IEthSensorListener.hpp"

using namespace std;

typedef struct{
    bool isNum;
    string name;
    string units;
}FieldDescriptor;

class EthSensor{    

private:
    unsigned int sensorID;
    string IP;
    unsigned int port;
    string name;
    string lineEnd;
    string delimeter;
    map<unsigned int, FieldDescriptor> fields;
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

    void setRunning(bool);
    void parseLine(const boost::system::error_code& ec, size_t bytes_transferred);

public:
    EthSensor(const unsigned int _sensorID, const string IP, const unsigned int port, const string _name, const string _lineEnd, const string _delimeter, const map<unsigned int, FieldDescriptor> _fields, const string _startChars, const string _endChars);
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
