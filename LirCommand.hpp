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

#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "EthSensor.hpp"
#include "Spyder3Camera.hpp"
#include "Spyder3TiffWriter.hpp"
//#include "Spyder3FrameTracker.hpp"

#include "LirSQLiteWriter.hpp"

using namespace std;

// Singleton code from: http://www.yolinux.com/TUTORIALS/C++Singleton.html
class LirCommand{

    boost::mutex commandMutex;
    bool running;
    char* outputFolder;

    // Camera and Logger Classes
    Spyder3Camera* camera;
    Spyder3TiffWriter* writer;
//    Spyder3FrameTracker* tracker;

    // Sensor Classes
    vector<EthSensor *> sensors;
    vector<LirSQLiteWriter *> sensorWriters;

    // Parser Functions
    std::map<std::string, boost::function<string (LirCommand*,const string)> > commands;

    private:
        LirCommand();
        LirCommand(LirCommand const&){};
        LirCommand& operator=(LirCommand const*){};
        static LirCommand* m_pInstance;
        void ConnectListeners();

        string receiveStatusCommand(const string message);
        string receiveStartCommand(const string message);
        string receiveStopCommand(const string message);

    public:
        static LirCommand* Instance();
        ~LirCommand();
        bool loadConfig(char* configPath);
        bool startLogger();
        bool stopLogger();
        string parseCommand(const string message);
};

#endif
