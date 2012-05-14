
#ifndef LIRSQLITE3_HPP
#define LIRSQLITE3_HPP

#include "Spyder3TiffWriter.hpp"
#include "IEthSensorListener.hpp"
#include "sqlite3.h"
#include <time.h>

class LirSQLiteWriter : public IEthSensorListener{
    private:
        sqlite3 *db;
        string dbPath;
        Spyder3TiffWriter* camWriter; // Used to relate sensor readings to camera frames
    
    public:
        LirSQLiteWriter(Spyder3TiffWriter* _camWriter, string outputDirectory);
        ~LirSQLiteWriter();
        virtual void processReading(time_t time, const vector<EthSensorReading>& readings);
};

#endif
