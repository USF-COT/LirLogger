
#include "Spyder3PNGWriter.hpp"

#include <string>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <limits.h>

#include <boost/filesystem.hpp>

Spyder3PNGWriter::Spyder3PNGWriter(string outputFolder) : super(outputFolder, 1000){
}

Spyder3PNGWriter::~Spyder3PNGWriter(){
}

void Spyder3PNGWriter::processFrame(unsigned long cameraID, PvUInt32 lWidth, PvUInt32 lHeight, const PvBuffer *lBuffer){
    super::processFrame(cameraID, lWidth, lHeight, lBuffer);

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(!png_ptr){
        syslog(LOG_DAEMON|LOG_ERR, "SEVERE: Unable to initialize png_ptr");
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        syslog(LOG_DAEMON|LOG_ERR, "SEVERE: Unable to initialize info_ptr");
        return;
    }
    
    string path = this->getNextImagePath("png");

    FILE *fp = fopen(path.c_str(), "wb");
    if(!fp){
        syslog(LOG_DAEMON|LOG_ERR, "Unable to open file to write PNG @ %s", path.c_str());
    }

    if(setjmp(png_jmpbuf(png_ptr))){
        syslog(LOG_DAEMON|LOG_ERR, "Error writing PNG to %s", path.c_str());
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, lWidth, lHeight, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_bytep row_pointers[lHeight];
    const PvUInt8* img_pointer = lBuffer->GetDataPointer();
    for(unsigned int i=0; i < lHeight; ++i){
        row_pointers[i] = (png_byte*) img_pointer + i*lWidth;  // Byte per pixel
    }

    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    fclose(fp);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}
