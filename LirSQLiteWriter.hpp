
#ifndef LIRSQLITE3_HPP
#define LIRSQLITE3_HPP

#include "ISensor.hpp"
#include "ISensorListener.hpp"
#include "sqlite3.h"
#include <time.h>

#include "Spyder3ImageWriter.hpp"

#include <boost/thread.hpp>

class LirSQLiteWriter : public ISensorListener{
    private:
        boost::mutex pathMutex;
        sqlite3 *db;
        string outputFolder;
        string dbPath;
        Spyder3ImageWriter* camWriter; // Used to relate sensor readings to camera frames
        ISensor* sensor;
        string insertStmt;
        vector<FieldDescriptor> fields;

        time_t lastRowTimeLogged;
        bool logging;

        void initDatabase(string outputFolder);
    
    public:
        LirSQLiteWriter(Spyder3ImageWriter* _camWriter, ISensor* _sensor, string outputDirectory);
        ~LirSQLiteWriter();
        virtual void sensorStarting();
        void startLogging();
        virtual void processReading(const SensorReadingSet& set);
        void stopLogging();
        virtual void sensorStopping();
        void changeFolder(string outputFolder);
};

#endif
