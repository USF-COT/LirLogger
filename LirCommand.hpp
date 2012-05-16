/*
 * LirCommand.cpp - Responsible for providing a thread-safe overview command interface to all Lir components
 *
 * By: Michael Lindemuth
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <iostream>
#include <vector>
#include <map>
#include <string.h>

#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "EthSensor.hpp"
#include "Spyder3Camera.hpp"
#include "Spyder3TiffWriter.hpp"
//#include "Spyder3FrameTracker.hpp"

#include "LirSQLiteWriter.hpp"

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
    std::vector<EthSensor *> sensors;
    std::vector<LirSQLiteWriter *> sensorWriters;

    // Parser Functions
    std::map<std::string, boost::function<void (LirCommand*,int,char*)> > commands;

    private:
        LirCommand();
        LirCommand(LirCommand const&){};
        LirCommand& operator=(LirCommand const*){};
        static LirCommand* m_pInstance;
        void ConnectListeners();

        void receiveStatusCommand(int connection, char* buffer);
        void receiveStartCommand(int connection, char* buffer);
        void receiveStopCommand(int connection, char* buffer);

    public:
        static LirCommand* Instance();
        ~LirCommand();
        bool loadConfig(char* configPath);
        bool startLogger();
        bool stopLogger();
        void parseCommand(int connection, char* buffer);
};


