
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
    private:
        png_structp png_ptr;
        png_infop info_ptr;

    public:
        typedef Spyder3ImageWriter super;

        Spyder3PNGWriter(string outputFolder);
        ~Spyder3PNGWriter();

        virtual void processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
};

#endif
