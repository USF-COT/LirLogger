
#ifndef SPYDER3PNGWRITER_HPP
#define SPYDER3PNGWRITER_HPP

#include <string>
#include <utility>
#include <boost/thread.hpp>
#include "Spyder3ImageWriter.hpp"

extern "C"{
#include <png.h>
}

using namespace std;

class Spyder3PNGWriter : public Spyder3ImageWriter{
    public:
        typedef Spyder3ImageWriter super;

        Spyder3PNGWriter(string outputFolder);
        ~Spyder3PNGWriter();

        virtual void processFrame(unsigned long cameraID, PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
};

#endif
