#ifndef OFSL_TIME_H__
#define OFSL_TIME_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Time Struct
 * field    value range     description
 * year     0 - 4096        Year
 * month    1 - 12          Month
 * day      1 - 31          Day of Month
 * hour     0 - 23          Hour (24)
 * minute   0 - 60          Minute
 * second   0 - 60          Second
 * nsec10   0 - 100,000,000 Nanoseconds in Second Divided by 10
 */
typedef struct ofsl_time {
    uint16_t year : 12;
    uint16_t month : 4;

    uint16_t day : 5;
    uint16_t hour : 5;
    uint16_t minute : 6;

    uint8_t second : 6;
    uint8_t : 1;
    uint8_t valid : 1;

    uint32_t nsec10;
} ofsl_time_t;

void ofsl_gettime(ofsl_time_t* time);

size_t ofsl_strffstime(char* sbuf, size_t sbufsz, const char* fmt, const ofsl_time_t* time);

void ofsl_calcyear(ofsl_time_t* time, int ydiff);
void ofsl_calcmonth(ofsl_time_t* time, int mdiff);
void ofsl_calcday(ofsl_time_t* time, int ddiff);
void ofsl_calchour(ofsl_time_t* time, int hdiff);
void ofsl_calcminute(ofsl_time_t* time, int mdiff);
void ofsl_calcsecond(ofsl_time_t* time, int sdiff);
void ofsl_calcnsec(ofsl_time_t* time, int nsdiff);

int ofsl_validate(ofsl_time_t* time);
int ofsl_validate_const(const ofsl_time_t* time);

#ifdef __cplusplus
};
#endif

#endif
