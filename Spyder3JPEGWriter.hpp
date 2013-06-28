
#ifndef SPYDER3JPEGWRITER_HPP
#define SPYDER3JPEGWRITER_HPP

#include <string>
#include <utility>
#include <boost/thread.hpp>
#include "ISpyder3Listener.hpp"

using namespace std;

class Spyder3JPEGWriter : public ISpyder3Listener{
    private:
        string outputFolderPath;

        boost::mutex idMutex;
        unsigned long folderID;
        unsigned long frameID;

        bool setNextFolderPath();
    
    public:
        Spyder3JPEGWriter(string outputFolder);
        ~Spyder3JPEGWriter();
        std::pair<unsigned long, unsigned long> getFolderFrameIDs();
        virtual void processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
        void changeFolder(string folderPath);
};

#endif
