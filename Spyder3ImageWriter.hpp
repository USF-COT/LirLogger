
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

    protected:
        string getNextImagePath(const string extension);

    public:
        Spyder3ImageWriter(string outputFolder, unsigned long numImagesPerFolder);
        ~Spyder3ImageWriter();
        unsigned long getFolderID();
        unsigned long getFrameID();
        virtual void processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
        void changeFolder(string folderPath);
};

#endif
