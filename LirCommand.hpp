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
#include <pthread.h>

#include "EthSensor.hpp"
#include "Spyder3Camera.hpp"
#include "Spyder3TiffWriter.hpp"
//#include "Spyder3FrameTracker.hpp"

// Singleton code from: http://www.yolinux.com/TUTORIALS/C++Singleton.html
class LirCommand{

    pthread_mutex_t commandMutex;
    bool running;
    char* outputFolder;

    // Camera and Logger Classes
    Spyder3Camera* camera;
    Spyder3TiffWriter* writer;
//    Spyder3FrameTracker* tracker;

    // Sensor Classes
    std::vector<EthSensor *> sensors;

    private:
        LirCommand();
        LirCommand(LirCommand const&){};
        LirCommand& operator=(LirCommand const*){};
        static LirCommand* m_pInstance;
        void ConnectListeners();

    public:
        static LirCommand* Instance();
        ~LirCommand();
        bool loadConfig(char* configPath);
        bool startLogger();
        bool stopLogger();
};


