
#ifndef SPYDER3TURBOJPEGWRITER_HPP
#define SPYDER3TURBOJPEGWRITER_HPP

#include <string>
#include "Spyder3ImageWriter.hpp"

extern "C"{
#include "turbojpeg.h"
}

using namespace std;

class Spyder3TurboJPEGWriter : public Spyder3ImageWriter{
    private:
        tjhandle compressor;

        unsigned long jpegBufferSize;
        unsigned char* jpegBuffer;

    public:
        typedef Spyder3ImageWriter super;

        Spyder3TurboJPEGWriter(string outputFolder);
        ~Spyder3TurboJPEGWriter();

        virtual void processFrame(unsigned long cameraID, PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
};

#endif
