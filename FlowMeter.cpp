
#include "FlowMeter.hpp"
#include <syslog.h>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <time.h>

using namespace std;

FlowMeter::FlowMeter(): ios(), meter_port(ios){
    running = false;
}

FlowMeter::~FlowMeter(){
    this->Disconnect(); // Run this in case the sensor is left connected
}

void FlowMeter::setRunning(bool value){
    runMutex.lock();
    this->running = value;
    runMutex.unlock();
}

bool FlowMeter::isRunning(){
    bool retVal;
    runMutex.lock();
    retVal = running;
    runMutex.unlock();

    return retVal;
}

bool FlowMeter::Connect(){
    if(!this->isRunning()){
        syslog(LOG_DAEMON|LOG_INFO,"Connecting flow meter");

        try{
            meter_port.open("/dev/ttyUSB0");
            meter_port.set_option(boost::asio::serial_port_base::baud_rate(9600));
            meter_port.set_option(boost::asio::serial_port_base::character_size(8));
            boost::asio::async_read_until(meter_port,buf,"\r\n",boost::bind(&FlowMeter::parseLine,this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            readThread = new boost::thread(boost::ref(*this));
            this->setRunning(true);
            syslog(LOG_DAEMON|LOG_INFO,"Flow Meter Connected");
        } catch (std::exception& e){
            syslog(LOG_DAEMON|LOG_ERR,"Unable to connect to flow meter. Error: %s",e.what());
        }
    } else {
        syslog(LOG_DAEMON|LOG_INFO,"Flow meter already running");
    }
}

void FlowMeter::parseLine(const boost::system::error_code& ec, size_t bytes_transferred){
    time_t timestamp = time(NULL);
    if(!ec){
        runMutex.lock();
        if(running){
            // Parse Line Here
            istream is(&buf);
            string line;
            getline(is,line);

            unsigned int i=0;
            boost::char_separator<char> sep(",", "", boost::drop_empty_tokens);
            boost::tokenizer< boost::char_separator<char> > tokens(line, sep);

            int counts = 0;
            double flowRate = 0;
            bool error = false;
            BOOST_FOREACH(string t, tokens){
                boost::algorithm::trim(t);
                if(i == 0){
                    try{
                        counts = boost::lexical_cast<int>(t);
                    } catch( boost::bad_lexical_cast const &){
                        syslog(LOG_DAEMON|LOG_ERR, "Unable to convert flow meter counts");
                        error = true;
                    }
                } else if (i == 1){
                    try{
                        flowRate = boost::lexical_cast<double>(t);
                    } catch( boost::bad_lexical_cast const &){
                        syslog(LOG_DAEMON|LOG_ERR, "Unable to convert flow rate");
                        error = true;
                    }
                }
                i++;
            }


            // Pass Reading to Handlers
            if(!error){
                listenersMutex.lock();
                BOOST_FOREACH(IFlowMeterListener* l, listeners){
                    l->processFlowReading(timestamp, counts, flowRate);
                }
                listenersMutex.unlock();
            }

            // Schedule next request
            boost::asio::async_read_until(meter_port,buf,"\r\n",boost::bind(&FlowMeter::parseLine,this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)); 
        } else {
            ios.stop();
        }
        runMutex.unlock();
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Error communicating with flow meter");
    }
}

bool FlowMeter::Disconnect(){
    if(this->isRunning()){
        this->setRunning(false);

        try{
            meter_port.close();
        } catch (std::exception& e){
            syslog(LOG_DAEMON|LOG_ERR,"Unable to disconnect flow meter.  Error: %s",e.what());
        }
        if(readThread) readThread->join();
        syslog(LOG_DAEMON|LOG_INFO,"Flow meter disconnected");            
    }
}

void FlowMeter::addListener(IFlowMeterListener *l){
    listenersMutex.lock();
    listeners.push_back(l);
    listenersMutex.unlock();
}

void FlowMeter::clearListeners(){
    listenersMutex.lock();
    BOOST_FOREACH(IFlowMeterListener* l, listeners){
        l->flowMeterStopping();
    }
    listeners.clear();
    listenersMutex.unlock();
}

void FlowMeter::operator() (){
    listenersMutex.lock();
    BOOST_FOREACH(IFlowMeterListener* l, listeners){
        l->flowMeterStarting();
    }
    listenersMutex.unlock();

    ios.reset();
    ios.run();
    
    listenersMutex.lock();
    BOOST_FOREACH(IFlowMeterListener* l, listeners){
        l->flowMeterStopping();
    }
    listenersMutex.unlock(); 
}

