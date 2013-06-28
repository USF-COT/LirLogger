
#ifndef SPYDER3JPEGWRITER_HPP
#define SPYDER3JPEGWRITER_HPP

#include <string>
#include "Spyder3ImageWriter.hpp"

using namespace std;

class Spyder3JPEGWriter : public Spyder3ImageWriter{
    private:
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

    public:
        typedef Spyder3ImageWriter super;

        Spyder3JPEGWriter(string outputFolder);
        ~Spyder3JPEGWriter();

        virtual void processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer);
};

#endif
