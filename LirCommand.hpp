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


// Singleton code from: http://www.yolinux.com/TUTORIALS/C++Singleton.html
class LirCommand{

    pthread_mutex_t commandMutex;
    bool running;
    char* outputFolder;
    Spyder3Camera* camera;
    std::vector<EthSensor *> sensors;

    private:
        LirCommand();
        LirCommand(LirCommand const&){};
        LirCommand& operator=(LirCommand const*){};
        static LirCommand* m_pInstance;

    public:
        static LirCommand* Instance();
        ~LirCommand();
        bool loadConfig(char* configPath);
        bool startLogger();
        bool stopLogger();
};


