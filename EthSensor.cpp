
#include "EthSensor.hpp"
#include <syslog.h>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using namespace std;

EthSensor::EthSensor(const string _IP, const unsigned int _port, const string _name, const string _lineEnd, const string _delimeter, const vector<string> _fields, const string _startChars, const string _endChars) : IP(_IP), port(_port), name(_name), lineEnd(_lineEnd), delimeter(_delimeter), fields(_fields), startChars(_startChars), endChars(_endChars){
    running = false;
}

bool EthSensor::isRunning(){
    bool retVal;
    runMutex.lock();
    retVal = running;
    runMutex.unlock();

    return retVal;
}

bool EthSensor::Connect(){
    if(!this->isRunning()){
        running = true;
        syslog(LOG_DAEMON|LOG_INFO,"Connecting %s sensor @ %s:%d",name.c_str(),IP.c_str(),port);
        char portString[16];
        snprintf(portString,15,"%u",port);

        try{
            boost::asio::ip::tcp::resolver resolver(ios);
            boost::asio::ip::tcp::resolver::query query(IP.c_str(),portString);

            readSock = new boost::asio::ip::tcp::socket(ios);
            boost::asio::connect(*readSock,resolver.resolve(query));
            boost::asio::async_read_until(*readSock,buf,"\r\n",boost::bind(&EthSensor::parseLine,this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        } catch (std::exception& e){
            syslog(LOG_DAEMON|LOG_ERR,"Unable to connect to ethernet sensor %s @ %s:%d.  Error: %s",name.c_str(),IP.c_str(),port,e.what());
        }
    } else {
        syslog(LOG_DAEMON|LOG_INFO,"Sensor %s already running",name.c_str());
    }
}

void EthSensor::parseLine(const boost::system::error_code& ec, size_t bytes_transferred){
    syslog(LOG_DAEMON|LOG_INFO,"Parsing line from %s.",name.c_str());
    if(!ec){
        runMutex.lock();
        if(running){
            // Parse Line Here
            /*
            istream is(&buf);
            string line;
            getline(is,line);

            syslog(LOG_DAEMON|LOG_INFO,"Read line: %s",line.c_str());
            */
                        
            // Pass Reading to Handlers            

            // Schedule next request
            boost::asio::async_read_until(*readSock,buf,"\r\n",boost::bind(&EthSensor::parseLine,this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)); 
        }
        runMutex.unlock();
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Error communicating with %s serial server.",name.c_str());
    }
}

bool EthSensor::Disconnect(){
    if(this->isRunning()){
        runMutex.lock();
        running = false;
        runMutex.unlock();
        
        readSock->shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
        readSock->close();
        syslog(LOG_DAEMON|LOG_INFO,"Sensor %s disconnected",name.c_str());            
    }
}

void EthSensor::addListener(const IEthSensorListener *l){
    listenersMutex.lock();
    listeners.push_back(l);
    listenersMutex.unlock();
}

