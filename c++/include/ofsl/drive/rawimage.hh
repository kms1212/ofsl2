#ifndef OFSL_DRIVE_RAWIMAGE_HH__
#define OFSL_DRIVE_RAWIMAGE_HH__

#include <ofsl/drive/drive.h>

#include <string>

#include <ofsl/drive/drive.hh>

namespace ofsl {

class RawImage : public Drive {
public:
    RawImage(std::string filename, bool readonly = false, size_t sector_size = 512);
    ~RawImage();
};

};

#endif
