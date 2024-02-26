#ifndef OFSL_DRIVE_DRIVE_HH__
#define OFSL_DRIVE_DRIVE_HH__

#include <cstdlib>

#include <ofsl/drive/drive.h>

#include <string>

namespace ofsl {

typedef lba_t lba_t;

class Drive {
private:
    OFSL_Drive* drv;

public:
    Drive(OFSL_Drive* drv);
    virtual ~Drive() = 0;

    OFSL_Drive* get_c_obj(void);

    int update_info(void);
    ssize_t read_sector(void* buf, lba_t lba, size_t sector_size, size_t cnt);
    ssize_t write_sector(const void* buf, lba_t lba, size_t sector_size, size_t cnt);
};

};

#endif
