
#include "Spyder3TiffWriter.hpp"

extern "C"{
#include <tiffio.h>
}

#include <string>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <limits.h>

Spyder3TiffWriter::Spyder3TiffWriter(string outputFolderPath):super(outputFolderPath, 1000){
}

Spyder3TiffWriter::~Spyder3TiffWriter(){
}

void Spyder3TiffWriter::processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer){
    super::processFrame(lWidth, lHeight, lBuffer);

    TIFF *out = TIFFOpen(this->getNextImagePath("tif").c_str(),"w");
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, lWidth);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, lHeight);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
    TIFFSetField(out, TIFFTAG_JPEGQUALITY, 100);

    TIFFWriteEncodedStrip(out,0,(void*)lBuffer->GetDataPointer(),lWidth*lHeight);
    TIFFClose(out);
}
