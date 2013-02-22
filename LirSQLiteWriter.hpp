
#ifndef LIRSQLITE3_HPP
#define LIRSQLITE3_HPP

#include "Spyder3TiffWriter.hpp"
#include "EthSensor.hpp"
#include "IEthSensorListener.hpp"
#include "sqlite3.h"
#include <time.h>

#include <boost/thread.hpp>

class LirSQLiteWriter : public IEthSensorListener{
    private:
        boost::mutex pathMutex;
        sqlite3 *db;
        string outputFolder;
        string dbPath;
        Spyder3TiffWriter* camWriter; // Used to relate sensor readings to camera frames
        EthSensor* sensor;
        string insertStmt;
        vector<FieldDescriptor> fields;

        time_t lastRowTimeLogged;
        bool logging;

        void initDatabase(string outputFolder);
    
    public:
        LirSQLiteWriter(Spyder3TiffWriter* _camWriter, EthSensor* _sensor, string outputDirectory);
        ~LirSQLiteWriter();
        virtual void sensorStarting();
        void startLogging();
        virtual void processReading(const EthSensorReadingSet set);
        void stopLogging();
        virtual void sensorStopping();
        void changeFolder(string outputFolder);
};

#endif
