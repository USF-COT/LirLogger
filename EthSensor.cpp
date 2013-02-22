
#include "EthSensor.hpp"
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

EthSensor::EthSensor(const unsigned int _sensorID, const string _IP, const unsigned int _port, const string _name, const string _lineEnd, const string _delimeter, const vector<FieldDescriptor> _fields, const string _startChars, const string _endChars) : sensorID(_sensorID), IP(_IP), port(_port), name(_name), lineEnd(_lineEnd), delimeter(_delimeter), fields(_fields), startChars(_startChars), endChars(_endChars){
    running = false;

}

EthSensor::~EthSensor(){
    this->Disconnect(); // Run this in case the sensor is left connected
}

void EthSensor::setRunning(bool value){
    runMutex.lock();
    this->running = value;
    runMutex.unlock();
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
        syslog(LOG_DAEMON|LOG_INFO,"Connecting %s sensor @ %s:%d",name.c_str(),IP.c_str(),port);
        char portString[16];
        snprintf(portString,15,"%u",port);

        try{
            boost::asio::ip::tcp::resolver resolver(ios);
            boost::asio::ip::tcp::resolver::query query(IP.c_str(),portString);

            readSock = new boost::asio::ip::tcp::socket(ios);
            boost::asio::connect(*readSock,resolver.resolve(query));
            if(startChars.length() > 0) readSock->write_some(boost::asio::buffer(startChars));
            boost::asio::async_read_until(*readSock,buf,lineEnd,boost::bind(&EthSensor::parseLine,this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            readThread = new boost::thread(boost::ref(*this));
            this->setRunning(true);
            syslog(LOG_DAEMON|LOG_INFO,"%s Sensor Connected", name.c_str());
        } catch (std::exception& e){
            syslog(LOG_DAEMON|LOG_ERR,"Unable to connect to ethernet sensor %s @ %s:%d.  Error: %s",name.c_str(),IP.c_str(),port,e.what());
        }
    } else {
        syslog(LOG_DAEMON|LOG_INFO,"Sensor %s already running",name.c_str());
    }
}

void EthSensor::parseLine(const boost::system::error_code& ec, size_t bytes_transferred){
    EthSensorReadingSet set;
    set.time = time(NULL);
    set.sensorName = this->name;
    if(!ec){
        runMutex.lock();
        if(running){
            // Parse Line Here
            istream is(&buf);
            string line;
            getline(is,line);

            boost::algorithm::trim(line);

            unsigned int i=0;
            boost::char_separator<char> sep(delimeter.c_str(), "", boost::drop_empty_tokens);
            boost::tokenizer< boost::char_separator<char> > tokens(line, sep);
            BOOST_FOREACH(string t, tokens){
                EthSensorReading r;
                //syslog(LOG_DAEMON|LOG_INFO, "Reading @ column %d for field ID %d: %s", i, fields[i].id, t.c_str());
                r.fieldID = fields[i].id;
                r.field = fields[i].name;
                r.text = t;
                if(fields[i].isNum){
                    try{
                        r.num = boost::lexical_cast<double>(t);
                        r.isNum = true;
                    } catch( boost::bad_lexical_cast const &){
                        r.isNum = false;
                    }
                }
                set.readings.push_back(r);
                i = i++ < fields.size()-1 ? i : i=0;
            }

            // Pass Reading to Handlers            
            listenersMutex.lock();
            for(i=0; i < listeners.size(); ++i){
                listeners[i]->processReading(set);
            }
            listenersMutex.unlock();

            // Schedule next request
            boost::asio::async_read_until(*readSock,buf,lineEnd,boost::bind(&EthSensor::parseLine,this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)); 
        } else {
            ios.stop();
        }
        runMutex.unlock();
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Error communicating with %s serial server.",name.c_str());
    }
}

bool EthSensor::Disconnect(){
    if(this->isRunning()){
        this->setRunning(false);

        try{
            if(endChars.length() > 0) readSock->write_some(boost::asio::buffer(endChars));
            readSock->shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
            readSock->close();
        } catch (std::exception& e){
            syslog(LOG_DAEMON|LOG_ERR,"Unable to disconnect ethernet sensor %s @ %s:%d.  Error: %s",name.c_str(),IP.c_str(),port,e.what());
        }
        if(readThread) readThread->join();
        syslog(LOG_DAEMON|LOG_INFO,"Sensor %s disconnected",name.c_str());            
    }
}

void EthSensor::addListener(IEthSensorListener *l){
    listenersMutex.lock();
    listeners.push_back(l);
    listenersMutex.unlock();
}

void EthSensor::clearListeners(){
    listenersMutex.lock();
    BOOST_FOREACH(IEthSensorListener* l, listeners){
        l->sensorStopping();
    }
    listeners.clear();
    listenersMutex.unlock();
}

void EthSensor::operator() (){
    listenersMutex.lock();
    BOOST_FOREACH(IEthSensorListener* l, listeners){
        l->sensorStarting();
    }
    listenersMutex.unlock();

    ios.reset();
    ios.run();
    
    listenersMutex.lock();
    BOOST_FOREACH(IEthSensorListener* l, listeners){
        l->sensorStopping();
    }
    listenersMutex.unlock(); 
}

unsigned int EthSensor::getID(){
    return this->sensorID;
}

string EthSensor::getName(){
    string retVal(this->name);
    return retVal;
}

vector<FieldDescriptor> EthSensor::getFieldDescriptors(){
    vector<FieldDescriptor> retVal(this->fields);
    return retVal;
}
