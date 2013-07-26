
#ifndef LIRSQLITE3_HPP
#define LIRSQLITE3_HPP

#include "EthSensor.hpp"
#include "IEthSensorListener.hpp"
#include "IFlowMeterListener.hpp"
#include "sqlite3.h"
#include <time.h>

#include "Spyder3ImageWriter.hpp"

#include <boost/thread.hpp>

class LirSQLiteWriter : public IEthSensorListener, public IFlowMeterListener{
    private:
        boost::mutex pathMutex;
        sqlite3 *db;
        string outputFolder;
        string dbPath;
        Spyder3ImageWriter* camWriter; // Used to relate sensor readings to camera frames
        EthSensor* sensor;
        string insertStmt;
        vector<FieldDescriptor> fields;

        time_t lastRowTimeLogged;
        bool logging;

        void initDatabase(string outputFolder);
    
    public:
        LirSQLiteWriter(Spyder3ImageWriter* _camWriter, EthSensor* _sensor, string outputDirectory);
        ~LirSQLiteWriter();
        virtual void sensorStarting();
        void startLogging();
        virtual void processReading(const EthSensorReadingSet& set);
        void stopLogging();
        virtual void sensorStopping();
        void changeFolder(string outputFolder);

        // Flow Meter Methods
        virtual void flowMeterStarting();
        virtual void flowMeterStopping();
        virtual void processFlowReading(time_t timestamp, int counts, double flowRate);
};

#endif
