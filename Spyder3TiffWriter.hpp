
#ifndef SPYDER3TIFFWRITER_HPP
#define SPYDER3TIFFWRITER_HPP

#include <string>
#include <utility>
#include <boost/thread.hpp>
#include "Spyder3ImageWriter.hpp"

using namespace std;

class Spyder3TiffWriter : public Spyder3ImageWriter{
    public:
        typedef Spyder3ImageWriter super;

        Spyder3TiffWriter(string outputFolder);
        ~Spyder3TiffWriter();

        virtual void processFrame(unsigned long cameraID, PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
};

#endif
