
#ifndef FLOWMETER_HPP
#define FLOWMETER_HPP

#include <string.h>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "IFlowMeterListener.hpp"

using namespace std;

class FlowMeter{

private:
    vector<IFlowMeterListener *> listeners;

    boost::mutex runMutex;
    boost::mutex listenersMutex;
    bool running;

    // Socket Variables
    boost::asio::io_service ios;
    boost::asio::serial_port meter_port;
    boost::asio::streambuf buf;

    boost::thread* readThread;

    void setRunning(bool);
    void parseLine(const boost::system::error_code& ec, size_t bytes_transferred);

public:
    FlowMeter();
    ~FlowMeter();
    bool Connect();
    bool Disconnect();
    void addListener(IFlowMeterListener *l);
    void clearListeners();
    bool isRunning();
    void operator() ();
};

#endif
