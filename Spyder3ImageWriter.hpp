
#ifndef SPYDER3IMAGEWRITER_HPP
#define SPYDER3IMAGEWRITER_HPP

#include <string>
#include <vector>
#include <boost/thread.hpp>

#include "ISpyder3Listener.hpp"
#include "ISpyder3ImageWriterListener.hpp"

using namespace std;

class Spyder3ImageWriter : public ISpyder3Listener{
    private:
        string outputFolderPath;

        boost::mutex idMutex;
        unsigned long folderID;
        unsigned long frameID;
        string nextImagePath;

        unsigned long numImagesPerFolder;
        bool setNextFolderPath();

        boost::mutex listenerMutex;
        vector<ISpyder3ImageWriterListener*> statsListeners;

    protected:
        string getNextImagePath(const string extension);
        void updateStatsListeners(unsigned long cameraID, unsigned long bytesWritten);

    public:
        Spyder3ImageWriter(string outputFolder, unsigned long numImagesPerFolder);
        ~Spyder3ImageWriter();
        unsigned long getFolderID();
        unsigned long getFrameID();
        virtual void processFrame(unsigned long cameraID, PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
        void changeFolder(string folderPath);

        void addStatsListener(ISpyder3ImageWriterListener* l);
        void clearStatsListeners();
};

#endif
