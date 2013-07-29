/*
 * LirCommand.cpp - Responsible for providing a thread-safe overview command interface to all Lir components
 *
 * By: Michael Lindemuth
 */

#ifndef LIRCOMMAND_HPP
#define LIRCOMMAND_HPP

#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <json/json.h>

#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "ISensor.hpp"
#include "FlowMeter.hpp"
#include "Spyder3Camera.hpp"
#include "Spyder3ImageWriter.hpp"
#include "Spyder3TurboJPEGWriter.hpp"
#include "ZMQCameraStatsPublisher.hpp"
#include "ZMQSensorPublisher.hpp"

#include "LirSQLiteWriter.hpp"

// Camera Stats Publisher is on START_PORT+1
// All sensors are on START_PORT+2 and above 
#define START_PORT 5555

using namespace std;

// Singleton code from: http://www.yoeinux.com/TUTORIALS/C++Singleton.html
class LirCommand{

    boost::mutex commandMutex;
    bool deploymentSet;
    bool running;

    // Output folder defaults
    string outputFolder;

    // Deployment Settings
    string authority;
    string authorityIP;
    unsigned int deploymentID;
    string cruise;
    string station;
    string UDR; 

    // Camera and Logger Classes
    Spyder3Camera* camera;
    Spyder3ImageWriter* writer;
    ZMQCameraStatsPublisher* camStatsPublisher;

//    Spyder3FrameTracker* tracker;

    // Sensor Classes
    map <unsigned int, ISensor *> sensors;
    map <unsigned int, LirSQLiteWriter *> sensorWriters;
    map <unsigned int, ZMQSensorPublisher *> sensorPublishers;

    // Parser Functions
    std::map<std::string, boost::function<string (LirCommand*,const string)> > commands;

    private:
        LirCommand();
        LirCommand(LirCommand const&){};
        LirCommand& operator=(LirCommand const*){};
        static LirCommand* m_pInstance;
        void connectCameraListeners();
        void connectSensorSQLWriters();

        string receiveStatusCommand(const string message);
        string receivePublishersCommand(const string message);
        string receiveStartCommand(const string message);
        string receiveStopCommand(const string message);

        string receiveSetDeploymentCommand(const string message);

        void findLastDeploymentStation();
        string generateFolderName();
        void addSensor(const Json::Value& logger, const Json::Value& sensorConfig);
        void setupFlowMeter();
        string setupUDR(const Json::Value& config);
        void setListenersOutputFolder();

    public:
        static LirCommand* Instance();
        ~LirCommand();
        bool startLogger();
        bool stopLogger();
        string parseCommand(const string message);
        Spyder3Stats getCameraStats();
        vector<SensorReadingSet> getSensorSets();
};

#endif
