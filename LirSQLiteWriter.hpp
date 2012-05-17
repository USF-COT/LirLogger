
#ifndef LIRSQLITE3_HPP
#define LIRSQLITE3_HPP

#include "Spyder3TiffWriter.hpp"
#include "EthSensor.hpp"
#include "IEthSensorListener.hpp"
#include "sqlite3.h"
#include <time.h>

class LirSQLiteWriter : public IEthSensorListener{
    private:
        sqlite3 *db;
        string dbPath;
        Spyder3TiffWriter* camWriter; // Used to relate sensor readings to camera frames
        EthSensor* sensor;
        string insertStmt;
        vector<FieldDescriptor> fields;
    
    public:
        LirSQLiteWriter(Spyder3TiffWriter* _camWriter, EthSensor* _sensor, string outputDirectory);
        ~LirSQLiteWriter();
        virtual void sensorStarting();
        virtual void processReading(const EthSensorReadingSet set);
        virtual void sensorStopping();
};

#endif
