
#include "Spyder3JPEGWriter.hpp"

#include <stdio.h>

#include <string>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <limits.h>

Spyder3JPEGWriter::Spyder3JPEGWriter(string outputFolderPath):super(outputFolderPath, 1000){
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
}

Spyder3JPEGWriter::~Spyder3JPEGWriter(){
    jpeg_destroy_compress(&cinfo);
}

void Spyder3JPEGWriter::processFrame(PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer){
    super::processFrame(lWidth, lHeight, lBuffer);

    FILE * outfile;
    JSAMPROW row_pointer[1];

    string path = this->getNextImagePath("jpg");

    if((outfile = fopen(path.c_str(), "wb")) == NULL){
        syslog(LOG_DAEMON|LOG_ERR, "Can't open file @ %s", path.c_str());
        return;
    }

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = lWidth;
    cinfo.image_height = lHeight;

    jpeg_set_defaults(&cinfo);

    jpeg_set_quality(&cinfo, 100, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    const PvUInt8 *buffer = lBuffer->GetDataPointer();

    while(cinfo.next_scanline < cinfo.image_height){
        row_pointer[0] = (JSAMPLE*) buffer + (cinfo.next_scanline * lWidth);
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
}
