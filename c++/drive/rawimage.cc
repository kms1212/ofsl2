#include <ofsl/drive/rawimage.hh>
#include <ofsl/drive/rawimage.h>

#include <string>

ofsl::RawImage::RawImage(std::string filename, bool readonly, size_t sector_size)
    : Drive(ofsl_drive_rawimage_create(filename.c_str(), readonly, sector_size))
{
    ;
}

ofsl::RawImage::~RawImage() = default;
