
#ifndef SPYDER3TIFFWRITER_HPP
#define SPYDER3TIFFWRITER_HPP

#include <string>
#include <utility>
#include <boost/thread.hpp>
#include "ISpyder3Listener.hpp"

class Spyder3TiffWriter : public ISpyder3Listener{
    private:
        char* outputFolderPath;
        char* fullPath;
        unsigned long folderID;
        unsigned long frameID;

        boost::mutex idMutex;
    
    public:
        Spyder3TiffWriter(char* _outputFolder);
        ~Spyder3TiffWriter();
        std::pair<unsigned long, unsigned long> getFolderFrameIDs();
        virtual void processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
};

#endif
