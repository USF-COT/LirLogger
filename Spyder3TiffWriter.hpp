
#ifndef SPYDER3TIFFWRITER_HPP
#define SPYDER3TIFFWRITER_HPP

#include <string>
#include <utility>
#include <boost/thread.hpp>
#include "ISpyder3Listener.hpp"

using namespace std;

class Spyder3TiffWriter : public ISpyder3Listener{
    private:
        string outputFolderPath;
        unsigned long folderID;
        unsigned long frameID;

        boost::mutex idMutex;

        bool setNextFolderPath();
    
    public:
        Spyder3TiffWriter(string outputFolder);
        ~Spyder3TiffWriter();
        std::pair<unsigned long, unsigned long> getFolderFrameIDs();
        virtual void processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
};

#endif
