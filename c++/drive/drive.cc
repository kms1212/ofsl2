#include <ofsl/drive/drive.hh>
#include <ofsl/drive/drive.h>

#include <string>

ofsl::Drive::Drive(OFSL_Drive* drv)
{
    this->drv = drv;
}

ofsl::Drive::~Drive()
{
    ofsl_drive_delete(this->drv);
}

OFSL_Drive* ofsl::Drive::get_c_obj(void)
{
    return this->drv;
}

int ofsl::Drive::update_info(void)
{
    return ofsl_drive_update_info(this->drv);
}

ssize_t ofsl::Drive::read_sector(void* buf, lba_t lba, size_t sector_size, size_t cnt)
{
    return ofsl_drive_read_sector(this->drv, buf, lba, sector_size, cnt);
}

ssize_t ofsl::Drive::write_sector(const void* buf, lba_t lba, size_t sector_size, size_t cnt)
{
    return ofsl_drive_write_sector(this->drv, buf, lba, sector_size, cnt);
}

