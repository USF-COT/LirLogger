
#include "ISpyder3Listener.hpp"

class Spyder3TiffWriter : public ISpyder3Listener{
    private:
        char* outputFolderPath;
        char* fullPath;
        unsigned long folderID;
        unsigned long frameID;
    
    public:
        Spyder3TiffWriter(char* _outputFolder);
        ~Spyder3TiffWriter();
        virtual void processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
};

